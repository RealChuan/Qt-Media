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
    ~HardWareDecode() override;

    auto initPixelFormat(const AVCodec *decoder) -> bool;
    auto initHardWareDevice(CodecContext *codecContext) -> bool;
    auto transFromGpu(const QSharedPointer<Frame> &inPtr, bool &ok) -> QSharedPointer<Frame>;

    auto isVaild() -> bool;

private:
    class HardWareDecodePrivate;
    QScopedPointer<HardWareDecodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // HARDWAREDECODE_HPP
