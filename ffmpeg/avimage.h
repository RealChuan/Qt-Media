#ifndef AVIMAGE_H
#define AVIMAGE_H

#include <QtCore>

struct SwsContext;

namespace Ffmpeg {

class PlayFrame;
class CodecContext;
class AVImage
{
public:
    explicit AVImage(CodecContext *codecCtx);
    ~AVImage();

    void scale(PlayFrame *in, PlayFrame *out, int height);

private:
    Q_DISABLE_COPY(AVImage)

    void flush(PlayFrame *frame);

    struct SwsContext *m_swsContext;
};

} // namespace Ffmpeg

#endif // AVIMAGE_H
