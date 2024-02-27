#ifndef TRANSCODER_H
#define TRANSCODER_H

#include "encodecontext.hpp"
#include "mediainfo.hpp"

#include <ffmpeg/event/event.hpp>

#include <QThread>

extern "C" {
#include <libavcodec/codec_id.h>
}

namespace Ffmpeg {

class AVError;
class Frame;
class FFMPEG_EXPORT Transcoder : public QThread
{
    Q_OBJECT
public:
    explicit Transcoder(QObject *parent = nullptr);
    ~Transcoder() override;

    void setInFilePath(const QString &filePath);
    void parseInputFile();

    void setOutFilePath(const QString &filepath);

    void setGpuDecode(bool enable);
    auto isGpuDecode() -> bool;

    void setEncodeContexts(const EncodeContexts &encodeContexts);
    [[nodiscard]] auto decodeContexts() const -> EncodeContexts;

    [[nodiscard]] auto duration() const -> qint64; // microsecond
    auto mediaInfo() -> MediaInfo;
    void startPreviewFrames(int count);
    void setPreviewFrames(const std::vector<QSharedPointer<Frame>> &framePtrs);
    [[nodiscard]] auto previewFrames() const -> std::vector<QSharedPointer<Frame>>;

    void setRange(const QPair<qint64, qint64> &range);

    void setSubtitleFilename(const QString &filename);

    void startTranscode();
    void stopTranscode();

    auto fps() -> float;

    void setPropertyEventQueueMaxSize(size_t size);
    [[nodiscard]] auto propertEventyQueueMaxSize() const -> size_t;
    [[nodiscard]] auto propertyChangeEventSize() const -> size_t;
    auto takePropertyChangeEvent() -> PropertyChangeEventPtr;

signals:
    void eventIncrease();

protected:
    void run() override;

private:
    class TranscoderPrivate;
    QScopedPointer<TranscoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // TRANSCODER_H
