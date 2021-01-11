#ifndef FORMATCONTEXT_H
#define FORMATCONTEXT_H

#include <QObject>

extern "C"{
#include <libavformat/avformat.h>
}

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
    void findStreamIndex(int &audioIndex, int &videoIndex);

    AVStream *stream(int index);   //音频流

    bool readFrame(Packet *packet);

    void dumpFormat();

private:
    QScopedPointer<FormatContextPrivate> d_ptr;
};

#endif // FORMATCONTEXT_H
