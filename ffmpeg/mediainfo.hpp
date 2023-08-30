#pragma once

#include "ffmepg_global.h"

#include <QImage>
#include <QtCore>

extern "C" {
#include <libavutil/avutil.h>
}

struct AVStream;

namespace Ffmpeg {

enum MediaState { Stopped, Opening, Playing, Pausing };

struct FFMPEG_EXPORT StreamInfo
{
    void parseStreamData(struct AVStream *stream);

    [[nodiscard]] auto info() const -> QString;

    int index = 0;
    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    QString lang;
    QString title;
    qint64 NUMBER_OF_BYTES = 0;
    QString codec;
    qint64 bitRate = -1;
    // Video
    double fps = 0;
    int width = 0;
    int height = 0;
    // Audio
    int channels = 0;
    int sampleRate = 0;
    // 封面
    QImage image;

    bool defaultSelected = false;
    bool selected = false;
};

} // namespace Ffmpeg
