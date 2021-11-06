#ifndef PACKET_H
#define PACKET_H

#include <QtCore>

struct AVPacket;

namespace Ffmpeg {

class Packet
{
public:
    explicit Packet();
    Packet(const Packet &other);
    Packet &operator=(const Packet &other);
    ~Packet();

    void clear();

    AVPacket *avPacket();

private:
    AVPacket *m_packet;
};

} // namespace Ffmpeg

#endif // PACKET_H
