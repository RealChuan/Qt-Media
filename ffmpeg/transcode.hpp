#ifndef TRANSCODE_HPP
#define TRANSCODE_HPP

#include "ffmepg_global.h"

#include <QSize>
#include <QThread>

extern "C" {
#include <libavcodec/codec_id.h>
}

namespace Ffmpeg {

class AVError;
class Frame;
class FFMPEG_EXPORT Transcode : public QThread
{
    Q_OBJECT
public:
    explicit Transcode(QObject *parent = nullptr);
    ~Transcode();

    void setInFilePath(const QString &filePath);
    void setOutFilePath(const QString &filepath);

    void setAudioEncodecID(AVCodecID codecID);
    void setVideoEnCodecID(AVCodecID codecID);

    void setSize(const QSize &size);
    void setQuailty(int quailty);

    void startTranscode();
    void stopTranscode();

signals:
    void error(const Ffmpeg::AVError &avError);
    void progressChanged(qreal); // 0.XXXX

protected:
    void run() override;

private:
    bool openInputFile();
    bool openOutputFile();
    void initFilters();
    void loop();
    void cleanup();
    bool filterEncodeWriteframe(Frame *frame, uint stream_index);
    bool encodeWriteFrame(uint stream_index, int flush, Frame *frame);
    bool flushEncoder(uint stream_index);

    void setError(int errorCode);

    class TranscodePrivate;
    QScopedPointer<TranscodePrivate> d_ptr;
};

struct CodecInfo
{
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    AVCodecID codecID = AV_CODEC_ID_NONE;
    QSize size = QSize(-1, -1);
    QPair<int, int> quantizer = {-1, -1};
};

QVector<CodecInfo> FFMPEG_EXPORT getFileCodecInfo(const QString &filePath);

} // namespace Ffmpeg

#endif // TRANSCODE_HPP
