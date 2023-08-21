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
    explicit SubtitlePrivate(Subtitle *q)
        : q_ptr(q)
    {}
    ~SubtitlePrivate() { freeSubtitle(); }

    void freeSubtitle() { avsubtitle_free(&subtitle); }

    void parseImage(SwsContext *swsContext)
    {
        image = QImage(videoResolutionRatio, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        for (size_t i = 0; i < subtitle.num_rects; i++) {
            auto sub_rect = subtitle.rects[i];

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
            sws_scale(swsContext, sub_rect->data, sub_rect->linesize, 0, sub_rect->h, pixels, pitch);
            //这里也使用RGBA
            auto image = QImage(pixels[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888);
            if (!image.isNull()) {
                painter.drawImage(QRect(sub_rect->x, sub_rect->y, sub_rect->w, sub_rect->h), image);
            }
            av_freep(&pixels[0]);
        }
        pts = subtitle.start_display_time / 1000.0;
        duration = (subtitle.end_display_time - subtitle.start_display_time) / 1000.0;
    }

    void parseText()
    {
        pts = QString::asprintf("%.2f", pts).toDouble(); // libass只支持0.01秒，还要四舍五入
        for (size_t i = 0; i < subtitle.num_rects; i++) {
            auto sub_rect = subtitle.rects[i];
            QByteArray text;
            switch (sub_rect->type) {
            case AVSubtitleType::SUBTITLE_TEXT:
                type = Subtitle::TEXT;
                text = sub_rect->text;
                break;
            case AVSubtitleType::SUBTITLE_ASS:
                type = Subtitle::ASS;
                text = sub_rect->ass;
                break;
            default: continue;
            }
            texts.append(text);
            //qDebug() << "Subtitle Type:" << sub_rect->type << QString::fromUtf8(text);
        }
    }

    Subtitle *q_ptr;

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

Subtitle::~Subtitle() = default;

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
        d_ptr->parseImage(swsContext);
        break;
    default: d_ptr->parseText(); break;
    }
}

auto Subtitle::texts() const -> QByteArrayList
{
    return d_ptr->texts;
}

auto Subtitle::avSubtitle() -> AVSubtitle *
{
    return &d_ptr->subtitle;
}

void Subtitle::clear()
{
    return d_ptr->freeSubtitle();
}

auto Subtitle::pts() -> double
{
    return d_ptr->pts;
}

auto Subtitle::duration() -> double
{
    return d_ptr->duration;
}

auto Subtitle::type() const -> Subtitle::Type
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

auto Subtitle::videoResolutionRatio() const -> QSize
{
    return d_ptr->videoResolutionRatio;
}

auto Subtitle::resolveAss(Ass *ass) -> bool
{
    if (d_ptr->type != Type::ASS) {
        return false;
    }
    for (const auto &data : qAsConst(d_ptr->texts)) {
        if (data.startsWith("Dialogue")) {
            ass->addSubtitleEvent(data);
        } else {
            ass->addSubtitleChunk(data, d_ptr->pts, d_ptr->duration);
        }
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

auto Subtitle::generateImage() const -> QImage
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

auto Subtitle::image() const -> QImage
{
    return d_ptr->image;
}

} // namespace Ffmpeg
