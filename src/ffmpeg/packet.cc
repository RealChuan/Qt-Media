#include "packet.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

struct AVPacketDeleter
{
    void operator()(AVPacket *p) const noexcept { av_packet_free(&p); }
};
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;

class Packet::PacketPrivate
{
public:
    explicit PacketPrivate()
        : packet(av_packet_alloc())
    {
        if (!packet) {
            throw std::bad_alloc();
        }
    }
    ~PacketPrivate() = default;

    AVPacketPtr packet;
};

Packet::Packet()
    : d_ptr(std::make_unique<PacketPrivate>())
{}

Packet::Packet(const Packet &other)
    : d_ptr(std::make_unique<PacketPrivate>())
{
    AVPacket *src = other.d_ptr->packet.get();
    if (src && src->buf) { // 有数据才引用
        if (av_packet_ref(d_ptr->packet.get(), src) < 0)
            throw std::runtime_error("av_packet_ref failed");
    }
}

Packet::Packet(Packet &&other) noexcept = default;

Packet &Packet::operator=(const Packet &other)
{
    if (this != &other) {
        AVPacket *src = other.d_ptr->packet.get();
        av_packet_unref(d_ptr->packet.get()); // 先释放旧数据
        if (src && src->buf) {
            if (av_packet_ref(d_ptr->packet.get(), src) < 0)
                throw std::runtime_error("av_packet_ref failed");
        }
    }
    return *this;
}

Packet &Packet::operator=(Packet &&other) noexcept = default;

Packet::~Packet() = default;

auto Packet::isValid() const -> bool
{
    auto *p = d_ptr->packet.get();
    return p && p->size > 0;
}

auto Packet::isKey() const -> bool
{
    auto *p = d_ptr->packet.get();
    return p && (p->flags & AV_PKT_FLAG_KEY);
}

void Packet::unref()
{
    av_packet_unref(d_ptr->packet.get());
}

void Packet::setPts(qint64 pts)
{
    d_ptr->packet->pts = pts;
}

auto Packet::pts() const -> qint64
{
    return d_ptr->packet->pts;
}

void Packet::setDuration(qint64 duration)
{
    d_ptr->packet->duration = duration;
}

auto Packet::duration() const -> qint64
{
    return d_ptr->packet->duration;
}

void Packet::setStreamIndex(int index)
{
    d_ptr->packet->stream_index = index;
}
auto Packet::streamIndex() const -> int
{
    return d_ptr->packet->stream_index;
}

void Packet::rescaleTs(const AVRational &src, const AVRational &dst)
{
    av_packet_rescale_ts(d_ptr->packet.get(), src, dst);
}

auto Packet::avPacket() -> AVPacket *
{
    return d_ptr->packet.get();
}

} // namespace Ffmpeg
