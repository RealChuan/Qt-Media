#ifndef AVCONTEXTINFO_H
#define AVCONTEXTINFO_H

#include <QObject>

struct AVStream;

namespace Ffmpeg {

class AVError;
class Subtitle;
class Packet;
class Frame;
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

    bool findDecoder(bool useGpu = false);

    bool sendPacket(Packet *packet);
    bool receiveFrame(Frame *frame);
    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);
    // sendPacket and receiveFrame
    Frame *decodeFrame(Packet *packet);

    void flush();

    double timebase();
    double fps();
    qint64 fames();
    QSize resolutionRatio();

    bool isGpuDecode();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void showCodecpar();
    void showMetaData();

    struct AVContextInfoPrivate;
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AVCONTEXTINFO_H
