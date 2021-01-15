#include "packet.h"

#include <QtCore>

extern "C"{
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

Packet::~Packet()
{
    Q_ASSERT(nullptr != m_packet);
    av_packet_free(&m_packet);
}

void Packet::clear()
{
    av_packet_unref(m_packet);
}

AVPacket *Packet::avPacket()
{
    Q_ASSERT(nullptr != m_packet);
    return m_packet;
}

}
