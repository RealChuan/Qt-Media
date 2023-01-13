#include "packet.h"

#include <QtCore>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

class Packet::PacketPrivate
{
public:
    PacketPrivate() {}
    ~PacketPrivate() { reset(); }

    void reset()
    {
        pts = 0;
        duration = 0;
        Q_ASSERT(nullptr != packet);
        av_packet_free(&packet);
    }

    AVPacket *packet = nullptr;
    double pts = 0;
    double duration = 0;
};

Packet::Packet()
    : d_ptr(new PacketPrivate)
{
    d_ptr->packet = av_packet_alloc();
    Q_ASSERT(nullptr != d_ptr->packet);
}

Packet::Packet(const Packet &other)
    : d_ptr(new PacketPrivate)
{
    d_ptr->packet = av_packet_clone(other.d_ptr->packet);
    d_ptr->pts = other.d_ptr->pts;
    d_ptr->duration = other.d_ptr->duration;
}

Packet &Packet::operator=(const Packet &other)
{
    d_ptr->reset();
    d_ptr->packet = av_packet_clone(other.d_ptr->packet);
    d_ptr->pts = other.d_ptr->pts;
    d_ptr->duration = other.d_ptr->duration;
    return *this;
}

Packet::~Packet() {}

bool Packet::isKey()
{
    return d_ptr->packet->flags & AV_PKT_FLAG_KEY;
}

void Packet::unref()
{
    av_packet_unref(d_ptr->packet);
}

void Packet::setPts(double pts)
{
    d_ptr->pts = pts;
}

double Packet::pts()
{
    return d_ptr->pts;
}

void Packet::setDuration(double duration)
{
    d_ptr->duration = duration;
}

double Packet::duration()
{
    return d_ptr->duration;
}

void Packet::setStreamIndex(int index)
{
    d_ptr->packet->stream_index = index;
}

int Packet::streamIndex() const
{
    return d_ptr->packet->stream_index;
}

void Packet::rescaleTs(const AVRational &srcTimeBase, const AVRational &dstTimeBase)
{
    av_packet_rescale_ts(d_ptr->packet, srcTimeBase, dstTimeBase);
}

AVPacket *Packet::avPacket()
{
    Q_ASSERT(nullptr != d_ptr->packet);
    return d_ptr->packet;
}

} // namespace Ffmpeg
