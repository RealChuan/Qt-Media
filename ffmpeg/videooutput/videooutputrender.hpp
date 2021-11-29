#ifndef VIDEOOUTPUTRENDER_HPP
#define VIDEOOUTPUTRENDER_HPP

#include <QtCore/QObject>

#include <ffmpeg/ffmepg_global.h>
#include <ffmpeg/subtitle.h>

namespace Ffmpeg {

class FFMPEG_EXPORT VideoOutputRender
{
    Q_DISABLE_COPY(VideoOutputRender)
public:
    explicit VideoOutputRender() {}

    virtual void onReadyRead(const QImage &image) = 0;
    virtual void onSubtitleImages(const QVector<Ffmpeg::SubtitleImage> &) = 0;
    void onFinish();

protected:
    void drawBackGround(QPainter &painter, const QRect &rect);
    void drawVideoImage(QPainter &painter, const QRect &rect);
    void checkSubtitle();

    QImage m_image;
    QVector<SubtitleImage> m_subtitleImages;
};

} // namespace Ffmpeg

#endif // VIDEOOUTPUTRENDER_HPP
