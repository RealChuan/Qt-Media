#ifndef ENCODECONTEXT_HPP
#define ENCODECONTEXT_HPP

#include "ffmpegutils.hpp"

#include <QtCore>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
}

struct AVStream;

namespace Ffmpeg {

namespace EncodeLimit {

static const int crf_min = 0;
static const int crf_max = 51;
static const int invalid_crf = -999;

static const QStringList presets = QStringList{"ultrafast",
                                               "superfast",
                                               "veryfast",
                                               "faster",
                                               "fast",
                                               "medium",
                                               "slow",
                                               "slower",
                                               "veryslow",
                                               "placebo"};
static const QStringList tunes = QStringList{"film",
                                             "animation",
                                             "grain",
                                             "stillimage",
                                             "psnr",
                                             "ssim",
                                             "fastdecode",
                                             "zerolatency"};

} // namespace EncodeLimit
struct FFMPEG_EXPORT EncodeContext
{
    EncodeContext() = default;
    explicit EncodeContext(AVStream *stream, AVContextInfo *info);

    auto setEncoderName(AVCodecID codecId) -> bool;
    auto setEncoderName(const QString &name) -> bool;
    [[nodiscard]] auto codecInfo() const -> CodecInfo { return m_codecInfo; }

    void setChannel(AVChannel channel);
    [[nodiscard]] auto chLayout() const -> ChLayout { return m_chLayout; }

    void setProfile(int profile);
    [[nodiscard]] auto profile() const -> AVProfile { return m_profile; }

    QString sourceInfo;

    int streamIndex = -1;
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;

    qint64 minBitrate = -1;
    qint64 maxBitrate = -1;
    qint64 bitrate = -1;

    int threadCount = -1;
    int crf = 35;

    // video
    QSize size = {-1, -1};
    QString preset = "slow";
    QString tune = "film";

    // audio
    int sampleRate = -1;

    // subtitle
    bool burn = false;
    bool external = false;

    QList<AVProfile> profiles;
    ChLayouts chLayouts;
    QList<int> sampleRates;

private:
    void init(AVContextInfo *info);

    CodecInfo m_codecInfo;
    ChLayout m_chLayout;
    AVProfile m_profile;
};

using EncodeContexts = QList<EncodeContext>;

} // namespace Ffmpeg

Q_DECLARE_METATYPE(Ffmpeg::EncodeContext);

#endif // ENCODECONTEXT_HPP
