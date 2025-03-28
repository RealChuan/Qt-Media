#ifndef ASSDATA_HPP
#define ASSDATA_HPP

#include <QRect>
#include <QSharedData>

namespace Ffmpeg {

class AssData : public QSharedData
{
public:
    AssData() = default;
    AssData(const AssData &other)
        : QSharedData(other)
        , rgba(other.rgba)
        , rect(other.rect)
    {}
    ~AssData() = default;

    QByteArray rgba;
    QRect rect;
};

class AssDataInfo
{
public:
    AssDataInfo()
        : d_ptr(new AssData)
    {}
    AssDataInfo(const QByteArray &rgba, const QRect &rect)
        : d_ptr(new AssData)
    {
        setRGBA(rgba);
        setRect(rect);
    }
    AssDataInfo(const AssDataInfo &other)
        : d_ptr(other.d_ptr)
    {}
    ~AssDataInfo() = default;

    void setRGBA(const QByteArray &rgba) { d_ptr->rgba = rgba; }
    [[nodiscard]] auto rgba() const -> QByteArray { return d_ptr->rgba; }

    void setRect(const QRect &rect) { d_ptr->rect = rect; }
    [[nodiscard]] auto rect() const -> QRect { return d_ptr->rect; }

private:
    QExplicitlySharedDataPointer<AssData> d_ptr;
};

using AssDataInfoList = QList<AssDataInfo>;

} // namespace Ffmpeg

#endif // ASSDATA_HPP
