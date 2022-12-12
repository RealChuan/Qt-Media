#ifndef VIDEOOUTPUTRENDER_HPP
#define VIDEOOUTPUTRENDER_HPP

#include <QImage>
#include <QObject>

#include <ffmpeg/ffmepg_global.h>
#include <ffmpeg/subtitle.h>

namespace Ffmpeg {

class FFMPEG_EXPORT VideoOutputRender
{
    Q_DISABLE_COPY_MOVE(VideoOutputRender)
public:
    explicit VideoOutputRender() {}

    virtual void setDisplayImage(const QImage &image) = 0;
    void onFinish();

protected:
    void drawBackGround(QPainter &painter, const QRect &rect);
    void drawVideoImage(QPainter &painter, const QRect &rect);

    QImage m_image;
};

} // namespace Ffmpeg

#endif // VIDEOOUTPUTRENDER_HPP
