#include "mpvplayer.hpp"
#include "qthelper.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QWidget>

#include <mpv/client.h>

#include <sstream>
#include <stdexcept>

namespace Mpv {

static void wakeup(void *ctx)
{
    // This callback is invoked from any mpv thread (but possibly also
    // recursively from a thread that is calling the mpv API). Just notify
    // the Qt GUI thread to wake up (so that it can process events with
    // mpv_wait_event()), and return as quickly as possible.
    auto *w = static_cast<MpvPlayer *>(ctx);
    QMetaObject::invokeMethod(w, "onMpvEvents", Qt::QueuedConnection);
}

class MpvPlayer::MpvPlayerPrivate
{
public:
    explicit MpvPlayerPrivate(MpvPlayer *parent)
        : owner(parent)
    {}

    ~MpvPlayerPrivate() { destroy(); }

    void init()
    {
        destroy();
        mpv = mpv_create();
        if (mpv == nullptr) {
            throw std::runtime_error("can't create mpv instance");
        }
    }

    void destroy()
    {
        if (mpv != nullptr) {
            mpv_terminate_destroy(mpv);
            mpv = nullptr;
        }
    }

    void handle_mpv_event(mpv_event *event)
    {
        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto *prop = static_cast<mpv_event_property *>(event->data);
            if (strcmp(prop->name, "time-pos") == 0) {
                if (prop->format == MPV_FORMAT_DOUBLE) {
                    position = *static_cast<double *>(prop->data);
                    emit owner->positionChanged(position);
                } else if (prop->format == MPV_FORMAT_NONE) {
                    // The property is unavailable, which probably means playback
                    // was stopped.
                    position = 0;
                    emit owner->positionChanged(position);
                }
            } else if (strcmp(prop->name, "duration") == 0) {
                if (prop->format == MPV_FORMAT_DOUBLE) {
                    emit owner->durationChanged(*static_cast<double *>(prop->data));
                }
            } else if (strcmp(prop->name, "chapter-list") == 0) {
                // Dump the properties as JSON for demo purposes.
                if (prop->format == MPV_FORMAT_NODE) {
                    auto v = mpv::qt::node_to_variant(static_cast<mpv_node *>(prop->data));
                    // Abuse JSON support for easily printing the mpv_node contents.
                    auto d = QJsonDocument::fromVariant(v);
                    emit owner->mpvLogMessage("Change property " + QString(prop->name) + ":\n");
                    emit owner->mpvLogMessage(d.toJson());

                    chapters.clear();
                    auto array = d.array();
                    for (auto iter = array.cbegin(); iter != array.cend(); iter++) {
                        auto obj = (*iter).toObject();
                        chapters.append(Chapter(obj));
                    }
                    emit owner->chapterChanged();
                }
            } else if (strcmp(prop->name, "track-list") == 0) {
                // Dump the properties as JSON for demo purposes.
                if (prop->format == MPV_FORMAT_NODE) {
                    auto v = mpv::qt::node_to_variant(static_cast<mpv_node *>(prop->data));
                    // Abuse JSON support for easily printing the mpv_node contents.
                    auto d = QJsonDocument::fromVariant(v);
                    emit owner->mpvLogMessage("Change property " + QString(prop->name) + ":\n");
                    emit owner->mpvLogMessage(d.toJson());

                    videoTracks.clear();
                    audioTracks.clear();
                    subTracks.clear();
                    auto array = d.array();
                    for (auto iter = array.cbegin(); iter != array.cend(); iter++) {
                        auto obj = (*iter).toObject();
                        auto traskInfo = TraskInfo(obj);
                        if (obj.value("type") == "audio") {
                            audioTracks.append(traskInfo);
                        } else if (obj.value("type") == "video") {
                            videoTracks.append(traskInfo);
                        } else if (obj.value("type") == "sub") {
                            subTracks.append(traskInfo);
                        }
                    }
                    emit owner->trackChanged();
                }
            } else if (strcmp(prop->name, "cache-speed") == 0) {
                if (prop->format == MPV_FORMAT_INT64) {
                    cache_speed = *static_cast<int64_t *>(prop->data);
                    emit owner->cacheSpeedChanged(cache_speed);
                }
            }
            break;
        }
        case MPV_EVENT_VIDEO_RECONFIG: {
            // Retrieve the new video size.
            int64_t w, h;
            if (mpv_get_property(mpv, "dwidth", MPV_FORMAT_INT64, &w) >= 0
                && mpv_get_property(mpv, "dheight", MPV_FORMAT_INT64, &h) >= 0 && w > 0 && h > 0) {
                // Note that the MPV_EVENT_VIDEO_RECONFIG event doesn't necessarily
                // imply a resize, and you should check yourself if the video
                // dimensions really changed.
                // mpv itself will scale/letter box the video to the container size
                // if the video doesn't fit.
                std::stringstream ss;
                ss << "Reconfig: " << w << " " << h << "\n";
                emit owner->mpvLogMessage(QString::fromStdString(ss.str()));
            }
            break;
        }
        case MPV_EVENT_LOG_MESSAGE: {
            auto *msg = static_cast<struct mpv_event_log_message *>(event->data);
            std::stringstream ss;
            ss << "[" << msg->prefix << "] " << msg->level << ": " << msg->text;
            emit owner->mpvLogMessage(QString::fromStdString(ss.str()));
            break;
        }
        case MPV_EVENT_SHUTDOWN: destroy(); break;
        case MPV_EVENT_FILE_LOADED: emit owner->fileLoaded(); break;
        case MPV_EVENT_END_FILE: {
            auto *prop = static_cast<mpv_event_end_file *>(event->data);
            if (prop->reason == MPV_END_FILE_REASON_EOF) {
                emit owner->fileFinished();
            }
        } break;
        default:
            break;
            // Ignore uninteresting or unknown events.
        }
    }

    MpvPlayer *owner;

    struct mpv_handle *mpv = nullptr;
    TraskInfos videoTracks;
    TraskInfos audioTracks;
    TraskInfos subTracks;
    Chapters chapters;
    double position = 0;
    int64_t cache_speed = 0;
};

