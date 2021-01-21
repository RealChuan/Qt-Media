#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QThread>

namespace Ffmpeg {

class Packet;
class AVContextInfo;
class VideoDecoderPrivate;
class VideoDecoder : public QThread
{
    Q_OBJECT
public:
    VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    void startDecoder(AVContextInfo *audioInfo);
    void stopDecoder();

    void append(const Packet& packet);

    int size();

signals:
    void readyRead(const QPixmap &pixmap);

protected:
    void run() override;

private:
    QScopedPointer<VideoDecoderPrivate> d_ptr;
};

}

#endif // VIDEODECODER_H
