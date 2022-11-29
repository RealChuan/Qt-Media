#ifndef HARDWAREDECODE_HPP
#define HARDWAREDECODE_HPP

#include <QObject>

#define HardWareDecodeOn

struct AVCodec;

namespace Ffmpeg {

class Frame;
class AVError;
class CodecContext;
class HardWareDecode : public QObject
{
    Q_OBJECT
public:
    explicit HardWareDecode(QObject *parent = nullptr);
    ~HardWareDecode();

    bool initPixelFormat(const AVCodec *decoder);
    bool initHardWareDevice(CodecContext *codecContext);
    Frame *transforFrame(Frame *playFrame, bool &ok);

    bool isVaild();

    AVError avError();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    struct HardWareDecodePrivate;
    QScopedPointer<HardWareDecodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // HARDWAREDECODE_HPP
