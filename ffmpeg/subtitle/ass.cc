#include "ass.hpp"

#include <QDebug>
#include <QImage>

extern "C" {
#include <ass/ass.h>
}

namespace Ffmpeg {

void msg_callback(int level, const char *fmt, va_list va, void *data)
{
    Q_UNUSED(data)
    if (level > 6) {
        return;
    }
    qInfo() << "libass:" << QString::vasprintf(fmt, va);
}

void print_font_providers(ASS_Library *ass_library)
{
    QMap<ASS_DefaultFontProvider, QString> font_provider_maps
        = {{ASS_FONTPROVIDER_NONE, "None"},
           {ASS_FONTPROVIDER_AUTODETECT, "Autodetect"},
           {ASS_FONTPROVIDER_CORETEXT, "CoreText"},
           {ASS_FONTPROVIDER_FONTCONFIG, "Fontconfig"},
           {ASS_FONTPROVIDER_DIRECTWRITE, "DirectWrite"}};

    ASS_DefaultFontProvider *providers;
    size_t providers_size = 0;
    ass_get_available_font_providers(ass_library, &providers, &providers_size);
    qDebug("test.c: Available font providers (%zu): ", providers_size);
    for (int i = 0; i < int(providers_size); i++) {
        qDebug() << font_provider_maps[providers[i]];
    }
    free(providers);
}

class Ass::AssPrivate
{
public:
    AssPrivate(QObject *parent)
        : owner(parent)
    {
        qInfo() << "ass_library_version: " << ass_library_version();

        print_font_providers(ass_library);
        ass_library = ass_library_init();
        ass_set_message_cb(ass_library, msg_callback, NULL);
        ass_set_extract_fonts(ass_library, 1);

        ass_renderer = ass_renderer_init(ass_library);

        acc_track = ass_new_track(ass_library);
        //        ass_alloc_style(acc_track);
        //        acc_track->styles[0].ScaleX = acc_track->styles[0].ScaleY = 1;

        ass_set_fonts(ass_renderer, NULL, "sans-serif", ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
    }
    ~AssPrivate()
    {
        ass_free_track(acc_track);
        ass_renderer_done(ass_renderer);
        ass_clear_fonts(ass_library);
        ass_library_done(ass_library);
    }

    QObject *owner;

    ASS_Library *ass_library;
    ASS_Renderer *ass_renderer;
    ASS_Track *acc_track;
    QSize size;
};

Ass::Ass(QObject *parent)
    : QObject{parent}
    , d_ptr(new AssPrivate(this))
{}

Ass::~Ass() {}

void Ass::init(uint8_t *extradata, int extradata_size)
{
    ass_process_codec_private(d_ptr->acc_track, (char *) extradata, extradata_size);
    for (int i = 0; i < d_ptr->acc_track->n_events; ++i) {
        d_ptr->acc_track->events[i].ReadOrder = i;
    }
}

void Ass::setWindowSize(const QSize &size)
{
    d_ptr->size = size;
    ass_set_storage_size(d_ptr->ass_renderer, d_ptr->size.width(), d_ptr->size.height());
    ass_set_frame_size(d_ptr->ass_renderer, d_ptr->size.width(), d_ptr->size.height());
}

void Ass::setFont(const QString &fontFamily)
{
    ass_set_fonts(d_ptr->ass_renderer,
                  NULL,
                  fontFamily.toStdString().c_str(),
                  ASS_FONTPROVIDER_AUTODETECT,
                  NULL,
                  1);
}

void Ass::addFont(const QByteArray &name, const QByteArray &data)
{
    ass_add_font(d_ptr->ass_library,
                 (char *) name.constData(),
                 (char *) data.constData(),
                 data.size());
}

void Ass::addSubtitleData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    ass_process_data(d_ptr->acc_track, (char *) data.constData(), data.size());
}

void Ass::addSubtitleData(const QByteArray &data, double pts, double duration)
{
    if (data.isEmpty() || pts < 0 || duration < 0) {
        return;
    }
    int eventID = ass_alloc_event(d_ptr->acc_track);
    ASS_Event *event = &d_ptr->acc_track->events[eventID];
    event->Text = strdup(data.constData());
    event->Start = pts * 1000;
    event->Duration = duration * 1000;
    event->Style = 0;
    event->ReadOrder = eventID;
}

void Ass::getRGBAData(AssDataInfoList &list, double pts)
{
    int ch;
    ASS_Image *img = ass_render_frame(d_ptr->ass_renderer, d_ptr->acc_track, pts * 1000, &ch);
    while (img) {
        auto rect = QRect(img->dst_x, img->dst_y, img->w, img->h);
        auto rgba = QByteArray(img->w * img->h * sizeof(uint32_t), Qt::Uninitialized);

        const quint8 r = img->color >> 24;
        const quint8 g = img->color >> 16;
        const quint8 b = img->color >> 8;
        const quint8 a = ~img->color & 0xFF;

        auto data = reinterpret_cast<uint32_t *>(rgba.data());
        for (int y = 0; y < img->h; y++) {
            const int offsetI = y * img->stride;
            const int offsetB = y * img->w;
            for (int x = 0; x < img->w; x++) {
                data[offsetB + x] = (a * img->bitmap[offsetI + x] / 0xFF) << 24 | b << 16 | g << 8
                                    | r;
            }
        }
        list.append(AssDataInfo(rgba, rect));
        img = img->next;
    }
}

void Ass::flushASSEvents()
{
    ass_flush_events(d_ptr->acc_track);
}

} // namespace Ffmpeg
