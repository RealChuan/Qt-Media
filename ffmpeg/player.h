#ifndef PLAYER_H
#define PLAYER_H

#include "ffmepg_global.h"
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
    auto speed() -> double;
    auto isGpuDecode() -> bool;
    auto mediaState() -> MediaState;

    [[nodiscard]] auto duration() const -> qint64; // microsecond
    [[nodiscard]] auto position() const -> qint64; // microsecond
    [[nodiscard]] auto fames() const -> qint64;
    [[nodiscard]] auto resolutionRatio() const -> QSize;
    [[nodiscard]] auto fps() const -> double;

    [[nodiscard]] auto audioIndex() const -> int;
    [[nodiscard]] auto videoIndex() const -> int;
    [[nodiscard]] auto subtitleIndex() const -> int;

    void setVideoRenders(QVector<VideoRender *> videoRenders);
    QVector<VideoRender *> videoRenders();

    void setPropertyEventQueueMaxSize(size_t size);
    [[nodiscard]] size_t propertEventyQueueMaxSize() const;
    [[nodiscard]] size_t propertyChangeEventSize() const;
    PropertyChangeEventPtr takePropertyChangeEvent();

    void setEventQueueMaxSize(size_t size);
    [[nodiscard]] size_t eventQueueMaxSize() const;
    [[nodiscard]] size_t eventSize() const;
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
