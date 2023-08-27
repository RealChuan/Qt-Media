#include "player.h"
#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "formatcontext.h"
#include "packet.h"
#include "subtitledecoder.h"
#include "videodecoder.h"

#include <utils/utils.h>
#include <videorender/videorender.hpp>

#include <QImage>

#include <memory>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class Player::PlayerPrivate
{
public:
    PlayerPrivate(Player *q)
        : q_ptr(q)
    {
        formatCtx = new FormatContext(q_ptr);

        audioInfo = new AVContextInfo(q_ptr);
        videoInfo = new AVContextInfo(q_ptr);
        subtitleInfo = new AVContextInfo(q_ptr);

        audioDecoder = new AudioDecoder(q_ptr);
        videoDecoder = new VideoDecoder(q_ptr);
        subtitleDecoder = new SubtitleDecoder(q_ptr);
    }

    auto initAvCodec() -> bool
    {
        isopen = false;

        //初始化pFormatCtx结构
        if (!formatCtx->openFilePath(filepath)) {
            return false;
        }

        //获取音视频流数据信息
        if (!formatCtx->findStream()) {
            return false;
        }

        emit q_ptr->durationChanged(formatCtx->duration());
        q_ptr->onPositionChanged(0);

        audioInfo->resetIndex();
        auto audioTracks = formatCtx->audioMap();
        if (!audioTracks.isEmpty()) {
            int audioIndex = formatCtx->findBestStreamIndex(AVMEDIA_TYPE_AUDIO);
            if (!setMediaIndex(audioInfo, audioIndex)) {
                return false;
            }
            emit q_ptr->audioTracksChanged(audioTracks.values());
            emit q_ptr->audioTrackChanged(audioTracks.value(audioIndex));
        }

        if (!formatCtx->coverImage().isNull()) {
            for (auto render : videoRenders) {
                render->setImage(formatCtx->coverImage());
            }
        }

        videoInfo->resetIndex();
        auto videoIndexs = formatCtx->videoIndexs();
        if (!videoIndexs.isEmpty()) {
            int videoIndex = videoIndexs.first();
            if (!setMediaIndex(videoInfo, videoIndex)) {
                videoInfo->resetIndex();
            }
        }

        subtitleInfo->resetIndex();
        auto subtitleTracks = formatCtx->subtitleMap();
        if (!subtitleTracks.isEmpty()) {
            int subtitleIndex = formatCtx->findBestStreamIndex(AVMEDIA_TYPE_SUBTITLE);
            if (!setMediaIndex(subtitleInfo, subtitleIndex)) {
                return false;
            }
            subtitleDecoder->setVideoResolutionRatio(resolutionRatio());
            emit q_ptr->subTracksChanged(subtitleTracks.values());
            emit q_ptr->subTrackChanged(subtitleTracks.value(subtitleIndex));
        }

        qDebug() << audioInfo->index() << videoInfo->index() << subtitleInfo->index();
        if (!audioInfo->isIndexVaild() && !videoInfo->isIndexVaild()) {
            return false;
        }

        isopen = true;
        // formatCtx->printFileInfo();
        formatCtx->dumpFormat();
        return true;
    }

    void playVideo()
    {
        Q_ASSERT(isopen);

        formatCtx->discardStreamExcluded(
            {audioInfo->index(), videoInfo->index(), subtitleInfo->index()});
        formatCtx->seekFirstFrame();

        videoDecoder->startDecoder(formatCtx, videoInfo);
        subtitleDecoder->startDecoder(formatCtx, subtitleInfo);
        audioDecoder->startDecoder(formatCtx, audioInfo);

        if (audioInfo->isIndexVaild()) {
            audioDecoder->setMasterClock();
        } else if (videoInfo->isIndexVaild()) {
            videoDecoder->setMasterClock();
        } else {
            Q_ASSERT(false);
        }

        emit q_ptr->playStarted();

        qint64 readSize = 0;
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();

        while (runing) {
            PacketPtr packetPtr(new Packet);
            if (!formatCtx->readFrame(packetPtr.data())) {
                break;
            }

            calculateSpeed(elapsedTimer, readSize, packetPtr->avPacket()->size);

            auto stream_index = packetPtr->streamIndex();
            if (!formatCtx->checkPktPlayRange(packetPtr.data())) {
            } else if (audioInfo->isIndexVaild()
                       && stream_index == audioInfo->index()) { // 如果是音频数据
                audioDecoder->append(packetPtr);
            } else if (videoInfo->isIndexVaild() && stream_index == videoInfo->index()
                       && !(videoInfo->stream()->disposition
                            & AV_DISPOSITION_ATTACHED_PIC)) { // 如果是视频数据
                videoDecoder->append(packetPtr);
            } else if (subtitleInfo->isIndexVaild()
                       && stream_index == subtitleInfo->index()) { // 如果是字幕数据
                subtitleDecoder->append(packetPtr);
            }
        }
        while (runing && (videoDecoder->size() > 0 || audioDecoder->size() > 0)) {
            msleep(Sleep_Queue_Full_Milliseconds);
        }
        subtitleDecoder->stopDecoder();
        videoDecoder->stopDecoder();
        audioDecoder->stopDecoder();

        qInfo() << "play finish";
    }

    auto setMediaIndex(AVContextInfo *contextInfo, int index) -> bool
    {
        contextInfo->setIndex(index);
        contextInfo->setStream(formatCtx->stream(index));
        if (!contextInfo->initDecoder(formatCtx->guessFrameRate(index))) {
            return false;
        }
        return contextInfo->openCodec(gpuDecode ? AVContextInfo::GpuType::GpuDecode
                                                : AVContextInfo::GpuType::NotUseGpu);
    }

    void setMediaState(MediaState mediaState)
    {
        mediaState = mediaState;
        emit q_ptr->stateChanged(mediaState);
    }

    QSize resolutionRatio() const
    {
        return videoInfo->isIndexVaild() ? videoInfo->resolutionRatio() : QSize();
    }

    void calculateSpeed(QElapsedTimer &timer, qint64 &readSize, qint64 addSize)
    {
        readSize += addSize;
        auto elapsed = timer.elapsed();
        if (elapsed < 5000) {
            return;
        }

        auto speed = readSize * 1000 / elapsed;
        emit q_ptr->readSpeedChanged(speed);
        timer.restart();
        readSize = 0;
    }

    Player *q_ptr;

    FormatContext *formatCtx;
    AVContextInfo *audioInfo;
    AVContextInfo *videoInfo;
    AVContextInfo *subtitleInfo;

    AudioDecoder *audioDecoder;
    VideoDecoder *videoDecoder;
    SubtitleDecoder *subtitleDecoder;

    QString filepath;
    std::atomic_bool isopen = true;
    std::atomic_bool runing = true;
    bool gpuDecode = false;
    qint64 position = 0;

    std::atomic<Player::MediaState> mediaState = Player::MediaState::StoppedState;

    QVector<VideoRender *> videoRenders = {};
};

