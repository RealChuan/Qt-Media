#pragma once

#include <QObject>
#include <QVariantList>

namespace Ffmpeg {

class Event : public QObject
{
public:
    enum EventType {
        None,
        OpenMedia,
        Seek,
        Pause,
        Stop,
        Volume,
        Speed,
        AudioTrack,
        SubtitleTrack,
        VideoRenders,
        UseGpuDecode,
    };
    Q_ENUM(EventType);

    explicit Event(EventType type, const QVariantList &args = {}, QObject *parent = nullptr);
    ~Event() override;

    [[nodiscard]] auto type() const -> EventType;
    [[nodiscard]] auto args() const -> QVariantList;

private:
    class EventPrivate;
    QScopedPointer<EventPrivate> d_ptr;
};

using EventPtr = QSharedPointer<Event>;

} // namespace Ffmpeg
