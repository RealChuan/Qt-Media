#include "player.h"
#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "clock.hpp"
#include "codeccontext.h"
#include "formatcontext.h"
#include "packet.h"
#include "subtitledecoder.h"
#include "videodecoder.h"

#include <event/errorevent.hpp>
#include <event/trackevent.hpp>
#include <event/valueevent.hpp>
#include <utils/speed.hpp>
#include <utils/threadsafequeue.hpp>
#include <utils/utils.h>
#include <videorender/videorender.hpp>

#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class Player::PlayerPrivate
{
public:
    explicit PlayerPrivate(Player *q)
        : q_ptr(q)
    {
        formatCtx = new FormatContext(q_ptr);

        audioInfo = new AVContextInfo(q_ptr);
        videoInfo = new AVContextInfo(q_ptr);
        subtitleInfo = new AVContextInfo(q_ptr);

        audioDecoder = new AudioDecoder(q_ptr);
        videoDecoder = new VideoDecoder(q_ptr);
        subtitleDecoder = new SubtitleDecoder(q_ptr);

        QObject::connect(AVErrorManager::instance(),
                         &AVErrorManager::error,
                         q_ptr,
                         [this](const AVError &error) { addEvent(new ErrorEvent(error)); });
    }

    auto initAvCodec() -> bool
    {
        isOpen = false;

        //初始化pFormatCtx结构
        if (!formatCtx->openFilePath(filepath)) {
            return false;
        }

        //获取音视频流数据信息
        if (!formatCtx->findStream()) {
            return false;
        }

        addEvent(new DurationChangedEvent(formatCtx->duration()));
        q_ptr->onPositionChanged(0);

        if (!setBestMediaIndex()) {
            return false;
        }

        if (!audioInfo->isIndexVaild() && !videoInfo->isIndexVaild()) {
            return false;
        }

        isOpen = true;
        // formatCtx->printFileInfo();
        formatCtx->dumpFormat();
        return true;
    }

    auto setBestMediaIndex() -> bool
    {
        audioInfo->resetIndex();
        const auto audioTracks = formatCtx->audioTracks();
        for (const auto &track : qAsConst(audioTracks)) {
            if (!track.selected) {
                continue;
            }
            if (!setMediaIndex(audioInfo, track.index)) {
                audioInfo->resetIndex();
                return false;
            }
        }

        videoInfo->resetIndex();
        const auto videoTracks = formatCtx->vidioTracks();
        for (const auto &track : qAsConst(videoTracks)) {
            if (!track.selected) {
                continue;
            }
            if (!setMediaIndex(videoInfo, track.index)) {
                videoInfo->resetIndex();
                return false;
            }
            setMusicCover(track.image);
        }

        subtitleInfo->resetIndex();
        const auto subtitleTracks = formatCtx->subtitleTracks();
        for (const auto &track : qAsConst(subtitleTracks)) {
            if (!track.selected) {
                continue;
            }
            if (!setMediaIndex(subtitleInfo, track.index)) {
                subtitleInfo->resetIndex();
                return false;
            }
            subtitleDecoder->setVideoResolutionRatio(resolutionRatio());
        }

        const auto attachmentTracks = formatCtx->attachmentTracks();
        for (const auto &track : qAsConst(subtitleTracks)) {
            if (!track.selected) {
                continue;
            }
            setMusicCover(track.image);
        }

        QVector<StreamInfo> tracks = audioTracks;
        tracks.append(videoTracks);
        tracks.append(subtitleTracks);
        addEvent(new TracksChangedEvent(tracks));

        qInfo() << audioInfo->index() << videoInfo->index() << subtitleInfo->index();
        return true;
    }

    void setMusicCover(const QImage &image)
    {
        if (image.isNull()) {
            return;
        }
        for (auto render : videoRenders) {
            render->setImage(image);
        }
    }

    void playVideo()
    {
        Q_ASSERT(isOpen);

        setMediaState(Playing);

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
        Clock::master()->invalidate();

        QScopedPointer<Utils::Speed> speedPtr(new Utils::Speed);
        QElapsedTimer timer;
        timer.start();

        while (runing) {
            PacketPtr packetPtr(new Packet);
            if (!formatCtx->readFrame(packetPtr.data())) {
                break;
            }
            speedPtr->addSize(packetPtr->avPacket()->size);
            if (timer.elapsed() > 1000) {
                addEvent(new CacheSpeedChangedEvent(speedPtr->getSpeed()));
                timer.restart();
            }

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

        setMediaState(Stopped);
    }

    auto setMediaIndex(AVContextInfo *contextInfo, int index) const -> bool
    {
        contextInfo->setIndex(index);
        contextInfo->setStream(formatCtx->stream(index));
        if (!contextInfo->initDecoder(formatCtx->guessFrameRate(index))) {
            return false;
        }
        return contextInfo->openCodec(gpuDecode ? AVContextInfo::GpuType::GpuDecode
                                                : AVContextInfo::GpuType::NotUseGpu);
    }

    void setMediaState(MediaState mediaState_)
    {
        mediaState = mediaState_;
        addEvent(new MediaStateChangedEvent(mediaState));
    }

    [[nodiscard]] QSize resolutionRatio() const
    {
        return videoInfo->isIndexVaild() ? videoInfo->resolutionRatio() : QSize();
    }

    void addEvent(Event *event)
    {
        EventPtr eventPtr(event);
        eventQueue.put(eventPtr);
        while (eventQueue.size() > maxEventCount) {
            eventQueue.take();
        }
        emit q_ptr->eventIncrease();
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
    std::atomic_bool isOpen = true;
    std::atomic_bool runing = true;
    bool gpuDecode = true;
    qint64 position = 0;
    std::atomic<MediaState> mediaState = MediaState::Stopped;

    Utils::ThreadSafeQueue<EventPtr> eventQueue;
    int maxEventCount = 100;

    QVector<VideoRender *> videoRenders = {};
};

Player::Player(QObject *parent)
    : QThread(parent)
    , d_ptr(new PlayerPrivate(this))
{
    av_log_set_level(AV_LOG_INFO);

    buildConnect();
}

Player::~Player()
{
    onStop();
}

auto Player::filePath() const -> QString &
{
    return d_ptr->filepath;
}

void Player::openMedia(const QString &filepath)
{
    onStop();
    d_ptr->filepath = filepath;
    d_ptr->setMediaState(Opening);
    d_ptr->initAvCodec();
    if (!d_ptr->isOpen) {
        d_ptr->setMediaState(Stopped);
        qWarning() << "initAvCode Error";
        return;
    }
    onPlay();
}

void Player::onPlay()
{
    buildConnect(true);
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
}

void Player::onPositionChanged(qint64 position)
{
    auto diff = (position - d_ptr->position) / AV_TIME_BASE;
    if (diff < 1) {
        return;
    }
    d_ptr->position = position;
    d_ptr->addEvent(new PositionChangedEvent(position));
}

void Player::seek(qint64 position)
{
    qInfo() << "Seek To: "
            << QTime::fromMSecsSinceStartOfDay(position / 1000).toString("hh:mm:ss.zzz");
}

void Player::setAudioTrack(const QString &text) // 停止再播放最简单 之后在优化
{
    // auto audioTracks = d_ptr->formatCtx->audioMap();
    // auto list = audioTracks.values();
    // if (!list.contains(text)) {
    //     return;
    // }
    // int index = audioTracks.key(text);
    // int subtitleIndex = d_ptr->subtitleInfo->index();

    // onStop();
    // d_ptr->initAvCodec();
    // if (subtitleIndex >= 0) {
    //     if (!d_ptr->setMediaIndex(d_ptr->subtitleInfo, subtitleIndex)) {
    //         return;
    //     }
    //     emit subTrackChanged(d_ptr->formatCtx->subtitleMap().value(subtitleIndex));
    // }
    // if (!d_ptr->setMediaIndex(d_ptr->audioInfo, index)) {
    //     return;
    // }
    // emit audioTrackChanged(text);
    // seek(d_ptr->position);
    // onPlay();
}

void Player::setSubtitleTrack(const QString &text)
{
    // auto subtitleTracks = d_ptr->formatCtx->subtitleMap();
    // auto list = subtitleTracks.values();
    // if (!list.contains(text)) {
    //     return;
    // }
    // int index = subtitleTracks.key(text);
    // int audioIndex = d_ptr->audioInfo->index();

    // onStop();
    // d_ptr->initAvCodec();
    // if (audioIndex >= 0) {
    //     if (!d_ptr->setMediaIndex(d_ptr->audioInfo, audioIndex)) {
    //         return;
    //     }
    //     emit audioTrackChanged(d_ptr->formatCtx->audioMap().value(audioIndex));
    // }
    // if (!d_ptr->setMediaIndex(d_ptr->subtitleInfo, index)) {
    //     return;
    // }
    // emit subTrackChanged(text);
    // seek(d_ptr->position);
    // onPlay();
}

auto Player::isOpen() -> bool
{
    return d_ptr->isOpen;
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

auto Player::speed() -> double
{
    return 1.0;
}

void Player::pause(bool status)
{
    if (status) {
        d_ptr->setMediaState(MediaState::Pausing);
    } else if (isRunning()) {
        d_ptr->setMediaState(d_ptr->isOpen ? MediaState::Playing : MediaState::Opening);
    } else {
        d_ptr->setMediaState(MediaState::Stopped);
    }
}

void Player::setUseGpuDecode(bool on)
{
    // d_ptr->gpuDecode = on;
    // if (!isRunning()) {
    //     return;
    // }
    // int audioIndex = d_ptr->audioInfo->index();
    // int subtitleIndex = d_ptr->subtitleInfo->index();
    // onStop();
    // d_ptr->initAvCodec();
    // if (audioIndex >= 0) {
    //     if (!d_ptr->setMediaIndex(d_ptr->audioInfo, audioIndex)) {
    //         return;
    //     }
    //     emit audioTrackChanged(d_ptr->formatCtx->audioMap().value(audioIndex));
    // }
    // if (subtitleIndex >= 0) {
    //     if (!d_ptr->setMediaIndex(d_ptr->subtitleInfo, subtitleIndex)) {
    //         return;
    //     }
    //     emit subTrackChanged(d_ptr->formatCtx->subtitleMap().value(subtitleIndex));
    // }
    // seek(d_ptr->position);
    // onPlay();
}

auto Player::isGpuDecode() -> bool
{
    return d_ptr->gpuDecode;
}

auto Player::mediaState() -> MediaState
{
    return d_ptr->mediaState;
}

auto Player::duration() const -> qint64
{
    return d_ptr->formatCtx->duration();
}

auto Player::position() const -> qint64
{
    return d_ptr->position;
}

auto Player::fames() const -> qint64
{
    return d_ptr->videoInfo->isIndexVaild() ? d_ptr->videoInfo->fames() : 0;
}

QSize Player::resolutionRatio() const
{
    return d_ptr->resolutionRatio();
}

auto Player::fps() const -> double
{
    return d_ptr->videoInfo->isIndexVaild() ? d_ptr->videoInfo->fps() : 0;
}

auto Player::audioIndex() const -> int
{
    return d_ptr->audioInfo->index();
}

auto Player::videoIndex() const -> int
{
    return d_ptr->videoInfo->index();
}

auto Player::subtitleIndex() const -> int
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

size_t Player::eventCount() const
{
    return d_ptr->eventQueue.size();
}

EventPtr Player::takeEvent()
{
    return d_ptr->eventQueue.take();
}

void Player::run()
{
    if (!d_ptr->isOpen) {
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

} // namespace Ffmpeg
