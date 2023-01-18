#ifndef BUFFERREF_HPP
#define BUFFERREF_HPP

#include <QObject>

extern "C" {
#include <libavutil/hwcontext.h>
}

struct AVBufferRef;

namespace Ffmpeg {

class BufferRef : public QObject
{
public:
    explicit BufferRef(QObject *parent = nullptr);
    ~BufferRef();

    bool hwdeviceCtxCreate(AVHWDeviceType hwDeviceType);
    BufferRef *hwframeCtxAlloc();

    bool hwframeCtxInit();

    AVBufferRef *ref();
    AVBufferRef *avBufferRef();

private:
    class BufferRefPrivate;
    QScopedPointer<BufferRefPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // BUFFERREF_HPP
