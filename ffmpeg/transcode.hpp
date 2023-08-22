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
    ~Transcode() override;

    void setUseGpuDecode(bool useGpu);

    void setInFilePath(const QString &filePath);
    void setOutFilePath(const QString &filepath);

    void setAudioEncodecID(AVCodecID codecID);
    void setVideoEnCodecID(AVCodecID codecID);
    void setAudioEncodecName(const QString &name);
    void setVideoEncodecName(const QString &name);

    void setSize(const QSize &size);
    void setSubtitleFilename(const QString &filename);

    void setQuailty(int quailty);
    void setMinBitrate(int64_t bitrate);
    void setMaxBitrate(int64_t bitrate);
    void setCrf(int crf);

    void setPreset(const QString &preset);
    [[nodiscard]] auto preset() const -> QString;
    [[nodiscard]] auto presets() const -> QStringList;

    void setTune(const QString &tune);
    [[nodiscard]] auto tune() const -> QString;
    [[nodiscard]] auto tunes() const -> QStringList;

    void setProfile(const QString &profile);
    [[nodiscard]] auto profile() const -> QString;
    [[nodiscard]] auto profiles() const -> QStringList;

    void startTranscode();
    void stopTranscode();

    auto fps() -> float;

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