Player::Player(QObject *parent)
    : QThread(parent)
    , d_ptr(new PlayerPrivate(this))
{
    av_log_set_level(AV_LOG_INFO);

    buildConnect();
    buildErrorConnect();
}

Player::~Player()
{
    onStop();
}

QString &Player::filePath() const
{
    return d_ptr->filepath;
}

void Player::openMedia(const QString &filepath)
{
    onStop();
    d_ptr->filepath = filepath;
    d_ptr->initAvCodec();
    if (!d_ptr->isopen) {
        qWarning() << "initAvCode Error";
        return;
    }
    onPlay();
}

void Player::onPlay()
{
    buildConnect(true);
    d_ptr->setMediaState(MediaState::PlayingState);
    d_ptr->runing = true;
    start();
}

void Player::onStop()
{
    buildConnect(false);
    d_ptr->runing = false;
    if (isRunning()) {
        quit();
    }
    while (isRunning()) {
        qApp->processEvents();
    }
    int count = 1000;
    while (count-- > 0) {
        qApp->processEvents(); // just for signal finished
    }
    for (auto render : d_ptr->videoRenders) {
        render->resetAllFrame();
    }
    d_ptr->formatCtx->close();
    d_ptr->setMediaState(MediaState::StoppedState);
}

void Player::onPositionChanged(qint64 position)
{
    auto diff = (position - d_ptr->position) / AV_TIME_BASE;
    if (qAbs(diff) < 1) {
        return;
    }
    d_ptr->position = position;
    emit positionChanged(position);
}

void Player::seek(qint64 position)
{
    qInfo() << "Seek To: "
            << QTime::fromMSecsSinceStartOfDay(position / 1000).toString("hh:mm:ss.zzz");
}

void Player::setAudioTrack(const QString &text) // 停止再播放最简单 之后在优化
{
    auto audioTracks = d_ptr->formatCtx->audioMap();
    auto list = audioTracks.values();
    if (!list.contains(text)) {
        return;
    }
    int index = audioTracks.key(text);
    int subtitleIndex = d_ptr->subtitleInfo->index();

    onStop();
    d_ptr->initAvCodec();
    if (subtitleIndex >= 0) {
        if (!d_ptr->setMediaIndex(d_ptr->subtitleInfo, subtitleIndex)) {
            return;
        }
        emit subTrackChanged(d_ptr->formatCtx->subtitleMap().value(subtitleIndex));
    }
    if (!d_ptr->setMediaIndex(d_ptr->audioInfo, index)) {
        return;
    }
    emit audioTrackChanged(text);
    seek(d_ptr->position);
    onPlay();
}

