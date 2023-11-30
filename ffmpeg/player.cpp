#include "player.h"
#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "clock.hpp"
#include "codeccontext.h"
#include "formatcontext.h"
#include "mediainfo.hpp"
#include "packet.h"
#include "subtitledecoder.h"
#include "videodecoder.h"

#include <event/errorevent.hpp>
#include <event/seekevent.hpp>
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
                         [this](const AVError &error) {
                             addPropertyChangeEvent(new ErrorEvent(error));
                         });
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
        addPropertyChangeEvent(new DurationEvent(formatCtx->duration()));
        q_ptr->onPositionChanged(0);

        if (!setBestMediaIndex()) {
            return false;
        }
        if (!audioInfo->isIndexVaild() && !videoInfo->isIndexVaild()) {
            return false;
        }
        isOpen = true;
        formatCtx->dumpFormat();
        return true;
    }

    auto setBestMediaIndex() -> bool
    {
        audioInfo->resetIndex();
        auto audioTracks = formatCtx->audioTracks();
        for (auto &track : audioTracks) {
            if (meidaIndex.audioindex >= 0) { // 如果手动指定了音频索引
                track.selected = track.index == meidaIndex.audioindex;
            }
            if (!track.selected) {
                continue;
            }
            if (!setMediaIndex(audioInfo, track.index)) {
                audioInfo->resetIndex();
                return false;
            }
        }

        videoInfo->resetIndex();
        auto videoTracks = formatCtx->vidioTracks();
        for (auto &track : videoTracks) {
            if (meidaIndex.videoindex >= 0) { // 如果手动指定了视频索引
                track.selected = track.index == meidaIndex.videoindex;
            }
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
        auto subtitleTracks = formatCtx->subtitleTracks();
        for (auto &track : subtitleTracks) {
            if (meidaIndex.subtitleindex >= 0) { // 如果手动指定了字幕索引
                track.selected = track.index == meidaIndex.subtitleindex;
            }
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
        for (const auto &track : std::as_const(subtitleTracks)) {
            if (!track.selected) {
                continue;
            }
            setMusicCover(track.image);
        }

        QVector<StreamInfo> tracks = audioTracks;
        tracks.append(videoTracks);
        tracks.append(subtitleTracks);
        addPropertyChangeEvent(new MediaTrackEvent(tracks));

        qInfo() << audioInfo->index() << videoInfo->index() << subtitleInfo->index();
        return true;
    }

    void setMusicCover(const QImage &image)
    {
        if (image.isNull()) {
            return;
        }
        for (auto *render : videoRenders) {
            render->setImage(image);
        }
    }

    void startDecoder()
    {
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

        speedPtr.reset(new Utils::Speed);
        speedTimer.restart();
    }

    void stopDecoder()
    {
        speedTimer.invalidate();
        speedPtr.reset();

        videoDecoder->stopDecoder();
        subtitleDecoder->stopDecoder();
        audioDecoder->stopDecoder();
    }

    void addSpeedChangeEvent(int size)
    {
        speedPtr->addSize(size);
        if (speedTimer.hasExpired(1000)) {
            addPropertyChangeEvent(new CacheSpeedEvent(speedPtr->getSpeed()));
            speedTimer.restart();
        }
    }

    void playVideo()
    {
        Q_ASSERT(isOpen);
        setMediaState(Playing);
        startDecoder();

        while (runing) {
            processEvent();

            PacketPtr packetPtr(new Packet);
            if (!formatCtx->readFrame(packetPtr.data())) {
                break;
            }
            addSpeedChangeEvent(packetPtr->avPacket()->size);

            auto stream_index = packetPtr->streamIndex();
            if (!formatCtx->checkPktPlayRange(packetPtr.data())) {
            } else if (stream_index == audioInfo->index()) { // 如果是音频数据
                audioDecoder->append(packetPtr);
            } else if (stream_index == videoInfo->index()
                       && ((videoInfo->stream()->disposition & AV_DISPOSITION_ATTACHED_PIC)
                           == 0)) { // 如果是视频数据
                videoDecoder->append(packetPtr);
            } else if (stream_index == subtitleInfo->index()) { // 如果是字幕数据
                subtitleDecoder->append(packetPtr);
            }
        }
        while (runing && (videoDecoder->size() > 0 || audioDecoder->size() > 0)) {
            msleep(s_waitQueueEmptyMilliseconds);
        }
        stopDecoder();
        setMediaState(Stopped);
        qInfo() << "play finish";
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
        addPropertyChangeEvent(new MediaStateEvent(mediaState));
    }

    [[nodiscard]] auto resolutionRatio() const -> QSize
    {
        return videoInfo->isIndexVaild() ? videoInfo->resolutionRatio() : QSize();
    }

    void wakePause()
    {
        paused.store(false);
        waitCondition.wakeAll();
    }

    void addPropertyChangeEvent(PropertyChangeEvent *event)
    {
        propertyChangeEventQueue.append(PropertyChangeEventPtr(event));
        while (propertyChangeEventQueue.size() > maxPropertyEventQueueSize.load()) {
            propertyChangeEventQueue.take();
        }
        emit q_ptr->eventIncrease();
    }

    void addEvent(const EventPtr &eventPtr)
    {
        eventQueue.append(eventPtr);
        while (eventQueue.size() > maxEventQueueSize.load()) {
            eventQueue.take();
        }
        if (!paused.load()) {
            return;
        }
        switch (eventPtr->type()) {
        case Event::EventType::Pause: break;
        default: {
            eventQueue.append(EventPtr(new PauseEvent(true)));
        } break;
        }
        wakePause();
    }

    void processEvent()
    {
        while (!eventQueue.empty()) {
            auto eventPtr = eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: processPauseEvent(eventPtr); break;
            case Event::EventType::Seek: processSeekEvent(eventPtr); break;
            case Event::EventType::SeekRelative: processSeekRelativeEvent(eventPtr); break;
            default: break;
            }
        }
    }

    void processEvent(const EventPtr &eventPtr)
    {
        switch (eventPtr->type()) {
        case Event::EventType::OpenMedia:
            processCloseMediaEvent();
            processOpenMediaEvent(eventPtr);
            break;
        case Event::EventType::CloseMedia: processCloseMediaEvent(); break;
        case Event::EventType::AudioTarck:
        case Event::EventType::VideoTrack:
        case Event::EventType::SubtitleTrack: processSelectedMediaTrackEvent(eventPtr); break;
        case Event::EventType::Volume: processVolumeEvent(eventPtr); break;
        case Event::EventType::Speed: processSpeedEvent(eventPtr); break;
        case Event::EventType::Gpu: processGpuEvent(eventPtr); break;
        default: break;
        }
    }

    void processPauseEvent(const EventPtr &eventPtr)
    {
        auto *pauseEvent = dynamic_cast<PauseEvent *>(eventPtr.data());
        if (pauseEvent->paused()) {
            setMediaState(MediaState::Pausing);
            addPropertyChangeEvent(new CacheSpeedEvent(0));
        } else if (q_ptr->isRunning()) {
            setMediaState(isOpen ? MediaState::Playing : MediaState::Opening);
        } else {
            setMediaState(MediaState::Stopped);
        }
        audioDecoder->addEvent(eventPtr);
        videoDecoder->addEvent(eventPtr);
        subtitleDecoder->addEvent(eventPtr);
        paused.store(pauseEvent->paused());
        if (paused.load()) {
            QMutexLocker locker(&mutex);
            waitCondition.wait(&mutex);
        } else {
            Clock::master()->invalidate();
        }
    }

    void processSeekEvent(const EventPtr &eventPtr)
    {
        QElapsedTimer timer;
        timer.start();
        q_ptr->blockSignals(true);
        Clock::globalSerialRef();
        int count = 0;
        if (audioInfo->isIndexVaild()) {
            count++;
        }
        if (videoInfo->isIndexVaild()) {
            count++;
        }
        if (subtitleInfo->isIndexVaild()) {
            count++;
        }
        auto *seekEvent = dynamic_cast<SeekEvent *>(eventPtr.data());
        seekEvent->setWaitCountdown(count);
        audioDecoder->addEvent(eventPtr);
        videoDecoder->addEvent(eventPtr);
        subtitleDecoder->addEvent(eventPtr);
        auto position = seekEvent->position();
        seekEvent->wait();

        formatCtx->seek(position, position < this->position);
        if (audioInfo->isIndexVaild()) {
            audioInfo->codecCtx()->flush();
        }
        if (videoInfo->isIndexVaild()) {
            videoInfo->codecCtx()->flush();
        }
        if (subtitleInfo->isIndexVaild()) {
            subtitleInfo->codecCtx()->flush();
        }
        q_ptr->blockSignals(false);
        this->position = position;
        Clock::master()->invalidate();
        qInfo() << "Seek To: "
                << QTime::fromMSecsSinceStartOfDay(position / 1000).toString("hh:mm:ss.zzz")
                << "Seeked elapsed: " << timer.elapsed() << "ms";

        addPropertyChangeEvent(new SeekChangedEvent(position));
    }

    void processSeekRelativeEvent(const EventPtr &eventPtr)
    {
        auto *seekRelativeEvent = dynamic_cast<SeekRelativeEvent *>(eventPtr.data());
        auto relativePosition = seekRelativeEvent->relativePosition();
        auto position = this->position + relativePosition * AV_TIME_BASE;
        if (position < 0) {
            position = 0;
        } else if (position > q_ptr->duration()) {
            position = q_ptr->duration();
        }
        eventQueue.insertHead(EventPtr(new SeekEvent(position)));
    }

    void processGpuEvent(const EventPtr &eventPtr)
    {
        auto *gpuEvent = dynamic_cast<GpuEvent *>(eventPtr.data());
        gpuDecode = gpuEvent->use();
        if (filepath.isEmpty()) {
            return;
        }

        meidaIndex.resetIndex();
        meidaIndex.audioindex = audioInfo->index();
        meidaIndex.videoindex = videoInfo->index();
        meidaIndex.subtitleindex = subtitleInfo->index();

        auto position = this->position - 5 * AV_TIME_BASE;
        if (position < 0) {
            position = 0;
        }

        q_ptr->addEvent(EventPtr(new CloseMediaEvent));
        q_ptr->addEvent(EventPtr(new OpenMediaEvent(filepath)));
        eventQueue.insertHead(EventPtr(new SeekEvent(position)));
    }

    void processSelectedMediaTrackEvent(const EventPtr &eventPtr)
    {
        if (filepath.isEmpty()) {
            return;
        }
        meidaIndex.resetIndex();
        meidaIndex.audioindex = audioInfo->index();
        meidaIndex.videoindex = videoInfo->index();
        meidaIndex.subtitleindex = subtitleInfo->index();

        auto position = this->position - 5 * AV_TIME_BASE;
        if (position < 0) {
            position = 0;
        }

        auto *selectedMediaTrackEvent = dynamic_cast<SelectedMediaTrackEvent *>(eventPtr.data());
        switch (selectedMediaTrackEvent->type()) {
        case Event::EventType::AudioTarck:
            meidaIndex.audioindex = selectedMediaTrackEvent->index();
            break;
        case Event::EventType::VideoTrack:
            meidaIndex.videoindex = selectedMediaTrackEvent->index();
            break;
        case Event::EventType::SubtitleTrack:
            meidaIndex.subtitleindex = selectedMediaTrackEvent->index();
            break;
        default: break;
        }

        q_ptr->addEvent(EventPtr(new CloseMediaEvent));
        q_ptr->addEvent(EventPtr(new OpenMediaEvent(filepath)));
        eventQueue.insertHead(EventPtr(new SeekEvent(position)));
    }

    void processOpenMediaEvent(const EventPtr &eventPtr)
    {
        auto *openMediaEvent = dynamic_cast<OpenMediaEvent *>(eventPtr.data());
        filepath = openMediaEvent->filepath();
        q_ptr->onPlay();
    }

    void processCloseMediaEvent()
    {
        q_ptr->buildConnect(false);
        runing.store(false);
        wakePause();
        if (q_ptr->isRunning()) {
            q_ptr->quit();
        }
        while (q_ptr->isRunning()) {
            qApp->processEvents();
        }
        for (auto *render : videoRenders) {
            render->resetAllFrame();
        }
        formatCtx->close();
    }

    void processSpeedEvent(const EventPtr &eventPtr)
    {
        auto *speedEvent = dynamic_cast<SpeedEvent *>(eventPtr.data());
        Clock::setSpeed(speedEvent->speed());
    }

    void processVolumeEvent(const EventPtr &eventPtr)
    {
        auto *volumeEvent = dynamic_cast<VolumeEvent *>(eventPtr.data());
        audioDecoder->setVolume(volumeEvent->volume());
    }

    Player *q_ptr;

    FormatContext *formatCtx;
    AVContextInfo *audioInfo;
    AVContextInfo *videoInfo;
    AVContextInfo *subtitleInfo;

    AudioDecoder *audioDecoder;
    VideoDecoder *videoDecoder;
    SubtitleDecoder *subtitleDecoder;

    MediaIndex meidaIndex;

    QString filepath;
    std::atomic_bool isOpen = true;
    std::atomic_bool runing = true;
    bool gpuDecode = true;
    qint64 position = 0;
    std::atomic<MediaState> mediaState = MediaState::Stopped;

    std::atomic_bool paused = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    Utils::ThreadSafeQueue<PropertyChangeEventPtr> propertyChangeEventQueue;
    std::atomic<size_t> maxPropertyEventQueueSize = 100;
    Utils::ThreadSafeQueue<EventPtr> eventQueue;
    std::atomic<size_t> maxEventQueueSize = 100;

    QScopedPointer<Utils::Speed> speedPtr;
    QElapsedTimer speedTimer;

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
    addEvent(EventPtr(new CloseMediaEvent));
}

