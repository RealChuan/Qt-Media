#ifndef MPVPLAYER_HPP
#define MPVPLAYER_HPP

#include "mediainfo.hpp"
#include "mpv_global.h"

#include <QObject>

struct mpv_handle;

namespace Mpv {

class MPV_LIB_EXPORT MpvPlayer : public QObject
{
    Q_OBJECT
public:
    enum GpuApiType {
        Auto,
        Opengl,
        Vulkan,
#ifdef Q_OS_WIN
        D3d11
#endif
    };
    Q_ENUM(GpuApiType)

    explicit MpvPlayer(QObject *parent = nullptr);
    ~MpvPlayer() override;

    void initMpv(QWidget *widget);

    void openMedia(const QString &filePath);

    void play();
    void stop();

    [[nodiscard]] auto filename() const -> QString;
    [[nodiscard]] auto filepath() const -> QString;
    [[nodiscard]] auto filesize() const -> double;

    [[nodiscard]] auto duration() const -> double; // seconds
    [[nodiscard]] auto position() const -> double; // seconds

    [[nodiscard]] auto chapters() const -> Chapters;

    [[nodiscard]] auto videoTracks() const -> TraskInfos;
    [[nodiscard]] auto audioTracks() const -> TraskInfos;
    [[nodiscard]] auto subTracks() const -> TraskInfos;

    void setVideoTrack(int vid);
    void blockVideoTrack();
    void setAudioTrack(int aid);
    void blockAudioTrack();
    void setSubTrack(int sid);
    void blockSubTrack();

    void addAudio(const QStringList &paths);
    void addSub(const QStringList &paths);

    void setPrintToStd(bool print);

    void setCache(bool cache);
    void setCacheSeconds(int seconds);
    [[nodiscard]] auto cacheSpeed() const -> double; // seconds
    void setCacheSpeed(double speed);                // bytes / seconds

    void setUseGpu(bool use);
    void setGpuApi(GpuApiType type);

    void setVolume(int value);
    [[nodiscard]] auto volume() const -> int;
    [[nodiscard]] auto volumeMax() const -> int;

    void seek(qint64 seconds);
    void seekRelative(qint64 seconds);

    void setSpeed(double speed);
    [[nodiscard]] auto speed() const -> double;

    void setSubtitleDelay(double delay); // seconds
    [[nodiscard]] auto subtitleDelay() const -> double;

    void setBrightness(int value);
    [[nodiscard]] auto brightness() const -> int;

    void setContrast(int value);
    [[nodiscard]] auto contrast() const -> int;

    void setSaturation(int value);
    [[nodiscard]] auto saturation() const -> int;

    void setGamma(int value);
    [[nodiscard]] auto gamma() const -> int;

    void setHue(int value);
    [[nodiscard]] auto hue() const -> int;

    void pauseAsync();
    void pauseSync(bool state);
    auto isPaused() -> bool;

    void abortAllAsyncCommands();

    void quit();

    auto mpv_handler() -> mpv_handle *;

signals:
    void fileLoaded();
    void fileFinished();
    void chapterChanged();
    void trackChanged();
    void durationChanged(double duration); // ms
    void positionChanged(double position); // ms
    void mpvLogMessage(const QString &log);
    void pauseStateChanged(bool state);
    void cacheSpeedChanged(int64_t);

private slots:
    void onMpvEvents();

private:
    class MpvPlayerPrivate;
    QScopedPointer<MpvPlayerPrivate> d_ptr;
};

} // namespace Mpv

#endif // MPVPLAYER_HPP