MpvPlayer::MpvPlayer(QObject *parent)
    : QObject{parent}
    , d_ptr(new MpvPlayerPrivate(this))
{
    qInfo() << "mpv client api version: " << mpv_client_api_version();
}

MpvPlayer::~MpvPlayer() = default;

void MpvPlayer::openMedia(const QString &filePath)
{
    if (d_ptr->mpv == nullptr) {
        return;
    }
    const QByteArray c_filename = filePath.toUtf8();
    const char *args[] = {"loadfile", c_filename.data(), nullptr};
    mpv_command_async(d_ptr->mpv, 0, args);
}

void MpvPlayer::play()
{
    mpv::qt::set_property_async(d_ptr->mpv, "pause", false);
}

void MpvPlayer::stop()
{
    mpv::qt::set_property_async(d_ptr->mpv, "pause", true);
    mpv::qt::set_property_async(d_ptr->mpv, "time-pos", 0);
}

auto MpvPlayer::filename() const -> QString
{
    return mpv::qt::get_property(d_ptr->mpv, "filename").toString();
}

auto MpvPlayer::filepath() const -> QString
{
    return mpv::qt::get_property(d_ptr->mpv, "path").toString();
}

auto MpvPlayer::filesize() const -> double
{
    return mpv::qt::get_property(d_ptr->mpv, "file-size").toDouble();
}

auto MpvPlayer::duration() const -> double
{
    return mpv::qt::get_property(d_ptr->mpv, "duration").toDouble();
}

auto MpvPlayer::position() const -> double
{
    // time-pos
    return mpv::qt::get_property(d_ptr->mpv, "playback-time").toDouble();
}

auto MpvPlayer::chapters() const -> Chapters
{
    return d_ptr->chapters;
}

auto MpvPlayer::videoTracks() const -> TraskInfos
{
    return d_ptr->videoTracks;
}

auto MpvPlayer::audioTracks() const -> TraskInfos
{
    return d_ptr->audioTracks;
}

auto MpvPlayer::subTracks() const -> TraskInfos
{
    return d_ptr->subTracks;
}

void MpvPlayer::start(qint64 seconds)
{
    mpv::qt::set_property(d_ptr->mpv, "start", QString::number(seconds));
}

