#include "formatcontext.h"
#include "packet.h"
#include "averror.h"

#include <QDebug>
#include <QImage>
#include <QTime>

extern "C"{
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class FormatContextPrivate{
public:
    FormatContextPrivate(QObject *parent)
        : owner(parent){
#if CONFIG_AVDEVICE
        avdevice_register_all();
#endif
        avformat_network_init();
        formatCtx = avformat_alloc_context();
        Q_ASSERT(formatCtx != nullptr);
    }

    ~FormatContextPrivate(){
        //Q_ASSERT(formatCtx != nullptr);
        avformat_free_context(formatCtx);
    }

    QObject *owner;
    AVFormatContext *formatCtx;
    QString filepath;
    bool isOpen = false;

    QVector<int> videoIndexs;
    QMap<int, QString> audioMap;
    QMap<int, QString> subtitleMap;
    QImage coverImage;
};

FormatContext::FormatContext(QObject *parent)
    : QObject(parent)
    , d_ptr(new FormatContextPrivate(this))
{

}

FormatContext::~FormatContext()
{

}

bool FormatContext::isOpen()
{
    return d_ptr->isOpen;
}

bool FormatContext::openFilePath(const QString &filepath)
{
    close();
    d_ptr->filepath = filepath;
    //初始化pFormatCtx结构
    int ret = avformat_open_input(&d_ptr->formatCtx, d_ptr->filepath.toLocal8Bit().constData(), nullptr, nullptr);
    if (ret != 0){
        emit error(AVError(ret));
        return false;
    }
    av_format_inject_global_side_data(d_ptr->formatCtx);
    d_ptr->isOpen = true;
    return true;
}

void FormatContext::close()
{
    if(!d_ptr->isOpen)
        return;
    avformat_close_input(&d_ptr->formatCtx);
    //d_ptr->formatCtx = avformat_alloc_context();
    //Q_ASSERT(d_ptr->formatCtx != nullptr);
    d_ptr->isOpen = false;
}

bool FormatContext::findStream()
{
    //获取音视频流数据信息
    int ret = avformat_find_stream_info(d_ptr->formatCtx, nullptr);
    if (ret < 0){
        emit error(AVError(ret));
        return false;
    }
    qDebug() << "The video file contains the number of stream information:" << d_ptr->formatCtx->nb_streams;
    printInformation();
    initMetaData();
    findStreamIndex();
    return true;
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

void FormatContext::findStreamIndex()
{
    d_ptr->videoIndexs.clear();
    d_ptr->audioMap.clear();
    d_ptr->subtitleMap.clear();
    d_ptr->coverImage = QImage();

    //nb_streams视音频流的个数
    for (uint i = 0; i < d_ptr->formatCtx->nb_streams; i++){
        switch (d_ptr->formatCtx->streams[i]->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO: d_ptr->videoIndexs.append(i); break;
        case AVMEDIA_TYPE_AUDIO: {
            AVDictionaryEntry *tag = nullptr;
            QString str;
            while (nullptr != (tag = av_dict_get(d_ptr->formatCtx->streams[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))){
                str = str + tag->key + ":"+ QString::fromUtf8(tag->value) + " ";
            }
            str.chop(1);
            d_ptr->audioMap.insert(i, str);
        }
        break;
        case AVMEDIA_TYPE_SUBTITLE:{
            QString str;
            AVDictionaryEntry *tag = nullptr;
            while (nullptr != (tag = av_dict_get(d_ptr->formatCtx->streams[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))){
                str = str + tag->key + ":"+ QString::fromUtf8(tag->value) + " ";
            }
            str.chop(1);
            d_ptr->subtitleMap.insert(i, str);
        } break;
        default: break;
        }

        if (d_ptr->formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC){
            AVPacket pkt = d_ptr->formatCtx->streams[i]->attached_pic;
            d_ptr->coverImage = QImage::fromData((uchar*)pkt.data, pkt.size);
        }
    }
}

void FormatContext::initMetaData()
{
    AVDictionaryEntry *tag = nullptr;
    while (nullptr != (tag = av_dict_get(d_ptr->formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))){
        qDebug() << tag->key << QString::fromUtf8(tag->value);
    }
}

void FormatContext::printInformation()
{
    qInfo() << tr("AV Format Name: ") << d_ptr->formatCtx->iformat->name;
    QTime time(QTime::fromMSecsSinceStartOfDay(d_ptr->formatCtx->duration / 1000));
    qInfo() << tr("Duration:") << time.toString("hh:mm:ss.zzz");

    if (d_ptr->formatCtx->pb)
        d_ptr->formatCtx->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
}

AVStream *FormatContext::stream(int index)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx->streams[index];
}

bool FormatContext::readFrame(Packet *packet)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_read_frame(d_ptr->formatCtx, packet->avPacket());
    if(ret >= 0)
        return true;
    emit error(AVError(ret));;
    return false;
}

int FormatContext::checkPktPlayRange(Packet *packet)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
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

bool FormatContext::seek(int index, int64_t timestamp)
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    int ret = av_seek_frame(d_ptr->formatCtx, index, timestamp, AVSEEK_FLAG_BACKWARD);
    if(ret < 0){
        emit error(AVError(ret));
        return false;
    }
    return true;
}

void FormatContext::dumpFormat()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    av_dump_format(d_ptr->formatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), 0);
}

AVFormatContext *FormatContext::avFormatContext()
{
    Q_ASSERT(d_ptr->formatCtx != nullptr);
    return d_ptr->formatCtx;
}

qint64 FormatContext::duration()
{
    return d_ptr->formatCtx->duration / 1000;
}

QImage FormatContext::coverImage() const
{
    return d_ptr->coverImage;
}

}
