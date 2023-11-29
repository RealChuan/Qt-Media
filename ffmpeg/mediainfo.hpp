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
    StreamInfo() = default;
    explicit StreamInfo(struct AVStream *stream);

    [[nodiscard]] auto info() const -> QString;

    int index = 0;
    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    QString typeStr;
    QString startTime;
    QString duration;
    QString codec;
    int nbFrames = 0;
    QString format;
    qint64 bitRate = 0;

    // Audio
    QString chLayout;
    int sampleRate = 0;
    int frameSize = 0;
    // Video
    double frameRate = 0;
    double aspectRatio = 0;
    QSize size{0, 0};
    QString colorRange;
    QString colorPrimaries;
    QString colorTrc;
    QString colorSpace;
    QString chromaLocation;
    int videoDelay = 0;

    // 封面
    QImage image;
    // DictionaryEntry
    QString lang;
    QString title;

    bool defaultSelected = false;
    bool selected = false;
};

using StreamInfos = QVector<StreamInfo>;

struct MediaIndex
{
    void resetIndex()
    {
        audioindex = -1;
        videoindex = -1;
        subtitleindex = -1;
    }

    [[nodiscard]] auto isIndexVaild() const -> bool
    {
        return audioindex >= 0 || videoindex >= 0 || subtitleindex >= 0;
    }

    int audioindex = -1;
    int videoindex = -1;
    int subtitleindex = -1;
};

} // namespace Ffmpeg
