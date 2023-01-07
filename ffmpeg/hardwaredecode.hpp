#ifndef HARDWAREDECODE_HPP
#define HARDWAREDECODE_HPP

#include <QObject>

struct AVCodec;

namespace Ffmpeg {

class Frame;
class CodecContext;
class HardWareDecode : public QObject
{
public:
    explicit HardWareDecode(QObject *parent = nullptr);
    ~HardWareDecode();

    bool initPixelFormat(const AVCodec *decoder);
    bool initHardWareDevice(CodecContext *codecContext);
    Frame *transforFrame(Frame *in, bool &ok);

    bool isVaild();

private:
    void setError(int errorCode);

    struct HardWareDecodePrivate;
    QScopedPointer<HardWareDecodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // HARDWAREDECODE_HPP
