#ifndef FORMATCONTEXT_H
#define FORMATCONTEXT_H

#include <QObject>

extern "C" {
#include <libavutil/avutil.h>
}

#define Seek_Offset 2 // S

struct AVStream;
struct AVFormatContext;

namespace Ffmpeg {

class AVError;
class Packet;
class FormatContext : public QObject
{
    Q_OBJECT
public:
    explicit FormatContext(QObject *parent = nullptr);
    ~FormatContext();

    bool isOpen();

    bool openFilePath(const QString &filepath);
    void close();

    bool findStream();

    QMap<int, QString> audioMap() const;
    QVector<int> videoIndexs() const;
    QMap<int, QString> subtitleMap() const;

    int findBestStreamIndex(AVMediaType type) const;
    // 丢弃除指定3个stream的音视频流，优化av_read_frame性能
    void discardStreamExcluded(int audioIndex, int videoIndex, int subtitleIndex);

    AVStream *stream(int index); //音频流

    bool readFrame(Packet *packet);

    int checkPktPlayRange(Packet *packet);

    bool seekFirstFrame();
    bool seek(int64_t timestamp);
    bool seek(int index, int64_t timestamp); // s

    void dumpFormat();

    qint64 duration() const; // ms

    QImage &coverImage() const;

    AVError avError() const;

    AVFormatContext *avFormatContext();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void findStreamIndex();
    void initMetaData();
    void printInformation();

    void setError(int errorCode);

    class FormatContextPrivate;
    QScopedPointer<FormatContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FORMATCONTEXT_H
