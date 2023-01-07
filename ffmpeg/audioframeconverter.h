#pragma once

#include <QAudioFormat>

namespace Ffmpeg {

class CodecContext;
class Frame;
class AudioFrameConverter
{
    Q_DISABLE_COPY_MOVE(AudioFrameConverter)
public:
    explicit AudioFrameConverter(CodecContext *codecCtx, QAudioFormat &format);
    ~AudioFrameConverter();

    QByteArray convert(Frame *frame);

private:
    void setError(int errorCode);

    class AudioFrameConverterPrivate;
    QScopedPointer<AudioFrameConverterPrivate> d_ptr;
};

QAudioFormat getAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize);

} // namespace Ffmpeg
