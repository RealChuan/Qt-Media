#ifndef PLAYER_H
#define PLAYER_H

#include "mediainfo.hpp"

#include <ffmpeg/event/event.hpp>

#include <QThread>

namespace Ffmpeg {

class VideoRender;

class FFMPEG_EXPORT Player : public QThread
{
    Q_OBJECT
public:
    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    [[nodiscard]] auto filePath() const -> QString &;
    auto isOpen() -> bool;
    static auto speed() -> double;
    auto isGpuDecode() -> bool;
    auto mediaState() -> MediaState;

    [[nodiscard]] auto duration() const -> qint64; // microsecond
    [[nodiscard]] auto position() const -> qint64; // microsecond
    [[nodiscard]] auto fames() const -> qint64;
    [[nodiscard]] auto resolutionRatio() const -> QSize;
    [[nodiscard]] auto fps() const -> double;
    auto mediaInfo() -> MediaInfo;

    [[nodiscard]] auto audioIndex() const -> int;
    [[nodiscard]] auto videoIndex() const -> int;
    [[nodiscard]] auto subtitleIndex() const -> int;

    void setVideoRenders(const QVector<VideoRender *> &videoRenders);
    auto videoRenders() -> QVector<VideoRender *>;

    void setPropertyEventQueueMaxSize(size_t size);
    [[nodiscard]] auto propertEventyQueueMaxSize() const -> size_t;
    [[nodiscard]] auto propertyChangeEventSize() const -> size_t;
    auto takePropertyChangeEvent() -> PropertyChangeEventPtr;

    void setEventQueueMaxSize(size_t size);
    [[nodiscard]] auto eventQueueMaxSize() const -> size_t;
    [[nodiscard]] auto eventSize() const -> size_t;
    auto addEvent(const EventPtr &eventPtr) -> bool;

public slots:
    void onPlay();

private slots:
    void onPositionChanged(qint64 position);

signals:
    void eventIncrease();

protected:
    void run() override;

private:
    void buildConnect(bool state = true);

    class PlayerPrivate;
    QScopedPointer<PlayerPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // PLAYER_H
