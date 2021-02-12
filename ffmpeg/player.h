#ifndef PLAYER_H
#define PLAYER_H

#include <QThread>

#include "ffmepg_global.h"

namespace Ffmpeg {

class PlayerPrivate;
class FFMPEG_EXPORT Player : public QThread
{
    Q_OBJECT
public:
    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    bool isOpen();
    QString lastError() const;

    void pause(bool status = true);

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

protected:
    void run() override;

private:
    void buildConnect();
    bool initAvCode();
    void playVideo();
    void checkSeek();

    QScopedPointer<PlayerPrivate> d_ptr;
};

}

#endif // PLAYER_H
