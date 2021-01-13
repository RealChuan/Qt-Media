#include "formatcontext.h"
#include "packet.h"

#include <QDebug>
#include <QTime>

extern "C"{
#include <libavformat/avformat.h>
}

class FormatContextPrivate{
public:
    FormatContextPrivate(QObject *parent)
        : owner(parent){
        formatCtx = avformat_alloc_context();
    }

    ~FormatContextPrivate(){
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

void FormatContext::findStreamIndex(int &audioIndex, int &videoIndex)
{
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
        default: break;
        }
    }
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

void FormatContext::dumpFormat()
{
    av_dump_format(d_ptr->formatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), 0);
}
