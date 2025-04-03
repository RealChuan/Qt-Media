#include "formatcontext.h"
#include "averrormanager.hpp"
#include "ffmpegutils.hpp"
#include "packet.h"

#include <QDebug>
#include <QImage>
#include <QTime>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

static void queryStreamInfo(int bestIndex, StreamInfo &streamInfo, StreamInfos &streamInfos)
{
    if (bestIndex == streamInfo.index) {
        streamInfo.defaultSelected = true;
        streamInfo.selected = true;
    }
    streamInfos.append(streamInfo);
}

class FormatContext::FormatContextPrivate
{
public:
    explicit FormatContextPrivate(FormatContext *q)
        : q_ptr(q)
    {
        avformat_network_init();
    }

    ~FormatContextPrivate() = default;

    void initStreamInfo()
    {
        audioTracks.clear();
        videoTracks.clear();
        subtitleTracks.clear();
        attachmentTracks.clear();

        const auto bestAudioIndex = findBestStreamIndex(AVMEDIA_TYPE_AUDIO);
        const auto bestVideoIndex = findBestStreamIndex(AVMEDIA_TYPE_VIDEO);
        const auto bestSubtitleIndex = findBestStreamIndex(AVMEDIA_TYPE_SUBTITLE);
        const auto bestAttachmentIndex = findBestStreamIndex(AVMEDIA_TYPE_ATTACHMENT);

        //nb_streams视音频流的个数
        for (uint i = 0; i < formatCtx->nb_streams; i++) {
            StreamInfo streamInfo(formatCtx->streams[i]);
            streamInfo.index = i;

            switch (formatCtx->streams[i]->codecpar->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                queryStreamInfo(bestAudioIndex, streamInfo, audioTracks);
                break;
            case AVMEDIA_TYPE_VIDEO:
                queryStreamInfo(bestVideoIndex, streamInfo, videoTracks);
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                queryStreamInfo(bestSubtitleIndex, streamInfo, subtitleTracks);
                break;
            case AVMEDIA_TYPE_ATTACHMENT:
                queryStreamInfo(bestAttachmentIndex, streamInfo, attachmentTracks);
                break;
            default: break;
            }
        }
    }

    [[nodiscard]] auto findBestStreamIndex(AVMediaType type) const -> int
    {
        Q_ASSERT(formatCtx != nullptr);
        return av_find_best_stream(formatCtx, type, -1, -1, nullptr, 0);
    }

    FormatContext *q_ptr;

    AVFormatContext *formatCtx = nullptr;
    QString filepath;
    FormatContext::OpenMode mode = FormatContext::ReadOnly;
    bool isOpen = false;

    StreamInfos audioTracks;
    StreamInfos videoTracks;
    StreamInfos subtitleTracks;
    StreamInfos attachmentTracks;

    const qint64 seekOffset = 2 * AV_TIME_BASE;
};

FormatContext::FormatContext(QObject *parent)
    : QObject(parent)
    , d_ptr(new FormatContextPrivate(this))
{}

FormatContext::~FormatContext()
{
    close();
}

void FormatContext::copyChapterFrom(FormatContext *src)
{
    auto *is = src->avFormatContext();
    auto *os = d_ptr->formatCtx;
    auto **tmp = static_cast<AVChapter **>(
        av_realloc_f(os->chapters, is->nb_chapters + os->nb_chapters, sizeof(*os->chapters)));
    if (tmp == nullptr) {
        qWarning() << tr("Memory request error");
        return;
    }
    os->chapters = tmp;

    for (uint i = 0; i < is->nb_chapters; i++) {
        auto *in_ch = is->chapters[i];
        AVChapter *out_ch;
        int64_t start_time = 0;
        int64_t ts_off = av_rescale_q(start_time, AVRational{1, AV_TIME_BASE}, in_ch->time_base);
        int64_t rt = INT64_MAX;

        if (in_ch->end < ts_off) {
            continue;
        }
        if (rt != INT64_MAX && in_ch->start > rt + ts_off) {
            break;
        }
        out_ch = static_cast<AVChapter *>(av_mallocz(sizeof(AVChapter)));
        if (out_ch == nullptr) {
            qWarning() << tr("Memory request error");
            return;
        }
        out_ch->id = in_ch->id;
        out_ch->time_base = in_ch->time_base;
        out_ch->start = FFMAX(0, in_ch->start - ts_off);
        out_ch->end = FFMIN(rt, in_ch->end - ts_off);

        av_dict_copy(&out_ch->metadata, in_ch->metadata, 0);

        os->chapters[os->nb_chapters++] = out_ch;
    }
}

