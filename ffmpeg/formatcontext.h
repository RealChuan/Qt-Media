#ifndef FORMATCONTEXT_H
#define FORMATCONTEXT_H

#include "ffmepg_global.h"

#include <QObject>

extern "C" {
#include <libavutil/avutil.h>
}

struct AVStream;
struct AVFormatContext;

namespace Ffmpeg {

class Packet;
class FFMPEG_EXPORT FormatContext : public QObject
{
public:
    enum OpenMode { ReadOnly = 1, WriteOnly };

    explicit FormatContext(QObject *parent = nullptr);
    ~FormatContext() override;

    void copyChapterFrom(FormatContext *src);

    auto isOpen() -> bool;
    auto openFilePath(const QString &filepath, OpenMode mode = ReadOnly) -> bool;
    void close();

    auto avio_open() -> bool;
    void avio_close();

    auto writeHeader() -> bool;
    auto writePacket(Packet *packet) -> bool;
    auto writeTrailer() -> bool;

    auto findStream() -> bool;
    [[nodiscard]] auto streams() const -> int;
    auto stream(int index) -> AVStream *; //音频流
    auto createStream() -> AVStream *;

    [[nodiscard]] auto audioMap() const -> QMap<int, QString>;
    [[nodiscard]] QVector<int> videoIndexs() const;
    [[nodiscard]] auto subtitleMap() const -> QMap<int, QString>;

    [[nodiscard]] auto findBestStreamIndex(AVMediaType type) const -> int;
    // 丢弃除indexs中包含的音视频流，优化av_read_frame性能
    void discardStreamExcluded(QVector<int> indexs);

    auto seekFirstFrame() -> bool;
    auto seek(qint64 timestamp) -> bool;            // microsecond
    auto seek(int index, qint64 timestamp) -> bool; // microsecond

    auto readFrame(Packet *packet) -> bool;

    auto checkPktPlayRange(Packet *packet) -> bool;

    [[nodiscard]] auto guessFrameRate(int index) const -> AVRational;
    auto guessFrameRate(AVStream *stream) const -> AVRational;

    [[nodiscard]] auto duration() const -> qint64; // microsecond

    [[nodiscard]] auto coverImage() const -> QImage &;

    void printFileInfo();

    void dumpFormat();

    auto avFormatContext() -> AVFormatContext *;

private:
    class FormatContextPrivate;
    QScopedPointer<FormatContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FORMATCONTEXT_H
