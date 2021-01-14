#ifndef PACKET_H
#define PACKET_H

struct AVPacket;

namespace Ffmpeg {

class Packet
{
public:
    explicit Packet();
    ~Packet();

    void clear();

    AVPacket *avPacket();

private:
    AVPacket *m_packet;
};

}

#endif // PACKET_H
