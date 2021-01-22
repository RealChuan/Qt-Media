#include "formatcontext.h"
#include "packet.h"

#include <QDebug>
#include <QTime>

extern "C"{
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class FormatContextPrivate{
public:
    FormatContextPrivate(QObject *parent)
        : owner(parent){
        formatCtx = avformat_alloc_context();
        Q_ASSERT(formatCtx != nullptr);
    }

    ~FormatContextPrivate(){
        Q_ASSERT(formatCtx != nullptr);
        avformat_free_context(formatCtx);
    }

    QObject *owner;
    AVFormatContext *formatCtx;
    QString filepath;
    QString error;
};

FormatContext::FormatContext(QObject *parent)
    : QObject(parent)
    , d_ptr(new FormatContextPrivate(this))
{

}

FormatContext::~FormatContext()
{

}

QString FormatContext::error() const
{
    return d_ptr->error;
}

bool FormatContext::openFilePath(const QString &filepath)
{
    d_ptr->filepath = filepath;
    //初始化pFormatCtx结构
    if (avformat_open_input(&d_ptr->formatCtx, d_ptr->filepath.toLocal8Bit().constData(), nullptr, nullptr) != 0){
        d_ptr->error = tr("Couldn't open input stream.");
        return false;
    }
    qInfo() << tr("AV Format Name: ") << d_ptr->formatCtx->iformat->name;
    QTime time(QTime::fromMSecsSinceStartOfDay(d_ptr->formatCtx->duration / 1000));
    qInfo() << tr("Duration:") << time.toString("hh:mm:ss.zzz");
    return true;
}

bool FormatContext::findStream()
{
    //获取音视频流数据信息
    if (avformat_find_stream_info(d_ptr->formatCtx, nullptr) < 0){
        d_ptr->error = tr("Couldn't find stream information");
        return false;
    }
    return true;
}

QVector<int> FormatContext::findStreamIndex(int &audioIndex, int &videoIndex)
{
    //av_find_best_stream

    QVector<int> subtitleIndex;
    //nb_streams视音频流的个数
    for (uint i = 0; i < d_ptr->formatCtx->nb_streams; i++){
        switch (d_ptr->formatCtx->streams[i]->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            if(videoIndex < 0)
                videoIndex = i;
            break;
        case AVMEDIA_TYPE_AUDIO:
            if(audioIndex < 0)
                audioIndex = i;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            subtitleIndex.append(i);
            break;
        default: break;
        }
    }
    return subtitleIndex;
}

AVStream *FormatContext::stream(int index)
{
    return d_ptr->formatCtx->streams[index];
}

bool FormatContext::readFrame(Packet *packet)
{
    if(av_read_frame(d_ptr->formatCtx, packet->avPacket()) < 0)
        return false;
    return true;
}

int FormatContext::checkPktPlayRange(Packet *packet)
{
    /* check if packet is in play range specified by user, then queue, otherwise discard */
    int64_t start_time = AV_NOPTS_VALUE;
    int64_t duration = AV_NOPTS_VALUE;
    int64_t stream_start_time = d_ptr->formatCtx->streams[packet->avPacket()->stream_index]->start_time;
    int64_t pkt_ts = packet->avPacket()->pts == AV_NOPTS_VALUE ? packet->avPacket()->dts : packet->avPacket()->pts;
    int pkt_in_play_range = duration == AV_NOPTS_VALUE ||
                            (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                                        av_q2d(d_ptr->formatCtx->streams[packet->avPacket()->stream_index]->time_base) -
                                    (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
                                <= ((double)duration / 1000000);
    return pkt_in_play_range;
}

void FormatContext::dumpFormat()
{
    av_dump_format(d_ptr->formatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), 0);
}

AVFormatContext *FormatContext::avFormatContext()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx;
}

}
