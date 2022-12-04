#ifndef AVAUDIO_H
#define AVAUDIO_H

#include <QAudioFormat>

namespace Ffmpeg {

class CodecContext;
class Frame;
class AVAudio
{
    Q_DISABLE_COPY_MOVE(AVAudio)
public:
    explicit AVAudio(CodecContext *codecCtx, QAudioFormat &format);
    ~AVAudio();

    QByteArray convert(Frame *frame);

private:
    class AVAudioPrivate;
    QScopedPointer<AVAudioPrivate> d_ptr;
};

QAudioFormat geAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize);

} // namespace Ffmpeg

#endif // AVAUDIO_H
