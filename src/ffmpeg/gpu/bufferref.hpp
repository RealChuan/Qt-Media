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
    ~BufferRef() override;

    auto hwdeviceCtxCreate(AVHWDeviceType hwDeviceType) -> bool;
    auto hwframeCtxAlloc() -> BufferRef *;

    auto hwframeCtxInit() -> bool;

    auto ref() -> AVBufferRef *;
    auto avBufferRef() -> AVBufferRef *;

private:
    class BufferRefPrivate;
    QScopedPointer<BufferRefPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // BUFFERREF_HPP
