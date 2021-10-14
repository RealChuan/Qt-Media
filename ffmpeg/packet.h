#ifndef PACKET_H
#define PACKET_H

#include <QtCore>

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
    Q_DISABLE_COPY(Packet)

    AVPacket *m_packet;
};

} // namespace Ffmpeg

#endif // PACKET_H
