#include "formatcontext.h"
#include "averrormanager.hpp"
#include "packet.h"

#include <QDebug>
#include <QImage>
#include <QTime>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class FormatContext::FormatContextPrivate
{
public:
    FormatContextPrivate(QObject *parent)
        : owner(parent)
    {
        avformat_network_init();
    }

    ~FormatContextPrivate() {}

    void findStreamIndex()
    {
        videoIndexs.clear();
        audioMap.clear();
        subtitleMap.clear();
        coverImage = QImage();

        //nb_streams视音频流的个数
        for (uint i = 0; i < formatCtx->nb_streams; i++) {
            switch (formatCtx->streams[i]->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO: videoIndexs.append(i); break;
            case AVMEDIA_TYPE_AUDIO: {
                AVDictionaryEntry *tag = nullptr;
                QString str;
                while (nullptr
                       != (tag = av_dict_get(formatCtx->streams[i]->metadata,
                                             "",
                                             tag,
                                             AV_DICT_IGNORE_SUFFIX))) {
                    str = str + tag->key + ":" + QString::fromUtf8(tag->value) + " ";
                    //qDebug() << tag->key << ":" << QString::fromUtf8(tag->value);
                }
                str.chop(1);
                audioMap.insert(i, str);
            } break;
            case AVMEDIA_TYPE_SUBTITLE: {
                QString str;
                AVDictionaryEntry *tag = nullptr;
                while (nullptr
                       != (tag = av_dict_get(formatCtx->streams[i]->metadata,
                                             "",
                                             tag,
                                             AV_DICT_IGNORE_SUFFIX))) {
                    str = str + tag->key + ":" + QString::fromUtf8(tag->value) + " ";
                    //qDebug() << tag->key << ":" << QString::fromUtf8(tag->value);
                }
                str.chop(1);
                subtitleMap.insert(i, str);
            } break;
            default: break;
            }

            if (formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket &pkt = formatCtx->streams[i]->attached_pic;
                coverImage = QImage::fromData((uchar *) pkt.data, pkt.size);
            }
        }
    }

    void initMetaData()
    {
        AVDictionaryEntry *tag = nullptr;
        while (nullptr != (tag = av_dict_get(formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            qDebug() << tag->key << QString::fromUtf8(tag->value);
        }
    }

    void printInformation()
    {
        qInfo() << tr("AV Format Name: ") << formatCtx->iformat->name;
        QTime time(QTime::fromMSecsSinceStartOfDay(formatCtx->duration / 1000));
        qInfo() << tr("Duration:") << time.toString("hh:mm:ss.zzz");

        if (formatCtx->pb) {
            // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
            formatCtx->pb->eof_reached = 0;
        }
    }

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    QObject *owner;
    AVFormatContext *formatCtx = nullptr;
    QString filepath;
    FormatContext::OpenMode mode = FormatContext::ReadOnly;
    bool isOpen = false;

    QVector<int> videoIndexs;
    QMap<int, QString> audioMap;
    QMap<int, QString> subtitleMap;
    QImage coverImage;
};

FormatContext::FormatContext(QObject *parent)
    : QObject(parent)
    , d_ptr(new FormatContextPrivate(this))
{}

FormatContext::~FormatContext()
{
    close();
}

bool FormatContext::isOpen()
{
    return d_ptr->isOpen;
}

bool FormatContext::openFilePath(const QString &filepath, OpenMode mode)
{
    d_ptr->mode = mode;
    close();
    d_ptr->filepath = filepath;
    //初始化pFormatCtx结构
    if (mode == ReadOnly) {
        auto ret = avformat_open_input(&d_ptr->formatCtx,
                                       d_ptr->filepath.toLocal8Bit().constData(),
                                       nullptr,
                                       nullptr);
        if (ret != 0) {
            d_ptr->setError(ret);
            return false;
        }
        av_format_inject_global_side_data(d_ptr->formatCtx);
        d_ptr->isOpen = true;
    } else if (mode == WriteOnly) {
        avformat_free_context(d_ptr->formatCtx);
        auto ret = avformat_alloc_output_context2(&d_ptr->formatCtx,
                                                  nullptr,
                                                  nullptr,
                                                  d_ptr->filepath.toLocal8Bit().constData());
        if (ret < 0) {
            d_ptr->setError(ret);
            return false;
        }
        d_ptr->isOpen = true;
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
    case WriteOnly: avio_close(); break;
    default: break;
    }
}

bool FormatContext::avio_open()
{
    if (d_ptr->formatCtx->oformat->flags & AVFMT_NOFILE) {
        return false;
    }
    auto ret = ::avio_open(&d_ptr->formatCtx->pb,
                           d_ptr->filepath.toLocal8Bit().constData(),
                           AVIO_FLAG_WRITE);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

void FormatContext::avio_close()
{
    if (!d_ptr->isOpen && d_ptr->mode != WriteOnly) {
        return;
    }
    if (d_ptr->formatCtx && !(d_ptr->formatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&d_ptr->formatCtx->pb);
    }
    avformat_free_context(d_ptr->formatCtx);
    d_ptr->formatCtx = nullptr;
    d_ptr->isOpen = false;
}

bool FormatContext::writeHeader()
{
    auto ret = avformat_write_header(d_ptr->formatCtx, nullptr);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FormatContext::writeFrame(Packet *packet)
{
    auto ret = av_interleaved_write_frame(d_ptr->formatCtx, packet->avPacket());
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FormatContext::writeTrailer()
{
    auto ret = av_write_trailer(d_ptr->formatCtx);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FormatContext::findStream()
{
    //获取音视频流数据信息
    int ret = avformat_find_stream_info(d_ptr->formatCtx, nullptr);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    qDebug() << "The video file contains the number of stream information:"
             << d_ptr->formatCtx->nb_streams;
    d_ptr->printInformation();
    d_ptr->initMetaData();
    d_ptr->findStreamIndex();
    return true;
}

int FormatContext::streams() const
{
    return d_ptr->formatCtx->nb_streams;
}

QMap<int, QString> FormatContext::audioMap() const
{
    return d_ptr->audioMap;
}

QVector<int> FormatContext::videoIndexs() const
{
    return d_ptr->videoIndexs;
}

QMap<int, QString> FormatContext::subtitleMap() const
{
    return d_ptr->subtitleMap;
}

int FormatContext::findBestStreamIndex(AVMediaType type) const
{
    return av_find_best_stream(d_ptr->formatCtx, type, -1, -1, nullptr, 0);
}

void FormatContext::discardStreamExcluded(int audioIndex, int videoIndex, int subtitleIndex)
{
    for (uint i = 0; i < d_ptr->formatCtx->nb_streams; i++) {
        if (i == audioIndex || i == videoIndex || i == subtitleIndex) {
            continue;
        }
        d_ptr->formatCtx->streams[i]->discard = AVDISCARD_ALL;
    }
}

AVStream *FormatContext::stream(int index)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx->streams[index];
}

AVStream *FormatContext::createStream()
{
    auto stream = avformat_new_stream(d_ptr->formatCtx, nullptr);
    if (!stream) {
        qWarning() << "Failed allocating output stream\n";
    }
    return stream;
}

bool FormatContext::readFrame(Packet *packet)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_read_frame(d_ptr->formatCtx, packet->avPacket());
    if (ret >= 0) {
        return true;
    }
    d_ptr->setError(ret);
    return false;
}

bool FormatContext::checkPktPlayRange(Packet *packet)
{
    auto avPacket = packet->avPacket();
    /* check if packet is in play range specified by user, then queue, otherwise discard */
    auto start_time = AV_NOPTS_VALUE;
    auto duration = AV_NOPTS_VALUE;
    auto stream_start_time = d_ptr->formatCtx->streams[avPacket->stream_index]->start_time;
    auto pkt_ts = avPacket->pts == AV_NOPTS_VALUE ? avPacket->dts : avPacket->pts;
    auto pkt_in_play_range
        = duration == AV_NOPTS_VALUE
          || (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0))
                         * av_q2d(d_ptr->formatCtx->streams[avPacket->stream_index]->time_base)
                     - (double) (start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
                 <= ((double) duration / 1000000);
    return pkt_in_play_range;
}

AVRational FormatContext::guessFrameRate(int index) const
{
    return guessFrameRate(d_ptr->formatCtx->streams[index]);
}

AVRational FormatContext::guessFrameRate(AVStream *stream) const
{
    return av_guess_frame_rate(d_ptr->formatCtx, stream, nullptr);
}

bool FormatContext::seekFirstFrame()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int64_t timestamp = 0;
    if (d_ptr->formatCtx->start_time != AV_NOPTS_VALUE) {
        timestamp = d_ptr->formatCtx->start_time;
    }
    int ret = avformat_seek_file(d_ptr->formatCtx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FormatContext::seek(int64_t timestamp)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    Q_ASSERT(timestamp >= 0);
    int64_t seek_min = timestamp - Seek_Offset;
    if (seek_min < 0) {
        seek_min = 0;
    }
    int64_t seek_max = timestamp + Seek_Offset;
    // qDebug() << "seek: " << timestamp;
    // AV_TIME_BASE
    int ret = avformat_seek_file(d_ptr->formatCtx,
                                 -1,
                                 seek_min * AV_TIME_BASE,
                                 timestamp * AV_TIME_BASE,
                                 seek_max * AV_TIME_BASE,
                                 0);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FormatContext::seek(int index, int64_t timestamp)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_seek_frame(d_ptr->formatCtx, index, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

void FormatContext::dumpFormat()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    av_dump_format(d_ptr->formatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), d_ptr->mode - 1);
}

AVFormatContext *FormatContext::avFormatContext()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx;
}

qint64 FormatContext::duration() const
{
    return d_ptr->isOpen ? (d_ptr->formatCtx->duration / 1000) : 0;
}

QImage &FormatContext::coverImage() const
{
    return d_ptr->coverImage;
}

} // namespace Ffmpeg