void MpvPlayer::setVid(const QVariant &vid)
{
    qInfo() << "vid: " << vid;
    mpv::qt::set_property_async(d_ptr->mpv, "vid", vid);
}

QVariant MpvPlayer::vid() const
{
    return mpv::qt::get_property(d_ptr->mpv, "vid");
}

void MpvPlayer::setAid(const QVariant &aid)
{
    qInfo() << "aid: " << aid;
    mpv::qt::set_property_async(d_ptr->mpv, "aid", aid);
}

QVariant MpvPlayer::aid() const
{
    return mpv::qt::get_property(d_ptr->mpv, "aid");
}

void MpvPlayer::setSid(const QVariant &sid)
{
    qInfo() << "sid: " << sid;
    mpv::qt::set_property_async(d_ptr->mpv, "sid", sid);
}

QVariant MpvPlayer::sid() const
{
    return mpv::qt::get_property(d_ptr->mpv, "sid");
}

void MpvPlayer::addAudio(const QStringList &paths)
{
    for (const auto &path : std::as_const(paths)) {
        mpv::qt::command_async(d_ptr->mpv, QVariantList() << "audio-add" << path);
    }
}

void MpvPlayer::addSub(const QStringList &paths)
{
    for (const auto &path : std::as_const(paths)) {
        mpv::qt::command_async(d_ptr->mpv, QVariantList() << "sub-add" << path);
    }
}

void MpvPlayer::setPrintToStd(bool print)
{
    mpv_set_option_string(d_ptr->mpv, "terminal", print ? "yes" : "no");
}

void MpvPlayer::setCache(bool cache)
{
    mpv::qt::set_property_async(d_ptr->mpv, "cahce", cache ? "auto" : "no");
}

void MpvPlayer::setCacheSeconds(int seconds)
{
    mpv::qt::set_property_async(d_ptr->mpv, "cache-secs", seconds);
}

auto MpvPlayer::cacheSpeed() const -> double
{
    return mpv::qt::get_property(d_ptr->mpv, "cache-speed").toDouble();
}

void MpvPlayer::setCacheSpeed(double speed)
{
    qInfo() << "cache-speed: " << speed;
    mpv::qt::set_property_async(d_ptr->mpv, "cache-speed", speed);
}

QStringList MpvPlayer::hwdecs() const
{
    static QStringList list{"no",
                            "auto-safe",
                            "auto",
                            "yes",
                            "auto-copy",
                            "d3d11va",
                            "d3d11va-copy",
                            "videotoolbox",
                            "videotoolbox-copy",
                            "vaapi",
                            "vaapi-copy",
                            "nvdec",
                            "nvdec-copy",
                            "drm",
                            "drm-copy",
                            "vulkan",
                            "vulkan-copy"};
    // list.append(QStringList{"dxva2",
    //                         "dxva2-copy",
    //                         "vdpau",
    //                         "vdpau-copy",
    //                         "mediacodec",
    //                         "mediacodec-copy",
    //                         "cuda",
    //                         "cuda-copy",
    //                         "crystalhd",
    //                         "rkmpp"});
    return list;
}

void MpvPlayer::setHwdec(const QString &hwdec)
{
    qInfo() << "hwdec: " << hwdec;
    mpv::qt::set_property_async(d_ptr->mpv, "hwdec", hwdec);
}

QString MpvPlayer::hwdec() const
{
    return mpv::qt::get_property(d_ptr->mpv, "hwdec").toString();
}

QStringList MpvPlayer::gpuApis() const
{
    static QStringList list{"auto", "opengl", "vulkan", "d3d11"};
    return list;
}

void MpvPlayer::setGpuApi(const QString &gpuApi)
{
    qInfo() << "gpu-api: " << gpuApi;
    mpv::qt::set_property_async(d_ptr->mpv, "gpu-api", gpuApi);
}

QString MpvPlayer::gpuApi() const
{
    return mpv::qt::get_property(d_ptr->mpv, "gpu-api").toString();
}

void MpvPlayer::setVolume(int value)
{
    qInfo() << "volume: " << value;
    mpv::qt::set_property_async(d_ptr->mpv, "volume", value);
}

auto MpvPlayer::volume() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "volume").toInt();
}

