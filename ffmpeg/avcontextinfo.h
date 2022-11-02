#ifndef AVCONTEXTINFO_H
#define AVCONTEXTINFO_H

#include <QObject>
#include <QSize>

struct AVStream;

namespace Ffmpeg {

class HardWareDecode;
class AVError;
class Subtitle;
class Packet;
class PlayFrame;
class CodecContext;
class AVContextInfo : public QObject
{
    Q_OBJECT
public:
    enum MediaType { Audio, Video, SubTiTle };
    Q_ENUM(MediaType)
    static QString mediaTypeString(MediaType type);

    explicit AVContextInfo(MediaType mediaType, QObject *parent = nullptr);
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

    bool imageAlloc(PlayFrame &frame, const QSize &size = QSize(-1, -1));

    void flush();

    double timebase();

    HardWareDecode *hardWareDecode();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    struct AVContextInfoPrivate;
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AVCONTEXTINFO_H
