#ifndef PLAYER_H
#define PLAYER_H

#include <QThread>

#include "ffmepg_global.h"
#include "subtitle.h"

namespace Ffmpeg {

class AVContextInfo;
class VideoOutputWidget;
class PlayerPrivate;
class FFMPEG_EXPORT Player : public QThread
{
    Q_OBJECT
public:
    enum MediaState {
        StoppedState,
        PlayingState,
        PausedState
    };

    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    bool isOpen();
    QString lastError() const;

    void setVolume(qreal volume);

    void setSpeed(double speed);
    double speed();

    void pause(bool status = true);

    MediaState mediaState();

    void setVideoOutputWidget(VideoOutputWidget* widget);

public slots:
    void onSetFilePath(const QString &filepath);
    void onPlay();
    void onStop();
    void onSeek(int timestamp); // s
    void onSetAudioTracks(const QString &text);
    void onSetSubtitleStream(const QString &text);

signals:
    void readyRead(const QImage &image);
    void error(const QString& e);
    void durationChanged(qint64 duration); // s
    void positionChanged(qint64 position); // ms
    void stateChanged(MediaState);
    void audioTracksChanged(const QStringList &tracks);
    void audioTrackChanged(const QString &track);
    void subtitleStreamsChanged(const QStringList &streams);
    void subtitleStreamChanged(const QString &stream);
    void subtitleImages(const QVector<SubtitleImage>&);

protected:
    void run() override;

private:
    void buildConnect(bool state = true);
    bool initAvCode();
    void playVideo();
    void checkSeek();
    void setMediaState(MediaState mediaState);
    bool setMediaIndex(AVContextInfo * contextInfo, int index);

    QScopedPointer<PlayerPrivate> d_ptr;
};

}

#endif // PLAYER_H
