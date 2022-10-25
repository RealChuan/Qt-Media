#ifndef FORMATCONTEXT_H
#define FORMATCONTEXT_H

#include <QObject>

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

    AVStream *stream(int index); //音频流

    bool readFrame(Packet *packet);

    int checkPktPlayRange(Packet *packet);

    bool seek(int index, int64_t timestamp); // s

    void dumpFormat();

    AVFormatContext *avFormatContext();

    qint64 duration(); // ms

    QImage &coverImage() const;

    AVError avError();

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
