#include "player.h"
#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "averror.h"
#include "formatcontext.h"
#include "packet.h"
#include "subtitledecoder.h"
#include "videodecoder.h"

#include <utils/utils.h>
#include <videooutput/videooutputrender.hpp>

#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QThread>
#include <QWaitCondition>

#include <memory>

namespace Ffmpeg {

class Player::PlayerPrivate
{
public:
    PlayerPrivate(QObject *parent)
        : owner(parent)
    {
        formatCtx = new FormatContext(owner);

        audioInfo = new AVContextInfo(AVContextInfo::Audio, owner);
        videoInfo = new AVContextInfo(AVContextInfo::Video, owner);
        subtitleInfo = new AVContextInfo(AVContextInfo::SubTiTle, owner);

        audioDecoder = new AudioDecoder(owner);
        videoDecoder = new VideoDecoder(owner);
        subtitleDecoder = new SubtitleDecoder(owner);
    }
    QObject *owner;

    FormatContext *formatCtx;
    AVContextInfo *audioInfo;
    AVContextInfo *videoInfo;
    AVContextInfo *subtitleInfo;

    AudioDecoder *audioDecoder;
    VideoDecoder *videoDecoder;
    SubtitleDecoder *subtitleDecoder;

    QString filepath;
    volatile bool isopen = true;
    volatile bool runing = true;
    volatile bool seek = false;
    qint64 seekTime = 0; // seconds

    volatile Player::MediaState mediaState = Player::MediaState::StoppedState;

