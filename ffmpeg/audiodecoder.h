#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QThread>

namespace Ffmpeg {

class Packet;
class AVContextInfo;
class AudioDecoderPrivate;
class AudioDecoder : public QThread
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder() override;

    void startDecoder(AVContextInfo *audioInfo);
    void stopDecoder();

    void append(const Packet& packet);

protected:
    void run() override;

private:
    QScopedPointer<AudioDecoderPrivate> d_ptr;
};

}

#endif // AUDIODECODER_H
