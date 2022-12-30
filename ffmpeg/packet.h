#ifndef PACKET_H
#define PACKET_H

#include <QtCore>

struct AVPacket;
struct AVRational;

namespace Ffmpeg {

class Packet
{
public:
    explicit Packet();
    Packet(const Packet &other);
    Packet &operator=(const Packet &other);
    ~Packet();

    bool isKey();

    void unref();

    void setPts(double pts);
    double pts();

    void setDuration(double duration);
    double duration();

    void setStreamIndex(int index);

    void rescaleTs(const AVRational &srcTimeBase, const AVRational &dstTimeBase);

    AVPacket *avPacket();

private:
    class PacketPrivate;
    QScopedPointer<PacketPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PACKET_H