auto FormatContext::isOpen() -> bool
{
    return d_ptr->isOpen;
}

auto FormatContext::openFilePath(const QString &filepath, OpenMode mode) -> bool
{
    d_ptr->mode = mode;
    close();
    d_ptr->filepath = filepath;

    auto inpuUrl = convertUrlToFfmpegInput(d_ptr->filepath);
    switch (mode) {
    case ReadOnly: {
        auto ret = avformat_open_input(&d_ptr->formatCtx, inpuUrl.constData(), nullptr, nullptr);
        if (ret != 0) {
            SET_ERROR_CODE(ret);
            return false;
        }
        av_format_inject_global_side_data(d_ptr->formatCtx);
        d_ptr->isOpen = true;
    } break;
    case WriteOnly: {
        avformat_free_context(d_ptr->formatCtx);
        auto ret = avformat_alloc_output_context2(&d_ptr->formatCtx,
                                                  nullptr,
                                                  nullptr,
                                                  inpuUrl.constData());
        if (ret < 0) {
            SET_ERROR_CODE(ret);
            return false;
        }
        d_ptr->isOpen = true;
    } break;
    default: break;
    }
    return d_ptr->isOpen;
}

void FormatContext::close()
{
    if (!d_ptr->isOpen) {
        return;
    }
    switch (d_ptr->mode) {
    case ReadOnly:
        avformat_close_input(&d_ptr->formatCtx);
        d_ptr->formatCtx = nullptr;
        d_ptr->isOpen = false;
        break;
    case WriteOnly: avioClose(); break;
    default: break;
    }
}

auto FormatContext::avioOpen() -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    if ((d_ptr->formatCtx->oformat->flags & AVFMT_NOFILE) != 0) {
        return false;
    }
    auto inpuUrl = convertUrlToFfmpegInput(d_ptr->filepath);
    auto ret = ::avio_open(&d_ptr->formatCtx->pb, inpuUrl.constData(), AVIO_FLAG_WRITE);
    ERROR_RETURN(ret)
}

void FormatContext::avioClose()
{
    if (!d_ptr->isOpen && d_ptr->mode != WriteOnly) {
        return;
    }
    if ((d_ptr->formatCtx != nullptr) && ((d_ptr->formatCtx->oformat->flags & AVFMT_NOFILE) == 0)) {
        avio_closep(&d_ptr->formatCtx->pb);
    }
    avformat_free_context(d_ptr->formatCtx);
    d_ptr->formatCtx = nullptr;
    d_ptr->isOpen = false;
}

auto FormatContext::writeHeader() -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto ret = avformat_write_header(d_ptr->formatCtx, nullptr);
    ERROR_RETURN(ret)
}

auto FormatContext::writePacket(Packet *packet) -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto ret = av_interleaved_write_frame(d_ptr->formatCtx, packet->avPacket());
    ERROR_RETURN(ret)
}

auto FormatContext::writeTrailer() -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto ret = av_write_trailer(d_ptr->formatCtx);
    ERROR_RETURN(ret)
}

auto FormatContext::findStream() -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    //获取音视频流数据信息
    int ret = avformat_find_stream_info(d_ptr->formatCtx, nullptr);
    if (ret < 0) {
        SET_ERROR_CODE(ret);
        return false;
    }
    if (d_ptr->formatCtx->pb != nullptr) {
        // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
        d_ptr->formatCtx->pb->eof_reached = 0;
    }
    d_ptr->initStreamInfo();
    return true;
}

auto FormatContext::streams() const -> int
{
    return static_cast<int>(d_ptr->formatCtx->nb_streams);
}

auto FormatContext::audioTracks() const -> StreamInfos
{
    return d_ptr->audioTracks;
}

auto FormatContext::videoTracks() const -> StreamInfos
{
    return d_ptr->videoTracks;
}

auto FormatContext::subtitleTracks() const -> StreamInfos
{
    return d_ptr->subtitleTracks;
}

