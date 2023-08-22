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
    ~AudioFrameConverter() override;

    auto convert(Frame *frame) -> QByteArray;

private:
    class AudioFrameConverterPrivate;
    QScopedPointer<AudioFrameConverterPrivate> d_ptr;
};

auto getAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize) -> QAudioFormat;

} // namespace Ffmpeg