void MpvPlayer::seek(qint64 percent)
{
    qInfo() << "seek: " << percent;
    mpv::qt::command_async(d_ptr->mpv, QVariantList() << "seek" << percent << "absolute");
}

void MpvPlayer::seekRelative(qint64 seconds)
{
    qInfo() << "seekRelative: " << seconds;
    mpv::qt::command_async(d_ptr->mpv, QVariantList() << "seek" << seconds << "relative");
}

void MpvPlayer::setSpeed(double speed)
{
    qInfo() << "speed: " << speed;
    mpv::qt::set_property_async(d_ptr->mpv, "speed", speed);
}

auto MpvPlayer::speed() const -> double
{
    return mpv::qt::get_property(d_ptr->mpv, "speed").toDouble();
}

void MpvPlayer::setSubtitleDelay(double delay)
{
    qInfo() << "sub-delay: " << delay;
    mpv::qt::set_property_async(d_ptr->mpv, "sub-delay", delay);
}

auto MpvPlayer::subtitleDelay() const -> double
{
    return mpv::qt::get_property(d_ptr->mpv, "sub-delay").toDouble();
}

void MpvPlayer::setBrightness(int value)
{
    qInfo() << "brightness: " << value;
    Q_ASSERT(value >= -100 && value <= 100);
    mpv::qt::set_property_async(d_ptr->mpv, "brightness", value);
}

auto MpvPlayer::brightness() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "brightness").toInt();
}

void MpvPlayer::setContrast(int value)
{
    qInfo() << "contrast: " << value;
    Q_ASSERT(value >= -100 && value <= 100);
    mpv::qt::set_property_async(d_ptr->mpv, "contrast", value);
}

auto MpvPlayer::contrast() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "contrast").toInt();
}

void MpvPlayer::setSaturation(int value)
{
    qInfo() << "saturation: " << value;
    Q_ASSERT(value >= -100 && value <= 100);
    mpv::qt::set_property_async(d_ptr->mpv, "saturation", value);
}

auto MpvPlayer::saturation() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "saturation").toInt();
}

void MpvPlayer::setGamma(int value)
{
    qInfo() << "gamma: " << value;
    Q_ASSERT(value >= -100 && value <= 100);
    mpv::qt::set_property_async(d_ptr->mpv, "gamma", value);
}

auto MpvPlayer::gamma() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "gamma").toInt();
}

void MpvPlayer::setHue(int value)
{
    qInfo() << "hue: " << value;
    Q_ASSERT(value >= -100 && value <= 100);
    mpv::qt::set_property_async(d_ptr->mpv, "hue", value);
}

auto MpvPlayer::hue() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "hue").toInt();
}

auto MpvPlayer::toneMappings() const -> QStringList
{
    static QStringList list{"auto",
                            "clip",
                            "mobius",
                            "reinhard",
                            "hable",
                            "bt.2390",
                            "gamma",
                            "linear",
                            "spline",
                            "bt.2446a",
                            "st2094-40",
                            "st2094-10"};
    return list;
}

void MpvPlayer::setToneMapping(const QString &toneMapping)
{
    qInfo() << "tone-mapping: " << toneMapping;
    mpv::qt::set_property_async(d_ptr->mpv, "tone-mapping", toneMapping);
}

auto MpvPlayer::toneMapping() const -> QString
{
    return mpv::qt::get_property(d_ptr->mpv, "tone-mapping").toString();
}

QStringList MpvPlayer::targetPrimaries() const
{
    static QStringList list{"auto",
                            "bt.470m",
                            "bt.601-525",
                            "bt.601-625",
                            "bt.709",
                            "bt.2020",
                            "apple",
                            "adobe",
                            "prophoto",
                            "cie1931",
                            "dci-p3",
                            "v-gamut",
                            "s-gamut"};
    return list;
}

void MpvPlayer::setTargetPrimaries(const QString &targetPrimaries)
{
    qInfo() << "target-prim: " << targetPrimaries;
    mpv::qt::set_property_async(d_ptr->mpv, "target-prim", targetPrimaries);
}

QString MpvPlayer::targetPrimariesName() const
{
    return mpv::qt::get_property(d_ptr->mpv, "target-prim").toString();
}

