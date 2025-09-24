#pragma once

#include "ffmepg_global.h"

#include <QtCore>

#include <memory>
#include <vector>

struct AVPacket;
struct AVRational;

namespace Ffmpeg {

class FFMPEG_EXPORT Packet
{
public:
    Packet();
    Packet(const Packet &other);
    Packet(Packet &&other) noexcept;
    Packet &operator=(const Packet &other);
    Packet &operator=(Packet &&other) noexcept;
    ~Packet();

    auto isValid() const -> bool;
    auto isKey() const -> bool;
    void unref();
    void setPts(qint64 pts); // microseconds
    auto pts() const -> qint64;
    void setDuration(qint64 duration); // microseconds
    auto duration() const -> qint64;
    void setStreamIndex(int index);
    auto streamIndex() const -> int;
    void rescaleTs(const AVRational &src, const AVRational &dst);
    auto avPacket() -> AVPacket *;

private:
    class PacketPrivate;
    std::unique_ptr<PacketPrivate> d_ptr;
};

using PacketPtr = std::shared_ptr<Packet>;
using PacketPtrList = std::vector<PacketPtr>;

} // namespace Ffmpeg
