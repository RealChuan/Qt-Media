#ifndef HARDWAREDECODE_HPP
#define HARDWAREDECODE_HPP

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

//#define HardWareDecodeOn

struct AVCodec;

namespace Ffmpeg {

class PlayFrame;
class AVError;
class CodecContext;
struct HardWareDecodePrivate;
class HardWareDecode : public QObject
{
    Q_OBJECT
public:
    explicit HardWareDecode(QObject *parent = nullptr);
    ~HardWareDecode();

    bool initPixelFormat(const AVCodec *decoder);
    bool initHardWareDevice(CodecContext *codecContext);
    PlayFrame *transforFrame(PlayFrame *playFrame, bool &ok);

    bool isVaild();

    AVError avError();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    QScopedPointer<HardWareDecodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // HARDWAREDECODE_HPP
