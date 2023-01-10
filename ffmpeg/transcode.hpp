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
    void setMinBitrate(int64_t bitrate);
    void setMaxBitrate(int64_t bitrate);
    void setCrf(int crf);

    void setPreset(const QString &preset);
    QString preset() const;
    QStringList presets() const;

    void setTune(const QString &tune);
    QString tune() const;
    QStringList tunes() const;

    void setProfile(const QString &profile);
    QString profile() const;
    QStringList profiles() const;

    void startTranscode();
    void stopTranscode();

    float fps();

signals:
    void error(const Ffmpeg::AVError &avError);
    void progressChanged(qreal); // 0.XXXX

protected:
    void run() override;

private:
    void buildConnect();
    void loop();

    class TranscodePrivate;
    QScopedPointer<TranscodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // TRANSCODE_HPP
