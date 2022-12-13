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

    bool isKey();

    void clear();

    void setPts(double pts);
    double pts();

    void setDuration(double duration);
    double duration();

    AVPacket *avPacket();

private:
    AVPacket *m_packet;
    double m_pts = 0;
    double m_duration = 0;
};

} // namespace Ffmpeg

#endif // PACKET_H