auto FormatContext::attachmentTracks() const -> StreamInfos
{
    return d_ptr->attachmentTracks;
}

auto FormatContext::findBestStreamIndex(AVMediaType type) const -> int
{
    return d_ptr->findBestStreamIndex(type);
}

void FormatContext::discardStreamExcluded(const QList<int> &indexs)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    for (uint i = 0; i < d_ptr->formatCtx->nb_streams; i++) {
        if (indexs.contains(i)) {
            continue;
        }
        d_ptr->formatCtx->streams[i]->discard = AVDISCARD_ALL;
    }
}

auto FormatContext::stream(int index) -> AVStream *
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx->streams[index];
}

auto FormatContext::createStream() -> AVStream *
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto *stream = avformat_new_stream(d_ptr->formatCtx, nullptr);
    if (stream == nullptr) {
        qWarning() << "Failed allocating output stream\n";
    }
    return stream;
}

auto FormatContext::readFrame(Packet *packet) -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_read_frame(d_ptr->formatCtx, packet->avPacket());
    ERROR_RETURN(ret)
}

auto FormatContext::checkPktPlayRange(Packet *packet) -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    auto *avPacket = packet->avPacket();
    /* check if packet is in play range specified by user, then queue, otherwise discard */
    auto start_time = AV_NOPTS_VALUE;
    auto duration = AV_NOPTS_VALUE;
    auto stream_start_time = d_ptr->formatCtx->streams[avPacket->stream_index]->start_time;
    auto pkt_ts = avPacket->pts == AV_NOPTS_VALUE ? avPacket->dts : avPacket->pts;
    auto pkt_in_play_range
        = duration == AV_NOPTS_VALUE
          || (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0))
                         * av_q2d(d_ptr->formatCtx->streams[avPacket->stream_index]->time_base)
                     - static_cast<double>(start_time != AV_NOPTS_VALUE ? start_time : 0)
                           / AV_TIME_BASE
                 <= (static_cast<double>(duration) / AV_TIME_BASE);
    return pkt_in_play_range;
}

auto FormatContext::guessFrameRate(int index) const -> AVRational
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return guessFrameRate(d_ptr->formatCtx->streams[index]);
}

auto FormatContext::guessFrameRate(AVStream *stream) const -> AVRational
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return av_guess_frame_rate(d_ptr->formatCtx, stream, nullptr);
}

auto FormatContext::seekFirstFrame() -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int64_t timestamp = 0;
    if (d_ptr->formatCtx->start_time != AV_NOPTS_VALUE) {
        timestamp = d_ptr->formatCtx->start_time;
    }
    int ret = avformat_seek_file(d_ptr->formatCtx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
    ERROR_RETURN(ret)
}

auto FormatContext::seek(qint64 timestamp) -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    Q_ASSERT(timestamp >= 0);
    auto seekMin = timestamp - d_ptr->seekOffset;
    auto seekMax = timestamp + d_ptr->seekOffset;
    auto ret = avformat_seek_file(d_ptr->formatCtx, -1, seekMin, timestamp, seekMax, 0);
    ERROR_RETURN(ret)
}

auto FormatContext::seek(qint64 timestamp, bool forward) -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    Q_ASSERT(timestamp >= 0);
    auto seekMin = forward ? INT64_MIN : timestamp - d_ptr->seekOffset;
    auto seekMax = forward ? timestamp + d_ptr->seekOffset : INT64_MAX;
    auto ret = avformat_seek_file(d_ptr->formatCtx, -1, seekMin, timestamp, seekMax, 0);
    ERROR_RETURN(ret)
}

auto FormatContext::seekFrame(int index, qint64 timestamp) -> bool
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_seek_frame(d_ptr->formatCtx, index, timestamp, AVSEEK_FLAG_BACKWARD);
    ERROR_RETURN(ret)
}

void FormatContext::dumpFormat()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    av_dump_format(d_ptr->formatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), d_ptr->mode - 1);
}

auto FormatContext::mediaInfo() -> MediaInfo
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return MediaInfo(d_ptr->formatCtx);
}

auto FormatContext::avFormatContext() -> AVFormatContext *
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx;
}

auto FormatContext::duration() const -> qint64
{
    return d_ptr->isOpen ? d_ptr->formatCtx->duration : 0;
}

} // namespace Ffmpeg
