#ifndef AVIMAGE_H
#define AVIMAGE_H

class PlayFrame;
class CodecContext;
class AVImage
{
public:
    explicit AVImage(CodecContext *codecCtx);
    ~AVImage();

    struct SwsContext *swsContext();

    void scale(PlayFrame *in, PlayFrame *out, int height);

private:
    struct SwsContext *m_swsContext;
};

#endif // AVIMAGE_H
