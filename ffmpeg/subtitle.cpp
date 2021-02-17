#include "subtitle.h"

#include <QDebug>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

class SubtitlePrivate{
public:
    SubtitlePrivate(QObject *parent)
        : owner(parent){
    }

    ~SubtitlePrivate(){
    }

    void freeSubtitle(){
        avsubtitle_free(&subtitle);
    }

    QObject *owner;
    AVSubtitle subtitle;
    double pts = 0;
    double duration = 0;
    QString text;
};

Subtitle::Subtitle(QObject *parent)
    : QObject(parent)
    , d_ptr(new SubtitlePrivate(this))
{

}

Subtitle::~Subtitle()
{

}

void Subtitle::setdefault(double pts, double duration, const QString &text)
{
    d_ptr->pts = pts;
    d_ptr->duration = duration;
    d_ptr->text = text;
}

QVector<SubtitleImage> Subtitle::subtitleImages()
{
    switch (d_ptr->subtitle.format) {
    case 0: return scale();
    default: return text();
    }
}

QVector<SubtitleImage> Subtitle::scale()
{
    QVector<SubtitleImage> subtitleImages;
    for (size_t i = 0; i < d_ptr->subtitle.num_rects; i++) {
        AVSubtitleRect *sub_rect = d_ptr->subtitle.rects[i];

        int dst_linesize[4];
        uint8_t *dst_data[4];
        //注意，这里是RGBA格式，需要Alpha
        av_image_alloc(dst_data, dst_linesize, sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA, 1);
        SwsContext *swsContext = sws_getContext(sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                                                sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR, nullptr, nullptr, nullptr);
        sws_scale(swsContext, sub_rect->data, sub_rect->linesize, 0, sub_rect->h, dst_data, dst_linesize);
        sws_freeContext(swsContext);
        //这里也使用RGBA
        QImage image = QImage(dst_data[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888).copy();
        av_freep(&dst_data[0]);

        subtitleImages.append(SubtitleImage{QRectF(sub_rect->x, sub_rect->y, sub_rect->w, sub_rect->h),
                                            image,
                                            "",
                                            d_ptr->subtitle.start_display_time,
                                            d_ptr->subtitle.end_display_time});
    }
    return subtitleImages;
}

QVector<SubtitleImage> Subtitle::text()
{
    QVector<SubtitleImage> subtitleImages;
    for (size_t i = 0; i < d_ptr->subtitle.num_rects; i++) {
        AVSubtitleRect *sub_rect = d_ptr->subtitle.rects[i];
        QString str;
        switch (sub_rect->type) {
        case AVSubtitleType::SUBTITLE_TEXT: str = QString::fromUtf8(sub_rect->text); break;
        case AVSubtitleType::SUBTITLE_ASS: str = sub_rect->ass; break;
        default: continue;
        }
        qDebug() << "Subtitle::text" << str;
        //    subtitleImages.append(SubtitleImage{QRectF(sub_rect->x, sub_rect->y, sub_rect->w, sub_rect->h),
        //                                        QImage(),
        //                                        str,
        //                                        qint64(d_ptr->pts * 1000),
        //                                        qint64((d_ptr->pts + d_ptr->duration) * 1000)});
        //}
    }
    subtitleImages.append(SubtitleImage{QRectF(),
                                        QImage(),
                                        d_ptr->text,
                                        qint64(d_ptr->pts * 1000),
                                        qint64((d_ptr->pts + d_ptr->duration) * 1000)});
    return subtitleImages;
}

AVSubtitle *Subtitle::avSubtitle()
{
    return &d_ptr->subtitle;
}

void Subtitle::clear()
{
    return d_ptr->freeSubtitle();
}

}