auto Player::filePath() const -> QString &
{
    return d_ptr->filepath;
}

void Player::onPlay()
{
    buildConnect(true);
    d_ptr->runing = true;
    start();
}

void Player::onPositionChanged(qint64 position)
{
    auto diff = (position - d_ptr->position) / AV_TIME_BASE;
    if (qAbs(diff) < 1) {
        return;
    }
    d_ptr->position = position;
    d_ptr->addPropertyChangeEvent(new PositionEvent(position));
}

auto Player::isOpen() -> bool
{
    return d_ptr->isOpen;
}

auto Player::speed() -> double
{
    return Clock::speed();
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

auto Player::resolutionRatio() const -> QSize
{
    return d_ptr->resolutionRatio();
}

auto Player::fps() const -> double
{
    return d_ptr->videoInfo->isIndexVaild() ? d_ptr->videoInfo->fps() : 0;
}

auto Player::mediaInfo() -> MediaInfo
{
    return d_ptr->formatCtx->mediaInfo();
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

void Player::setVideoRenders(const QVector<VideoRender *> &videoRenders)
{
    d_ptr->videoRenders = videoRenders;
    d_ptr->videoDecoder->setVideoRenders(videoRenders);
    d_ptr->subtitleDecoder->setVideoRenders(videoRenders);
}

auto Player::videoRenders() -> QVector<VideoRender *>
{
    return d_ptr->videoRenders;
}

void Player::setPropertyEventQueueMaxSize(size_t size)
{
    d_ptr->maxPropertyEventQueueSize.store(size);
}

auto Player::propertEventyQueueMaxSize() const -> size_t
{
    return d_ptr->maxPropertyEventQueueSize.load();
}

auto Player::propertyChangeEventSize() const -> size_t
{
    return d_ptr->propertyChangeEventQueue.size();
}

auto Player::takePropertyChangeEvent() -> PropertyChangeEventPtr
{
    return d_ptr->propertyChangeEventQueue.take();
}

void Player::setEventQueueMaxSize(size_t size)
{
    d_ptr->maxEventQueueSize.store(size);
}

auto Player::eventQueueMaxSize() const -> size_t
{
    return d_ptr->maxEventQueueSize.load();
}

[[nodiscard]] auto Player::eventSize() const -> size_t
{
    return d_ptr->eventQueue.size();
}

auto Player::addEvent(const EventPtr &eventPtr) -> bool
{
    if (eventPtr->type() < Event::EventType::Pause) {
        QMetaObject::invokeMethod(
            this,
            [=] { d_ptr->processEvent(eventPtr); },
            QThread::currentThread() == this->thread() ? Qt::DirectConnection
                                                       : Qt::QueuedConnection);
        return true;
    }
    if (!isRunning()) {
        return false;
    }
    d_ptr->addEvent(eventPtr);
    return true;
}

void Player::run()
{
    d_ptr->setMediaState(Opening);
    if (!d_ptr->initAvCodec()) {
        d_ptr->setMediaState(Stopped);
        qWarning() << "initAvCode Error";
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
