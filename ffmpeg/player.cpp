#include "player.h"
#include "formatcontext.h"
#include "avcontextinfo.h"
#include "packet.h"
#include "avimage.h"
#include "codeccontext.h"
#include "avaudio.h"
#include "playframe.h"
#include "audiodecoder.h"
#include "videodecoder.h"

#include <utils/utils.h>

#include <QDebug>
#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QThread>
#include <QWaitCondition>
#include <QDateTime>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class PlayerPrivate{
public:
    PlayerPrivate(QThread *parent)
        : owner(parent){
        formatCtx = new FormatContext(owner);
        audioInfo = new AVContextInfo(owner);
        videoInfo = new AVContextInfo(owner);

        audioDecoder = new AudioDecoder(owner);
        videoDecoder = new VideoDecoder(owner);
    }
    QThread *owner;

    FormatContext *formatCtx;
    AVContextInfo *audioInfo;
    AVContextInfo *videoInfo;

    AudioDecoder *audioDecoder;
    VideoDecoder *videoDecoder;

    QString filepath;
    volatile bool isopen = true;
    volatile bool runing = true;
    volatile bool pause = false;
    bool inPause = false;
    volatile bool seek = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    QString error;
};

Player::Player(QObject *parent)
    : QThread(parent)
    , d_ptr(new PlayerPrivate(this))
{
    qDebug() << avcodec_configuration();
    qDebug() << avcodec_version();
    buildConnect();
}

Player::~Player()
{
    stop();
}

void Player::onSetFilePath(const QString &filepath)
{
    d_ptr->filepath = filepath;
    d_ptr->isopen = initAvCode();
    if(!d_ptr->isopen){
        qWarning() << "initAvCode Error";
        return;
    }
}

void Player::onPlay()
{
    stop();
    d_ptr->runing = true;
    start();
}

void Player::onSeek(int timestamp)
{
    //qDebug() << "Seek: " << timestamp;

    pause(true);
    d_ptr->audioDecoder->clear();
    d_ptr->videoDecoder->clear();

    d_ptr->formatCtx->seek(d_ptr->audioInfo->index(), timestamp / av_q2d(d_ptr->audioInfo->stream()->time_base));
    d_ptr->formatCtx->seek(d_ptr->videoInfo->index(), timestamp / av_q2d(d_ptr->videoInfo->stream()->time_base));
    d_ptr->audioInfo->flush();
    d_ptr->videoInfo->flush();
    d_ptr->audioDecoder->setSeek(timestamp * 1000);
    pause(false);
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
    if(!d_ptr->formatCtx->openFilePath(d_ptr->filepath)){
        qWarning() << d_ptr->formatCtx->error();
        return false;
    }

    emit durationChanged(d_ptr->formatCtx->duration());

    //获取音视频流数据信息
    if(!d_ptr->formatCtx->findStream()){
        qWarning() << d_ptr->formatCtx->error();
        return false;
    }

    int audioIndex = -1;
    int videoIndex = -1;
    QVector<int> subTitleIndexs = d_ptr->formatCtx->findStreamIndex(audioIndex, videoIndex);
    if (audioIndex == -1 || videoIndex == -1){
        d_ptr->error = tr("Didn't find a video or audio stream.");
        emit error(d_ptr->error);
        return false;
    }

    qDebug() << audioIndex << videoIndex;

    d_ptr->audioInfo->setIndex(audioIndex);
    d_ptr->audioInfo->setStream(d_ptr->formatCtx->stream(audioIndex));
    if(!d_ptr->audioInfo->findDecoder())
        return false;

    d_ptr->videoInfo->setIndex(videoIndex);
    d_ptr->videoInfo->setStream(d_ptr->formatCtx->stream(videoIndex));
    if(!d_ptr->videoInfo->findDecoder())
        return false;

    d_ptr->isopen = true;

    d_ptr->formatCtx->dumpFormat();

    return true;
}

void Player::playVideo()
{
    if(!d_ptr->isopen)
        return;

    Packet packet;
    d_ptr->audioDecoder->startDecoder(d_ptr->formatCtx, d_ptr->audioInfo);
    d_ptr->videoDecoder->startDecoder(d_ptr->formatCtx, d_ptr->videoInfo);

    while (d_ptr->runing && d_ptr->formatCtx->readFrame(&packet)){
        if(d_ptr->formatCtx->checkPktPlayRange(&packet) <= 0){

        }else if(packet.avPacket()->stream_index == d_ptr->audioInfo->index()){ // 如果是音频数据
            d_ptr->audioDecoder->append(packet);
        }else if(packet.avPacket()->stream_index == d_ptr->videoInfo->index()
                   && !(d_ptr->videoInfo->stream()->disposition &AV_DISPOSITION_ATTACHED_PIC)){ // 如果是视频数据
            d_ptr->videoDecoder->append(packet);
        }
        packet.clear();

        while(d_ptr->pause){
            d_ptr->inPause = true;
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex);
            d_ptr->inPause = false;
        }

        while(d_ptr->videoDecoder->size() > 10)
            msleep(40);
    }

    while(d_ptr->runing && d_ptr->videoDecoder->size() != 0)
        msleep(40);
    d_ptr->audioDecoder->stopDecoder();
    d_ptr->videoDecoder->stopDecoder();
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
    onSeek(1000);
    playVideo();
}

void Player::buildConnect()
{
    connect(d_ptr->videoDecoder, &VideoDecoder::readyRead, this, &Player::readyRead);
    connect(d_ptr->audioDecoder, &AudioDecoder::positionChanged, this, &Player::positionChanged);
}

}
