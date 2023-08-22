#include "packet.h"

#include <QtCore>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

class Packet::PacketPrivate
{
public:
    PacketPrivate(Packet *q)
        : q_ptr(q)
    {}
    ~PacketPrivate()
    {
        if (packet) {
            av_packet_free(&packet);
        }
    }

    Packet *q_ptr;
    AVPacket *packet = nullptr;
};

Packet::Packet()
    : d_ptr(new PacketPrivate(this))
{
    d_ptr->packet = av_packet_alloc();

    if (!d_ptr->packet) {
        qWarning() << "Could not allocate packet";
    }

    av_init_packet(d_ptr->packet);
}

Packet::Packet(const Packet &other)
    : d_ptr(new PacketPrivate(this))
{
    d_ptr->packet = av_packet_clone(other.d_ptr->packet);

    if (!d_ptr->packet) {
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

Packet &Packet::operator=(const Packet &other)
{
    if (this != &other) {
        d_ptr->packet = av_packet_clone(other.d_ptr->packet);

        if (!d_ptr->packet) {
            qWarning() << "Could not clone packet";
        }
    }

    return *this;
}

Packet &Packet::operator=(Packet &&other) noexcept
{
    if (this != &other) {
        d_ptr->packet = other.d_ptr->packet;
        other.d_ptr->packet = nullptr;
    }

    return *this;
}

bool Packet::isValid()
{
    if (nullptr == d_ptr->packet) {
        return false;
    }
    return d_ptr->packet->size > 0;
}

bool Packet::isKey()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->flags & AV_PKT_FLAG_KEY;
}

void Packet::unref()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    av_packet_unref(d_ptr->packet);
}

void Packet::setPts(double pts)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    d_ptr->packet->pts = pts * AV_TIME_BASE;
}

double Packet::pts()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->pts / (double) AV_TIME_BASE;
}

void Packet::setDuration(double duration)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    d_ptr->packet->duration = duration * AV_TIME_BASE;
}

double Packet::duration()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->duration / (double) AV_TIME_BASE;
}

void Packet::setStreamIndex(int index)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    d_ptr->packet->stream_index = index;
}

int Packet::streamIndex() const
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet->stream_index;
}

void Packet::rescaleTs(const AVRational &srcTimeBase, const AVRational &dstTimeBase)
{
    Q_ASSERT(nullptr != d_ptr->packet);
    av_packet_rescale_ts(d_ptr->packet, srcTimeBase, dstTimeBase);
}

AVPacket *Packet::avPacket()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet;
}

} // namespace Ffmpeg
