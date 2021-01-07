#include "player.h"

#include <QDebug>
#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QThread>
#include <QWaitCondition>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class PlayerPrivate{
public:
    PlayerPrivate(QThread *parent)
        : owner(parent){
        pFormatCtx = avformat_alloc_context();
    }

    ~PlayerPrivate(){
        avformat_free_context(pFormatCtx);
    }
    QThread *owner;

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodecParameters *codecpar;
    AVCodec *pCodec;

    QString filepath;
    int videoindex = -1;
    bool isopen = true;
    bool runing = true;
    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    QString error;
};

Player::Player(QObject *parent)
    : QThread(parent)
    , d_ptr(new PlayerPrivate(this))
{

}

Player::~Player()
{
    stop();
}

void Player::onSetFilePath(const QString &filepath)
{
    d_ptr->filepath = filepath;
}

void Player::onPlay()
{
    stop();
    d_ptr->runing = true;
    start();
}

bool Player::isOpen()
{
    return d_ptr->isopen;
}

QString Player::lastError() const
{
    return d_ptr->error;
}

bool Player::initAvCode()
{
    d_ptr->isopen = false;

    //初始化pFormatCtx结构
    if (avformat_open_input(&d_ptr->pFormatCtx, d_ptr->filepath.toLocal8Bit().constData(), nullptr, nullptr) != 0){
        d_ptr->error = tr("Couldn't open input stream.");
        emit error(d_ptr->error);
        return false;
    }
    //获取音视频流数据信息
    if (avformat_find_stream_info(d_ptr->pFormatCtx, nullptr) < 0){
        d_ptr->error = tr("Couldn't find stream information");
        emit error(d_ptr->error);
        return false;
    }

    //nb_streams视音频流的个数，这里当查找到视频流时就中断了。
    for (uint i = 0; i < d_ptr->pFormatCtx->nb_streams; i++)
        if (d_ptr->pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            d_ptr->videoindex = i;
            break;
        }
    if (d_ptr->videoindex == -1){
        d_ptr->error = tr("Didn't find a video stream.n");
        emit error(d_ptr->error);
        return false;
    }

    //获取视频流编码结构
    d_ptr->codecpar = d_ptr->pFormatCtx->streams[d_ptr->videoindex]->codecpar;
    //查找解码器
    d_ptr->pCodec = avcodec_find_decoder(d_ptr->codecpar->codec_id);
    if (nullptr == d_ptr->pCodec){
        d_ptr->error =  tr("Codec not found.");
        emit error(d_ptr->error);
        return false;
    }

    d_ptr->pCodecCtx = avcodec_alloc_context3(d_ptr->pCodec);
    if(nullptr == d_ptr->pCodecCtx){
        d_ptr->error = tr("Could not open codec.");
        emit error(d_ptr->error);
        return false;
    }
    avcodec_parameters_to_context(d_ptr->pCodecCtx, d_ptr->codecpar);
    d_ptr->pCodecCtx->pkt_timebase = d_ptr->pFormatCtx->streams[d_ptr->videoindex]->time_base;
    //av_codec_set_pkt_timebase(d_ptr->pCodecCtx, d_ptr->pFormatCtx->streams[d_ptr->videoindex]->time_base);

    //用于初始化pCodecCtx结构
    if (avcodec_open2(d_ptr->pCodecCtx, d_ptr->pCodec, NULL) < 0){
        d_ptr->error = tr("Could not open codec.");
        emit error(d_ptr->error);
        return false;
    }

    d_ptr->isopen = true;
    return true;
}

void Player::playVideo()
{
    if(!d_ptr->isopen)
        return;

    //创建帧结构，此函数仅分配基本结构空间，图像数据空间需通过av_malloc分配
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameRGB = av_frame_alloc();

    //创建动态内存,创建存储图像数据的空间
    //av_image_get_buffer_size获取一帧图像需要的大小
    unsigned char *out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, d_ptr->pCodecCtx->width, d_ptr->pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, out_buffer,
                         AV_PIX_FMT_RGB32, d_ptr->pCodecCtx->width, d_ptr->pCodecCtx->height, 1);

    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //此函数打印输入或输出的详细信息
    av_dump_format(d_ptr->pFormatCtx, 0, d_ptr->filepath.toLocal8Bit().constData(), 0);
    //printf("-------------------------------------------------\n");
    //初始化img_convert_ctx结构
    struct SwsContext *img_convert_ctx = sws_getContext(d_ptr->pCodecCtx->width, d_ptr->pCodecCtx->height, d_ptr->pCodecCtx->pix_fmt,
                                                        d_ptr->pCodecCtx->width, d_ptr->pCodecCtx->height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

    //av_read_frame读取一帧未解码的数据
    while (av_read_frame(d_ptr->pFormatCtx, packet) >= 0 && d_ptr->runing){
        //如果是视频数据
        if (packet->stream_index == d_ptr->videoindex){
            //解码一帧视频数据
            int ret = avcodec_send_packet(d_ptr->pCodecCtx, packet);
            av_packet_unref(packet);
            if (ret < 0){
                d_ptr->error = tr("send_packet Error: %1.").arg(ret);
                emit error(d_ptr->error);
                continue;
            }

            ret = avcodec_receive_frame(d_ptr->pCodecCtx, pFrame);
            if(ret < 0){
                d_ptr->error = tr("avcodec_receive_frame Error: %1.").arg(ret);
                emit error(d_ptr->error);
                continue;
            }

            sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, d_ptr->pCodecCtx->height,
                      pFrameRGB->data, pFrameRGB->linesize);
            QImage img((uchar*)pFrameRGB->data[0], d_ptr->pCodecCtx->width, d_ptr->pCodecCtx->height, QImage::Format_RGB32);
            emit readyRead(QPixmap::fromImage(img));

            while(d_ptr->pause){
                QMutexLocker locker(&d_ptr->mutex);
                d_ptr->waitCondition.wait(&d_ptr->mutex);
            }

            QThread::msleep(1);
        }
    }
}

void Player::stop()
{
    d_ptr->pause = false;
    d_ptr->runing = false;
    d_ptr->waitCondition.wakeOne();
    if(isRunning()){
        quit();
        wait();
    }
}

void Player::pause(bool status)
{
    d_ptr->pause = status;
    if(status)
        return;
    d_ptr->waitCondition.wakeOne();
}

void Player::run()
{
    if(!initAvCode())
        return;
    playVideo();
}
