#ifndef SUBTITLEDECODER_H
#define SUBTITLEDECODER_H

#include "decoder.h"
#include "packet.h"
#include "subtitle.h"

namespace Ffmpeg {

class SubtitleDecoderPrivate;
class SubtitleDecoder : public Decoder<Packet>
{
    Q_OBJECT
public:
    SubtitleDecoder(QObject *parent = nullptr);
    ~SubtitleDecoder();

    void stopDecoder() override;

    void pause(bool state) override;

signals:
    void subtitleImages(const QVector<Ffmpeg::SubtitleImage>&);

protected:
    void runDecoder() override;

private:
    void checkPause();
    void checkSeek();

    QScopedPointer<SubtitleDecoderPrivate> d_ptr;
};

}

#endif // SUBTITLEDECODER_H