    QVector<VideoOutputRender *> videoOutputRenders;
};

Player::Player(QObject *parent)
    : QThread(parent)
    , d_ptr(new PlayerPrivate(this))
{
    qInfo() << avcodec_configuration();
    qInfo() << avcodec_version();
    qRegisterMetaType<Ffmpeg::AVError>("Ffmpeg::AVError");
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

void Player::onSetFilePath(const QString &filepath)
{
    onStop();
    d_ptr->filepath = filepath;
    d_ptr->audioDecoder->setIsLocalFile(QFile::exists(filepath));
    initAvCode();
    if (!d_ptr->isopen) {
        qWarning() << "initAvCode Error";
        return;
    }
    onPlay();
}

void Player::onPlay()
{
    buildConnect(true);
    setMediaState(MediaState::PlayingState);
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
    for (auto render : d_ptr->videoOutputRenders) {
        render->onFinish();
    }
    d_ptr->formatCtx->close();
    setMediaState(MediaState::StoppedState);
}

void Player::onSeek(int timestamp)
{
    qDebug() << "Seek: " << timestamp;

    if (d_ptr->seek) {
        return;
    }
    blockSignals(true);
    d_ptr->seek = true;
    d_ptr->seekTime = timestamp;
}

void Player::onSetAudioTracks(const QString &text) // 停止再播放最简单 之后在优化
{
    auto audioTracks = d_ptr->formatCtx->audioMap();
    auto list = audioTracks.values();
    if (!list.contains(text)) {
        return;
    }
    int index = audioTracks.key(text);
    int subtitleIndex = d_ptr->subtitleInfo->index();

    onStop();
    initAvCode();
    if (subtitleIndex >= 0) {
        if (!setMediaIndex(d_ptr->subtitleInfo, subtitleIndex)) {
            return;
        }
        emit subtitleStreamChanged(d_ptr->formatCtx->subtitleMap().value(subtitleIndex));
    }

    if (!setMediaIndex(d_ptr->audioInfo, index)) {
        return;
    }
    emit audioTrackChanged(text);
    onSeek(d_ptr->audioDecoder->clock());
    onPlay();
}

void Player::onSetSubtitleStream(const QString &text)
{
    auto subtitleTracks = d_ptr->formatCtx->subtitleMap();
    auto list = subtitleTracks.values();
    if (!list.contains(text)) {
        return;
    }
    int index = subtitleTracks.key(text);
    int audioIndex = d_ptr->audioInfo->index();

    onStop();
    initAvCode();
    if (audioIndex >= 0) {
        if (!setMediaIndex(d_ptr->audioInfo, audioIndex))
            return;
        emit audioTrackChanged(d_ptr->formatCtx->audioMap().value(audioIndex));
    }
    if (!setMediaIndex(d_ptr->subtitleInfo, index))
        return;
    emit subtitleStreamChanged(text);
    onSeek(d_ptr->audioDecoder->clock());
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
    d_ptr->audioDecoder->setSpeed(speed);
    d_ptr->videoDecoder->setSpeed(speed);
    d_ptr->subtitleDecoder->setSpeed(speed);
}

double Player::speed()
{
    return d_ptr->audioDecoder->speed();
}

bool Player::initAvCode()
{
    d_ptr->isopen = false;

    //初始化pFormatCtx结构
    if (!d_ptr->formatCtx->openFilePath(d_ptr->filepath)) {
        return false;
    }

    //获取音视频流数据信息
    if (!d_ptr->formatCtx->findStream()) {
        return false;
    }

    emit durationChanged(d_ptr->formatCtx->duration());
    emit positionChanged(0);

    d_ptr->audioInfo->resetIndex();
    QMap<int, QString> audioTracks = d_ptr->formatCtx->audioMap();
    if (!audioTracks.isEmpty()) {
        int audioIndex = audioTracks.firstKey();
        if (!setMediaIndex(d_ptr->audioInfo, audioIndex)) {
            return false;
        }
        emit audioTracksChanged(audioTracks.values());
        emit audioTrackChanged(audioTracks.value(audioIndex));
    }

    if (!d_ptr->formatCtx->coverImage().isNull()) {
        for (auto render : d_ptr->videoOutputRenders) {
            render->setDisplayImage(d_ptr->formatCtx->coverImage());
        }
    }

    d_ptr->videoInfo->resetIndex();
    if (!d_ptr->formatCtx->videoIndexs().isEmpty()) {
        int videoIndex = d_ptr->formatCtx->videoIndexs().at(0);
        if (!setMediaIndex(d_ptr->videoInfo, videoIndex)) {
            d_ptr->videoInfo->resetIndex();
        }
    }

    d_ptr->subtitleInfo->resetIndex();
    QMap<int, QString> subtitleTracks = d_ptr->formatCtx->subtitleMap();
    if (!subtitleTracks.isEmpty()) {
        int subtitleIndex = subtitleTracks.firstKey();
        if (!setMediaIndex(d_ptr->subtitleInfo, subtitleIndex)) {
            return false;
        }
        emit subtitleStreamsChanged(subtitleTracks.values());
        emit subtitleStreamChanged(subtitleTracks.value(subtitleIndex));
    }

    qDebug() << d_ptr->audioInfo->index() << d_ptr->videoInfo->index()
             << d_ptr->subtitleInfo->index();
    if (!d_ptr->audioInfo->isIndexVaild() && !d_ptr->videoInfo->isIndexVaild()) {
        return false;
    }

    d_ptr->isopen = true;
    d_ptr->formatCtx->dumpFormat();
    return true;
}

void Player::playVideo()
{
    d_ptr->formatCtx->seekFirstFrame();

    d_ptr->videoDecoder->startDecoder(d_ptr->formatCtx, d_ptr->videoInfo);
    d_ptr->subtitleDecoder->startDecoder(d_ptr->formatCtx, d_ptr->subtitleInfo);
    d_ptr->audioDecoder->startDecoder(d_ptr->formatCtx, d_ptr->audioInfo);
    emit playStarted();

    while (d_ptr->runing) {
        checkSeek();

        std::unique_ptr<Packet> packetPtr(new Packet);
        if (!d_ptr->formatCtx->readFrame(packetPtr.get())) {
            break;
        }

        if (d_ptr->formatCtx->checkPktPlayRange(packetPtr.get()) <= 0) {
        } else if (d_ptr->audioInfo->isIndexVaild()
                   && packetPtr->avPacket()->stream_index
                          == d_ptr->audioInfo->index()) { // 如果是音频数据
            d_ptr->audioDecoder->append(packetPtr.release());
        } else if (d_ptr->videoInfo->isIndexVaild()
                   && packetPtr->avPacket()->stream_index == d_ptr->videoInfo->index()
                   && !(d_ptr->videoInfo->stream()->disposition
                        & AV_DISPOSITION_ATTACHED_PIC)) { // 如果是视频数据
            d_ptr->videoDecoder->append(packetPtr.release());
        } else if (d_ptr->subtitleInfo->isIndexVaild()
                   && packetPtr->avPacket()->stream_index == d_ptr->subtitleInfo->index()) {
            d_ptr->subtitleDecoder->append(packetPtr.release());
        }
        while (d_ptr->runing && !d_ptr->seek
               && (d_ptr->videoDecoder->size() > Max_Frame_Size
                   || d_ptr->audioDecoder->size() > Max_Frame_Size)) {
            msleep(Sleep_Queue_Full_Milliseconds);
        }
    }
    while (d_ptr->runing && (d_ptr->videoDecoder->size() > 0 || d_ptr->audioDecoder->size() > 0)) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }
    d_ptr->subtitleDecoder->stopDecoder();
    d_ptr->videoDecoder->stopDecoder();
    d_ptr->audioDecoder->stopDecoder();

    qInfo() << "play finish";
}

void Player::checkSeek()
{
    if (!d_ptr->seek) {
        return;
    }
    QElapsedTimer timer;
    timer.start();

    QSharedPointer<Utils::CountDownLatch> latchPtr(new Utils::CountDownLatch(3));
    d_ptr->videoDecoder->seek(d_ptr->seekTime, latchPtr);
    d_ptr->audioDecoder->seek(d_ptr->seekTime, latchPtr);
    d_ptr->subtitleDecoder->seek(d_ptr->seekTime, latchPtr);
    latchPtr->wait();
    d_ptr->formatCtx->seek(d_ptr->seekTime);
    if (d_ptr->videoInfo->isIndexVaild()) {
        d_ptr->videoInfo->flush();
    }
    if (d_ptr->audioInfo->isIndexVaild()) {
        d_ptr->audioInfo->flush();
    }
    if (d_ptr->subtitleInfo->isIndexVaild()) {
        d_ptr->subtitleInfo->flush();
    }
    d_ptr->seek = false;
    blockSignals(false);
    setMediaState(MediaState::PlayingState);
    qDebug() << "Seek ElapsedTimer: " << timer.elapsed();
    emit seekFinished();
}

void Player::setMediaState(Player::MediaState mediaState)
{
    d_ptr->mediaState = mediaState;
    emit stateChanged(d_ptr->mediaState);
}

bool Player::setMediaIndex(AVContextInfo *contextInfo, int index)
{
    contextInfo->setIndex(index);
    contextInfo->setStream(d_ptr->formatCtx->stream(index));
    if (!contextInfo->findDecoder()) {
        return false;
    }
    return true;
}

void Player::buildErrorConnect()
{
    connect(d_ptr->formatCtx, &FormatContext::error, this, &Player::error);
    connect(d_ptr->audioInfo, &AVContextInfo::error, this, &Player::error);
    connect(d_ptr->videoInfo, &AVContextInfo::error, this, &Player::error);
    connect(d_ptr->subtitleInfo, &AVContextInfo::error, this, &Player::error);
}

void Player::pause(bool status)
{
    d_ptr->audioDecoder->pause(status);
    d_ptr->videoDecoder->pause(status);
    d_ptr->subtitleDecoder->pause(status);
    if (status) {
        setMediaState(MediaState::PausedState);
    } else if (isRunning()) {
        setMediaState(MediaState::PlayingState);
    } else {
        setMediaState(MediaState::StoppedState);
    }
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
    return d_ptr->audioDecoder->clock() * 1000;
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

void Player::setVideoOutputWidget(QVector<VideoOutputRender *> videoOutputRenders)
{
    d_ptr->videoOutputRenders = videoOutputRenders;
    connect(this,
            &Player::subtitleImages,
            this,
            [this](const QVector<Ffmpeg::SubtitleImage> &SubtitleImages) {
                for (auto render : d_ptr->videoOutputRenders) {
                    render->onSubtitleImages(SubtitleImages);
                }
            });
    d_ptr->videoDecoder->setVideoOutputRenders(videoOutputRenders);
}

void Player::run()
{
    if (!d_ptr->isopen) {
        return;
    }
    playVideo();
}

void Player::buildConnect(bool state)
{
    if (state) {
        connect(d_ptr->audioDecoder,
                &AudioDecoder::positionChanged,
                this,
                &Player::positionChanged,
                Qt::UniqueConnection);
        connect(d_ptr->subtitleDecoder,
                &SubtitleDecoder::subtitleImages,
                this,
                &Player::subtitleImages,
                Qt::UniqueConnection);
    } else {
        disconnect(d_ptr->audioDecoder,
                   &AudioDecoder::positionChanged,
                   this,
                   &Player::positionChanged);
        disconnect(d_ptr->subtitleDecoder,
                   &SubtitleDecoder::subtitleImages,
                   this,
                   &Player::subtitleImages);
    }
}

} // namespace Ffmpeg
