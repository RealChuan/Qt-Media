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
    switch (level) {
    case 0: qCritical() << "libass:" << QString::vasprintf(fmt, va); break;
    case 1:
    case 2:
    case 3: qWarning() << "libass:" << QString::vasprintf(fmt, va); break;
    case 4:
    case 5:
    case 6: qInfo() << "libass:" << QString::vasprintf(fmt, va); break;
    case 7:
    default: qDebug() << "libass:" << QString::vasprintf(fmt, va); break;
    }
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
    qDebug("Available font providers (%zu): ", providers_size);
    for (int i = 0; i < static_cast<int>(providers_size); i++) {
        qDebug() << font_provider_maps[providers[i]];
    }
    free(providers);
}

class Ass::AssPrivate
{
public:
    explicit AssPrivate(Ass *q)
        : q_ptr(q)
    {
        qInfo() << "ass_library_version: " << ass_library_version();

        ass_library = ass_library_init();
        print_font_providers(ass_library);
        ass_set_message_cb(ass_library, msg_callback, nullptr);
        ass_set_extract_fonts(ass_library, 1);

        ass_renderer = ass_renderer_init(ass_library);

        acc_track = ass_new_track(ass_library);
        //        ass_alloc_style(acc_track);
        //        acc_track->styles[0].ScaleX = acc_track->styles[0].ScaleY = 1;

        /* Initialize fonts */
        ass_set_fonts(ass_renderer, nullptr, nullptr, 1, nullptr, 1);
    }
    ~AssPrivate()
    {
        ass_free_track(acc_track);
        ass_renderer_done(ass_renderer);
        ass_clear_fonts(ass_library);
        ass_library_done(ass_library);
    }

    Ass *q_ptr;

    ASS_Library *ass_library;
    ASS_Renderer *ass_renderer;
    ASS_Track *acc_track;
    QSize size;

    const int microToMillon = 1000;
};

Ass::Ass(QObject *parent)
    : QObject{parent}
    , d_ptr(new AssPrivate(this))
{}

Ass::~Ass() = default;

void Ass::init(uint8_t *extradata, int extradata_size)
{
    ass_process_codec_private(d_ptr->acc_track, reinterpret_cast<char *>(extradata), extradata_size);
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
                  nullptr,
                  fontFamily.toStdString().c_str(),
                  ASS_FONTPROVIDER_AUTODETECT,
                  nullptr,
                  1);
}

void Ass::addFont(const QByteArray &name, const QByteArray &data)
{
    ass_add_font(d_ptr->ass_library,
                 const_cast<char *>(name.constData()),
                 const_cast<char *>(data.constData()),
                 static_cast<int>(data.size()));
}

void Ass::addSubtitleEvent(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    ass_process_data(d_ptr->acc_track,
                     const_cast<char *>(data.constData()),
                     static_cast<int>(data.size()));
}

void Ass::addSubtitleEvent(const QByteArray &data, qint64 pts, qint64 duration)
{
    if (data.isEmpty() || pts < 0 || duration < 0) {
        return;
    }
    int eventID = ass_alloc_event(d_ptr->acc_track);
    ASS_Event *event = &d_ptr->acc_track->events[eventID];
    event->Text = strdup(data.constData());
    event->Start = pts / d_ptr->microToMillon;
    event->Duration = duration / d_ptr->microToMillon;
    event->Style = 0;
    event->ReadOrder = eventID;
}

void Ass::addSubtitleChunk(const QByteArray &data, qint64 pts, qint64 duration)
{
    ass_process_chunk(d_ptr->acc_track,
                      const_cast<char *>(data.constData()),
                      static_cast<int>(data.size()),
                      pts / d_ptr->microToMillon,
                      duration / d_ptr->microToMillon);
}

void Ass::getRGBAData(AssDataInfoList &list, qint64 pts)
{
    int ch;
    auto *img = ass_render_frame(d_ptr->ass_renderer,
                                 d_ptr->acc_track,
                                 pts / d_ptr->microToMillon,
                                 &ch);
    while (img != nullptr) {
        auto rect = QRect(img->dst_x, img->dst_y, img->w, img->h);
        auto rgba = QByteArray(static_cast<qsizetype>(img->w) * img->h * sizeof(uint32_t),
                               Qt::Uninitialized);

        const quint8 r = img->color >> 24;
        const quint8 g = img->color >> 16;
        const quint8 b = img->color >> 8;
        const quint8 a = ~img->color & 0xFF;

        auto *data = reinterpret_cast<uint32_t *>(rgba.data());
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
