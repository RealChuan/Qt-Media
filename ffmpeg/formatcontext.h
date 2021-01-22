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

    bool openFilePath(const QString &filepath);
    bool findStream();
    QVector<int> findStreamIndex(int &audioIndex, int &videoIndex);

    AVStream *stream(int index);   //音频流

    bool readFrame(Packet *packet);

    int checkPktPlayRange(Packet *packet);

    void dumpFormat();

    AVFormatContext *avFormatContext();

private:
    QScopedPointer<FormatContextPrivate> d_ptr;
};

}

#endif // FORMATCONTEXT_H
