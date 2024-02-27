#pragma once

#include "ffmepg_global.h"
#include "ffmpegutils.hpp"

#include <QImage>
#include <QtCore>

extern "C" {
#include <libavutil/avutil.h>
}

struct AVStream;
struct AVFormatContext;
struct AVChapter;

namespace Ffmpeg {

enum MediaState { Stopped, Opening, Playing, Pausing };

struct FFMPEG_EXPORT StreamInfo
{
    StreamInfo() = default;
    explicit StreamInfo(struct AVStream *stream);

    [[nodiscard]] auto toJson() const -> QJsonObject;
    [[nodiscard]] auto info() const -> QString;

    int index = 0;
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    QString mediaTypeText;
    QString startTime;
    QString duration;
    AVCodecID codecId = AV_CODEC_ID_NONE;
    QString codecName;
    int nbFrames = 0;
    QString format;
    qint64 bitRate = 0;
    qint64 streamSize = 0;
    QString profile;
    int level = 0;

    // Audio
    QString chLayout;
    int sampleRate = 0;
    int frameSize = 0;
    // Video
    double frameRate = 0;
    QString aspectRatio = nullptr;
    QSize size{0, 0};
    QString colorRange;
    QString colorPrimaries;
    QString colorTrc;
    QString colorSpace;
    QString chromaLocation;
    int videoDelay = 0;

    // 封面
    QImage image;

    bool defaultSelected = false;
    bool selected = false;

    Metadatas metadatas;
};

using StreamInfos = QVector<StreamInfo>;

struct FFMPEG_EXPORT Chapter
{
    Chapter() = default;
    explicit Chapter(AVChapter *chapter);

    [[nodiscard]] auto toJson() const -> QJsonObject;

    qint64 id = 0;
    double timeBase = 0;
    qint64 startTime = 0; // second
    qint64 endTime = 0;   // second
    QString startTimeText;
    QString endTimeText;

    Metadatas metadatas;
};

using Chapters = QVector<Chapter>;

struct FFMPEG_EXPORT MediaInfo
{
    MediaInfo() = default;
    explicit MediaInfo(AVFormatContext *formatCtx);

    [[nodiscard]] auto toJson() const -> QJsonObject;

    QString name;
    QString longName;
    QString url;
    qint64 startTime = 0; // microsecond
    QString startTimeText;
    qint64 duration = 0; // microsecond
    QString durationText;
    qint64 bitRate = 0;
    qint64 size = 0;
    Metadatas metadatas;

    Chapters chapters;
    StreamInfos streamInfos;
};

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
