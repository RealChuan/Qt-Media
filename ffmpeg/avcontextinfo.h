#ifndef AVCONTEXTINFO_H
#define AVCONTEXTINFO_H

#include <QObject>

struct AVStream;

namespace Ffmpeg {

class AVError;
class Subtitle;
class Packet;
class PlayFrame;
class CodecContext;
struct AVContextInfoPrivate;
class AVContextInfo : public QObject
{
    Q_OBJECT
public:
    AVContextInfo(QObject *parent = nullptr);
    ~AVContextInfo();

    CodecContext *codecCtx();

    void resetIndex();
    void setIndex(int index);
    int index();

    bool isIndexVaild();

    void setStream(AVStream *stream);
    AVStream *stream();

    bool findDecoder();

    bool sendPacket(Packet *packet);
    bool receiveFrame(PlayFrame *frame);
    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);

    unsigned char *imageBuffer(PlayFrame &frame);
    void clearImageBuffer();

    void flush();

    double timebase();

signals:
    void error(const Ffmpeg::AVError& avError);

private:
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

}

#endif // AVCONTEXTINFO_H
