#include "packet.h"

#include <QtCore>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

Packet::Packet()
{
    m_packet = av_packet_alloc();
    Q_ASSERT(nullptr != m_packet);
}

Packet::Packet(const Packet &other)
{
    m_packet = av_packet_clone(other.m_packet);
}

Packet &Packet::operator=(const Packet &other)
{
    m_packet = av_packet_clone(other.m_packet);
    return *this;
}

Packet::~Packet()
{
    Q_ASSERT(nullptr != m_packet);
    av_packet_free(&m_packet);
}

void Packet::clear()
{
    av_packet_unref(m_packet);
}

void Packet::setPts(double pts)
{
    m_pts = pts;
}

double Packet::pts()
{
    return m_pts;
}

void Packet::setDuration(double duration)
{
    m_duration = duration;
}

double Packet::duration()
{
    return m_duration;
}

AVPacket *Packet::avPacket()
{
    Q_ASSERT(nullptr != m_packet);
    return m_packet;
}

} // namespace Ffmpeg