void MpvPlayer::setLogFile(const QString &logFile)
{
    qInfo() << "log-file: " << logFile;
    mpv::qt::set_property_async(d_ptr->mpv, "log-file", logFile);
}

QString MpvPlayer::logFile() const
{
    return mpv::qt::get_property(d_ptr->mpv, "log-file").toString();
}

void MpvPlayer::setConfigDir(const QString &configDir)
{
    qInfo() << "config-dir: " << configDir;
    mpv::qt::set_property_async(d_ptr->mpv, "config-dir", configDir);
}

QString MpvPlayer::configDir() const
{
    return mpv::qt::get_property(d_ptr->mpv, "config-dir").toString();
}

void MpvPlayer::pauseAsync()
{
    auto state = !isPaused();
    qInfo() << "pause: " << state;
    mpv::qt::set_property_async(d_ptr->mpv, "pause", state);
    emit pauseStateChanged(state);
}

void MpvPlayer::pauseSync(bool state)
{
    qInfo() << "pause: " << state;
    mpv::qt::set_property(d_ptr->mpv, "pause", state);
    emit pauseStateChanged(state);
}

auto MpvPlayer::isPaused() -> bool
{
    return mpv::qt::get_property(d_ptr->mpv, "pause").toBool();
}

auto MpvPlayer::volumeMax() const -> int
{
    return mpv::qt::get_property(d_ptr->mpv, "volume-max").toInt();
}

void MpvPlayer::abortAllAsyncCommands()
{
    mpv::qt::command_abort_async(d_ptr->mpv);
}

void MpvPlayer::quit()
{
    mpv::qt::set_property(d_ptr->mpv, "quit", true);
}

auto MpvPlayer::mpv_handler() -> mpv_handle *
{
    return d_ptr->mpv;
}

void MpvPlayer::onMpvEvents()
{
    // Process all events, until the event queue is empty.
    while (d_ptr->mpv != nullptr) {
        auto *event = mpv_wait_event(d_ptr->mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        d_ptr->handle_mpv_event(event);
    }
}

void MpvPlayer::initMpv(QWidget *widget)
{
    d_ptr->init();
    if (widget != nullptr) {
        auto raw_wid = widget->winId();
#ifdef _WIN32
        // Truncate to 32-bit, as all Windows handles are. This also ensures
        // it doesn't go negative.
        int64_t wid = static_cast<uint32_t>(raw_wid);
#else
        int64_t wid = raw_wid;
#endif
        mpv_set_property(d_ptr->mpv, "wid", MPV_FORMAT_INT64, &wid);
    } else {
        mpv_set_option_string(d_ptr->mpv, "vo", "libmpv");
    }
    // Enable default bindings, because we're lazy. Normally, a player using
    // mpv as backend would implement its own key bindings.
    mpv_set_property_string(d_ptr->mpv, "input-default-bindings", "yes");

    // Enable keyboard input on the X11 window. For the messy details, see
    // --input-vo-keyboard on the manpage.
    mpv_set_property_string(d_ptr->mpv, "input-vo-keyboard", "yes");

    // Let us receive property change events with MPV_EVENT_PROPERTY_CHANGE if
    // this property changes.
    mpv_observe_property(d_ptr->mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(d_ptr->mpv, 0, "cache-speed", MPV_FORMAT_INT64);

    mpv_observe_property(d_ptr->mpv, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(d_ptr->mpv, 0, "chapter-list", MPV_FORMAT_NODE);

    // Request log messages with level "info" or higher.
    // They are received as MPV_EVENT_LOG_MESSAGE.
    mpv_request_log_messages(d_ptr->mpv, "info");

    mpv::qt::set_property(d_ptr->mpv, "volume-max", 200);

    mpv::qt::set_property(d_ptr->mpv, "save-position-on-quit", true);

    // From this point on, the wakeup function will be called. The callback
    // can come from any thread, so we use the QueuedConnection mechanism to
    // relay the wakeup in a thread-safe way.
    mpv_set_wakeup_callback(d_ptr->mpv, wakeup, this);

    mpv_set_option_string(d_ptr->mpv, "msg-level", "all=v");

    if (mpv_initialize(d_ptr->mpv) < 0) {
        throw std::runtime_error("mpv failed to initialize");
    }
}

} // namespace Mpv