void Player::setSubtitleTrack(const QString &text)
{
    auto subtitleTracks = d_ptr->formatCtx->subtitleMap();
    auto list = subtitleTracks.values();
    if (!list.contains(text)) {
        return;
    }
    int index = subtitleTracks.key(text);
    int audioIndex = d_ptr->audioInfo->index();

    onStop();
    d_ptr->initAvCodec();
    if (audioIndex >= 0) {
        if (!d_ptr->setMediaIndex(d_ptr->audioInfo, audioIndex)) {
            return;
        }
        emit audioTrackChanged(d_ptr->formatCtx->audioMap().value(audioIndex));
    }
    if (!d_ptr->setMediaIndex(d_ptr->subtitleInfo, index)) {
        return;
    }
    emit subTrackChanged(text);
    seek(d_ptr->position);
    onPlay();
}

bool Player::isOpen()
{
    return d_ptr->isopen;
}

void Player::setVolume(qreal volume)
{
    if ((volume < 0) || (volume > 1)) {
        return;
    }
    d_ptr->audioDecoder->setVolume(volume);
}

void Player::setSpeed(double speed)
{
    if (speed <= 0) {
        qWarning() << tr("Speed cannot be less than or equal to 0!");
        return;
    }
}

double Player::speed()
{
    return 1.0;
}

void Player::pause(bool status)
{
    if (status) {
        d_ptr->setMediaState(MediaState::PausedState);
    } else if (isRunning()) {
        d_ptr->setMediaState(MediaState::PlayingState);
    } else {
        d_ptr->setMediaState(MediaState::StoppedState);
    }
}

void Player::setUseGpuDecode(bool on)
{
    d_ptr->gpuDecode = on;
    if (!isRunning()) {
        return;
    }
    int audioIndex = d_ptr->audioInfo->index();
    int subtitleIndex = d_ptr->subtitleInfo->index();
    onStop();
    d_ptr->initAvCodec();
    if (audioIndex >= 0) {
        if (!d_ptr->setMediaIndex(d_ptr->audioInfo, audioIndex)) {
            return;
        }
        emit audioTrackChanged(d_ptr->formatCtx->audioMap().value(audioIndex));
    }
    if (subtitleIndex >= 0) {
        if (!d_ptr->setMediaIndex(d_ptr->subtitleInfo, subtitleIndex)) {
            return;
        }
        emit subTrackChanged(d_ptr->formatCtx->subtitleMap().value(subtitleIndex));
    }
    seek(d_ptr->position);
    onPlay();
}

bool Player::isGpuDecode()
{
    return d_ptr->gpuDecode;
}

Player::MediaState Player::mediaState()
{
    return d_ptr->mediaState;
}

qint64 Player::duration() const
{
    return d_ptr->formatCtx->duration();
}

qint64 Player::position() const
{
    return d_ptr->position;
}

qint64 Player::fames() const
{
    return d_ptr->videoInfo->isIndexVaild() ? d_ptr->videoInfo->fames() : 0;
}

QSize Player::resolutionRatio() const
{
    return d_ptr->resolutionRatio();
}

double Player::fps() const
{
    return d_ptr->videoInfo->isIndexVaild() ? d_ptr->videoInfo->fps() : 0;
}

int Player::audioIndex() const
{
    return d_ptr->audioInfo->index();
}

int Player::videoIndex() const
{
    return d_ptr->videoInfo->index();
}

int Player::subtitleIndex() const
{
    return d_ptr->subtitleInfo->index();
}

void Player::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->videoRenders = videoRenders;
    d_ptr->videoDecoder->setVideoRenders(videoRenders);
    d_ptr->subtitleDecoder->setVideoRenders(videoRenders);
}

QVector<VideoRender *> Player::videoRenders()
{
    return d_ptr->videoRenders;
}

void Player::run()
{
    if (!d_ptr->isopen) {
        return;
    }
    d_ptr->playVideo();
}

void Player::buildConnect(bool state)
{
    if (state) {
        connect(d_ptr->audioDecoder,
                &AudioDecoder::positionChanged,
                this,
                &Player::onPositionChanged,
                Qt::UniqueConnection);
        connect(d_ptr->videoDecoder,
                &VideoDecoder::positionChanged,
                this,
                &Player::onPositionChanged,
                Qt::UniqueConnection);
    } else {
        disconnect(d_ptr->audioDecoder,
                   &AudioDecoder::positionChanged,
                   this,
                   &Player::onPositionChanged);
        disconnect(d_ptr->videoDecoder,
                   &VideoDecoder::positionChanged,
                   this,
                   &Player::onPositionChanged);
    }
}

void Player::buildErrorConnect()
{
    connect(AVErrorManager::instance(), &AVErrorManager::error, this, &Player::error);
}

} // namespace Ffmpeg
