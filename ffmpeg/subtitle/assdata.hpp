#ifndef ASSDATA_HPP
#define ASSDATA_HPP

#include <QRect>
#include <QSharedData>

namespace Ffmpeg {

class AssData : public QSharedData
{
public:
    AssData() {}
    AssData(const AssData &other)
        : QSharedData(other)
        , rgba(other.rgba)
        , rect(other.rect)
    {}
    ~AssData() {}

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
    ~AssDataInfo() {}

    void setRGBA(const QByteArray &rgba) { d_ptr->rgba = rgba; }
    QByteArray rgba() const { return d_ptr->rgba; }

    void setRect(const QRect &rect) { d_ptr->rect = rect; }
    QRect rect() const { return d_ptr->rect; }

private:
    QExplicitlySharedDataPointer<AssData> d_ptr;
};

using AssDataInfoList = QVector<AssDataInfo>;

} // namespace Ffmpeg

#endif // ASSDATA_HPP
