#ifndef PACKET_H
#define PACKET_H

#include "ffmepg_global.h"

#include <QtCore>

struct AVPacket;
struct AVRational;

namespace Ffmpeg {

class FFMPEG_EXPORT Packet
{
public:
    Packet();
    Packet(const Packet &other);
    Packet(Packet &&other) noexcept;
    ~Packet();

    auto operator=(const Packet &other) -> Packet &;
    auto operator=(Packet &&other) noexcept -> Packet &;

    auto isValid() -> bool;

    auto isKey() -> bool;

    void unref();

    void setPts(qint64 pts); // microseconds
    auto pts() -> qint64;

    void setDuration(qint64 duration); // microseconds
    auto duration() -> qint64;

    void setStreamIndex(int index);
    [[nodiscard]] auto streamIndex() const -> int;

    void rescaleTs(const AVRational &srcTimeBase, const AVRational &dstTimeBase);

    auto avPacket() -> AVPacket *;

private:
    class PacketPrivate;
    QScopedPointer<PacketPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PACKET_H
