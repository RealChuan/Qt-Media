#include "packet.h"

#include <QtCore>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

class Packet::PacketPrivate
{
public:
    explicit PacketPrivate(Packet *q)
        : q_ptr(q)
    {}
    ~PacketPrivate() { av_packet_free(&packet); }

    Packet *q_ptr;
    AVPacket *packet = nullptr;
};

Packet::Packet()
    : d_ptr(new PacketPrivate(this))
{
    d_ptr->packet = av_packet_alloc();

    if (d_ptr->packet == nullptr) {
        qWarning() << "Could not allocate packet";
    }
}

Packet::Packet(const Packet &other)
    : d_ptr(new PacketPrivate(this))
{
    d_ptr->packet = av_packet_clone(other.d_ptr->packet);

    if (d_ptr->packet == nullptr) {
        qWarning() << "Could not clone packet";
    }
}

Packet::Packet(Packet &&other) noexcept
    : d_ptr(new PacketPrivate(this))
{
    d_ptr->packet = other.d_ptr->packet;
    other.d_ptr->packet = nullptr;
}

Packet::~Packet() = default;

auto Packet::operator=(const Packet &other) -> Packet &
{
    if (this != &other) {
        d_ptr->packet = av_packet_clone(other.d_ptr->packet);

        if (d_ptr->packet == nullptr) {
            qWarning() << "Could not clone packet";
        }
    }

    return *this;
}

auto Packet::operator=(Packet &&other) noexcept -> Packet &
{
    if (this != &other) {
        d_ptr->packet = other.d_ptr->packet;
        other.d_ptr->packet = nullptr;
    }

    return *this;
}

auto Packet::isValid() -> bool
{
    if (nullptr == d_ptr->packet) {
        return false;
    }
    return d_ptr->packet->size > 0;
}

auto Packet::isKey() -> bool
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return (d_ptr->packet->flags & AV_PKT_FLAG_KEY) != 0;
}

void Packet::unref()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    av_packet_unref(d_ptr->packet);
}

void Packet::setPts(qint64 pts)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    d_ptr->packet->pts = pts;
}

auto Packet::pts() -> qint64
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->pts;
}

void Packet::setDuration(qint64 duration)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    d_ptr->packet->duration = duration;
}

auto Packet::duration() -> qint64
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->duration;
}

void Packet::setStreamIndex(int index)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    d_ptr->packet->stream_index = index;
}

auto Packet::streamIndex() const -> int
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->stream_index;
}

void Packet::rescaleTs(const AVRational &srcTimeBase, const AVRational &dstTimeBase)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    av_packet_rescale_ts(d_ptr->packet, srcTimeBase, dstTimeBase);
}

auto Packet::avPacket() -> AVPacket *
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet;
}

} // namespace Ffmpeg
