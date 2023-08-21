#ifndef PACKET_H
#define PACKET_H

#include "ffmepg_global.h"

#include <QtCore>

struct AVPacket;
struct AVRational;

namespace Ffmpeg {

class FFMPEG_EXPORT Packet
{
public:
    Packet();
    Packet(const Packet &other);
    Packet(Packet &&other);
    ~Packet();

    Packet &operator=(const Packet &other);
    Packet &operator=(Packet &&other);

    bool isValid();

    bool isKey();

    void unref();

    void setPts(double pts);
    double pts();

    void setDuration(double duration);
    double duration();

    void setStreamIndex(int index);
    int streamIndex() const;

    void rescaleTs(const AVRational &srcTimeBase, const AVRational &dstTimeBase);

    AVPacket *avPacket();

private:
    class PacketPrivate;
    QScopedPointer<PacketPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PACKET_H
