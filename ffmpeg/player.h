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

    void seek(qint64 position); // microsecond

    void setVolume(qreal volume);

    void setSpeed(double speed);
    auto speed() -> double;

    void pause(bool status = true);

    void setUseGpuDecode(bool on);
    auto isGpuDecode() -> bool;

    void setAudioTrack(const QString &text);
    void setSubtitleTrack(const QString &text);

    auto mediaState() -> MediaState;

    [[nodiscard]] auto duration() const -> qint64; // microsecond
    [[nodiscard]] auto position() const -> qint64; // microsecond
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

private slots:
    void onPositionChanged(qint64 position);

signals:
    void durationChanged(qint64 duration); // microsecond
    void positionChanged(qint64 position); // microsecond
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

    class PlayerPrivate;
    QScopedPointer<PlayerPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PLAYER_H
