#ifndef PLAYER_H
#define PLAYER_H

#include "ffmepg_global.h"

#include <QThread>

namespace Ffmpeg {

class AVError;
class AVContextInfo;
class VideoRender;

class FFMPEG_EXPORT Player : public QThread
{
    Q_OBJECT
public:
    enum MediaState { StoppedState, PlayingState, PausedState };

    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    void openMedia(const QString &filepath);
    [[nodiscard]] auto filePath() const -> QString &;

    auto isOpen() -> bool;

    void setVolume(qreal volume);

    void setSpeed(double speed);
    auto speed() -> double;

    void pause(bool status = true);

    void setUseGpuDecode(bool on);
    auto isGpuDecode() -> bool;

    void setAudioTrack(const QString &text);
    void setSubtitleTrack(const QString &text);

    auto mediaState() -> MediaState;

    [[nodiscard]] auto duration() const -> qint64; // ms
    [[nodiscard]] auto position() const -> qint64; // ms
    [[nodiscard]] auto fames() const -> qint64;
    [[nodiscard]] auto resolutionRatio() const -> QSize;
    [[nodiscard]] auto fps() const -> double;

    [[nodiscard]] auto audioIndex() const -> int;
    [[nodiscard]] auto videoIndex() const -> int;
    [[nodiscard]] auto subtitleIndex() const -> int;

    void setVideoRenders(QVector<VideoRender *> videoRenders);
    QVector<VideoRender *> videoRenders();

public slots:
    void onPlay();
    void onStop();
    void onSeek(int timestamp); // s

signals:
    void durationChanged(qint64 duration); // ms
    void positionChanged(qint64 position); // ms
    void stateChanged(Ffmpeg::Player::MediaState);
    void audioTracksChanged(const QStringList &tracks);
    void audioTrackChanged(const QString &track);
    void subTracksChanged(const QStringList &streams);
    void subTrackChanged(const QString &stream);
    void error(const Ffmpeg::AVError &avError);

    void playStarted();
    void seekFinished();

    void readSpeedChanged(qint64);

protected:
    void run() override;

private:
    void buildConnect(bool state = true);
    void buildErrorConnect();
    auto initAvCodec() -> bool;
    void playVideo();
    void checkSeek();
    void setMediaState(MediaState mediaState);
    auto setMediaIndex(AVContextInfo *contextInfo, int index) -> bool;
    void calculateSpeed(QElapsedTimer &timer, qint64 &readSize, qint64 addSize);

    class PlayerPrivate;
    QScopedPointer<PlayerPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PLAYER_H
