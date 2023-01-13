#pragma once

#include <QAudioFormat>

namespace Ffmpeg {

class CodecContext;
class Frame;
class AudioFrameConverter : public QObject
{
public:
    explicit AudioFrameConverter(CodecContext *codecCtx,
                                 QAudioFormat &format,
                                 QObject *parent = nullptr);
    ~AudioFrameConverter();

    QByteArray convert(Frame *frame);

private:
    class AudioFrameConverterPrivate;
    QScopedPointer<AudioFrameConverterPrivate> d_ptr;
};

QAudioFormat getAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize);

} // namespace Ffmpeg
