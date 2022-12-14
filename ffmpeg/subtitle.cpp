#include "subtitle.h"

#include <subtitle/ass.hpp>

#include <QDebug>
#include <QImage>
#include <QPainter>

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
    Subtitle::Type type = Subtitle::Unknown;
    QSize videoResolutionRatio = QSize(1280, 720);

    QByteArrayList texts;
    AssDataInfoList assList;
    QImage image;
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
        d_ptr->type = Subtitle::Graphics;
        parseImage(swsContext);
        break;
    default: parseText(); break;
    }
}

QByteArrayList Subtitle::texts() const
{
    return d_ptr->texts;
}

void Subtitle::parseImage(SwsContext *swsContext)
{
    d_ptr->image = QImage(d_ptr->videoResolutionRatio, QImage::Format_RGBA8888);
    d_ptr->image.fill(Qt::transparent);
    QPainter painter(&d_ptr->image);
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
        auto image = QImage(pixels[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888);
        if (!image.isNull()) {
            painter.drawImage(QRect(sub_rect->x, sub_rect->y, sub_rect->w, sub_rect->h), image);
        }
        av_freep(&pixels[0]);
    }
    d_ptr->pts = d_ptr->subtitle.start_display_time / 1000;
    d_ptr->duration = (d_ptr->subtitle.end_display_time - d_ptr->subtitle.start_display_time)
                      / 1000;
}

void Subtitle::parseText()
{
    for (size_t i = 0; i < d_ptr->subtitle.num_rects; i++) {
        AVSubtitleRect *sub_rect = d_ptr->subtitle.rects[i];
        QByteArray text;
        switch (sub_rect->type) {
        case AVSubtitleType::SUBTITLE_TEXT:
            d_ptr->type = Subtitle::TEXT;
            text = sub_rect->text;
            break;
        case AVSubtitleType::SUBTITLE_ASS:
            d_ptr->type = Subtitle::ASS;
            text = sub_rect->ass;
            break;
        default: continue;
        }
        d_ptr->texts.append(text);
        //qDebug() << "Subtitle Type:" << sub_rect->type << QString::fromUtf8(text);
    }
}

AVSubtitle *Subtitle::avSubtitle()
{
    return &d_ptr->subtitle;
}

void Subtitle::clear()
{
    return d_ptr->freeSubtitle();
}

double Subtitle::pts()
{
    return d_ptr->pts;
}

double Subtitle::duration()
{
    return d_ptr->duration;
}

Subtitle::Type Subtitle::type() const
{
    return d_ptr->type;
}

void Subtitle::setVideoResolutionRatio(const QSize &size)
{
    if (!size.isValid()) {
        return;
    }
    d_ptr->videoResolutionRatio = size;
}

QSize Subtitle::videoResolutionRatio() const
{
    return d_ptr->videoResolutionRatio;
}

bool Subtitle::resolveAss(Ass *ass)
{
    if (d_ptr->type != Type::ASS) {
        return false;
    }
    for (const auto &data : qAsConst(d_ptr->texts)) {
        ass->addSubtitleData(data);
    }
    ass->getRGBAData(d_ptr->assList, d_ptr->pts);
    return true;
}

void Subtitle::setAssDataInfoList(const AssDataInfoList &list)
{
    d_ptr->assList = list;
}

AssDataInfoList Subtitle::list() const
{
    return d_ptr->assList;
}

QImage Subtitle::generateImage() const
{
    if (d_ptr->type != Type::ASS) {
        return QImage();
    }
    d_ptr->image = QImage(d_ptr->videoResolutionRatio, QImage::Format_RGBA8888);
    d_ptr->image.fill(Qt::transparent);
    QPainter painter(&d_ptr->image);
    for (const auto &data : qAsConst(d_ptr->assList)) {
        auto rect = data.rect();
        QImage image((uchar *) data.rgba().constData(),
                     rect.width(),
                     rect.height(),
                     QImage::Format_RGBA8888);
        if (image.isNull()) {
            qWarning() << "image is null";
            continue;
        }
        painter.drawImage(rect, image);
    }

    return d_ptr->image;
}

QImage Subtitle::image() const
{
    return d_ptr->image;
}

} // namespace Ffmpeg
