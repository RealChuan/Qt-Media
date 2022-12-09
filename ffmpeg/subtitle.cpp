#include "subtitle.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

class Subtitle::SubtitlePrivate
{
public:
    SubtitlePrivate(QObject *parent)
        : owner(parent)
    {}
    ~SubtitlePrivate() { freeSubtitle(); }

    void freeSubtitle() { avsubtitle_free(&subtitle); }

    QObject *owner;
    AVSubtitle subtitle;
    double pts = 0;
    double duration = 0;
    QString text;

    SwsContext *swsContext;
    bool isText;

    QVector<QImage> images;
    QVector<QRect> rect;
    QByteArrayList texts;
};

Subtitle::Subtitle(QObject *parent)
    : QObject(parent)
    , d_ptr(new SubtitlePrivate(this))
{}

Subtitle::~Subtitle() {}

void Subtitle::setDefault(double pts, double duration, const QString &text)
{
    d_ptr->pts = pts;
    d_ptr->duration = duration;
    d_ptr->text = text;
}

void Subtitle::parse(SwsContext *swsContext)
{
    switch (d_ptr->subtitle.format) {
    case 0:
        d_ptr->isText = true;
        parseImage(swsContext);
        break;
    default:
        d_ptr->isText = false;
        parseText();
        break;
    }
}

QByteArrayList Subtitle::texts() const
{
    return d_ptr->texts;
}

void Subtitle::parseImage(SwsContext *swsContext)
{
    for (size_t i = 0; i < d_ptr->subtitle.num_rects; i++) {
        auto sub_rect = d_ptr->subtitle.rects[i];

        uint8_t *pixels[4];
        int pitch[4];
        //注意，这里是RGBA格式，需要Alpha
        av_image_alloc(pixels, pitch, sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA, 1);
        swsContext = sws_getCachedContext(swsContext,
                                          sub_rect->w,
                                          sub_rect->h,
                                          AV_PIX_FMT_PAL8,
                                          sub_rect->w,
                                          sub_rect->h,
                                          AV_PIX_FMT_RGBA,
                                          SWS_BILINEAR,
                                          nullptr,
                                          nullptr,
                                          nullptr);
        sws_scale(d_ptr->swsContext,
                  sub_rect->data,
                  sub_rect->linesize,
                  0,
                  sub_rect->h,
                  pixels,
                  pitch);
        //这里也使用RGBA
        auto image = QImage(pixels[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888).copy();
        av_freep(&pixels[0]);

        d_ptr->images.append(image);
        d_ptr->rect.append({sub_rect->x, sub_rect->y, sub_rect->w, sub_rect->h});
    }
    d_ptr->subtitle.start_display_time /= 1000;
    d_ptr->subtitle.end_display_time /= 1000;
}

void Subtitle::parseText()
{
    for (size_t i = 0; i < d_ptr->subtitle.num_rects; i++) {
        AVSubtitleRect *sub_rect = d_ptr->subtitle.rects[i];
        QByteArray text;
        switch (sub_rect->type) {
        case AVSubtitleType::SUBTITLE_TEXT: text = sub_rect->text; break;
        case AVSubtitleType::SUBTITLE_ASS: text = sub_rect->ass; break;
        default: continue;
        }
        //qDebug() << "Subtitle Type:" << sub_rect->type << QString::fromUtf8(text);
        d_ptr->texts.append(text);
    }
    d_ptr->subtitle.start_display_time = d_ptr->pts;
    d_ptr->subtitle.end_display_time = d_ptr->pts + d_ptr->duration;
}

AVSubtitle *Subtitle::avSubtitle()
{
    return &d_ptr->subtitle;
}

void Subtitle::clear()
{
    return d_ptr->freeSubtitle();
}

quint64 Subtitle::pts()
{
    return d_ptr->subtitle.start_display_time;
}

quint64 Subtitle::duration()
{
    return d_ptr->subtitle.end_display_time - d_ptr->subtitle.start_display_time;
}

} // namespace Ffmpeg
