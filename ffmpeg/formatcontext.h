#ifndef FORMATCONTEXT_H
#define FORMATCONTEXT_H

#include <QObject>

struct AVStream;
struct AVFormatContext;

namespace Ffmpeg {

class Packet;
class FormatContextPrivate;
class FormatContext : public QObject
{
public:
    explicit FormatContext(QObject *parent = nullptr);
    ~FormatContext();

    QString error() const;

    bool isOpen();

    bool openFilePath(const QString &filepath);
    void close();

    bool findStream();

    QVector<int> audioIndexs() const;
    QVector<int> videoIndexs() const;
    QVector<int> subtitleIndexs() const;

    AVStream *stream(int index);   //音频流

    bool readFrame(Packet *packet);

    int checkPktPlayRange(Packet *packet);

    bool seek(int index, int64_t timestamp); // s

    void dumpFormat();

    AVFormatContext *avFormatContext();

    qint64 duration(); // ms

    QImage coverImage() const;

private:
    void findStreamIndex();
    void initMetaData();

    QScopedPointer<FormatContextPrivate> d_ptr;
};

}

#endif // FORMATCONTEXT_H
