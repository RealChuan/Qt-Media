#ifndef PLAYER_H
#define PLAYER_H

#include <QThread>

#include "ffmepg_global.h"

namespace Ffmpeg {

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

signals:
    void readyRead(const QImage &image);
    void error(const QString& e);
    void durationChanged(qint64 duration); // s
    void positionChanged(qint64 position); // ms
    void stateChanged(MediaState);

private slots:
    void onReadyRead(const QImage &image);

protected:
    void run() override;

private:
    void buildConnect(bool state = true);
    bool initAvCode();
    void playVideo();
    void checkSeek();
    void setMediaState(MediaState mediaState);

    QScopedPointer<PlayerPrivate> d_ptr;
};

}

#endif // PLAYER_H
