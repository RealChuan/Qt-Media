#ifndef PACKET_H
#define PACKET_H

struct AVPacket;
class Packet
{
public:
    explicit Packet();
    ~Packet();

    AVPacket *avPacket();

private:
    AVPacket *m_packet;
};

#endif // PACKET_H
