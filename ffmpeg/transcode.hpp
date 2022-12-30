#ifndef TRANSCODE_HPP
#define TRANSCODE_HPP

#include "ffmepg_global.h"

extern "C" {
#include <libavcodec/codec_id.h>
}

#include <QThread>

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

    void setVideoEnCodecID(AVCodecID codecID);

    void startTranscode();
    void stopTranscode();

    void reset();

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

} // namespace Ffmpeg

#endif // TRANSCODE_HPP
