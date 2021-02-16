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
#include "videooutputwidget.h"

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
    volatile bool show = false;
    volatile bool isopen = true;
    volatile bool runing = true;
    volatile bool seek = false;
    qint64 seekTime = 0; // seconds

    volatile Player::MediaState mediaState = Player::MediaState::StoppedState;

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
    onStop();
}

void Player::onSetFilePath(const QString &filepath)
{
    onStop();
    d_ptr->filepath = filepath;
    d_ptr->isopen = initAvCode();
    if(!d_ptr->isopen){
        qWarning() << "initAvCode Error";
        return;
    }
}

void Player::onPlay()
{
    buildConnect(true);
    setMediaState(MediaState::PlayingState);
    d_ptr->show = true;
    d_ptr->runing = true;
    start();
}

void Player::onStop()
{
    buildConnect(false);
    d_ptr->show = false;
    d_ptr->runing = false;
    if(isRunning()){
        quit();
        wait();
    }
    d_ptr->formatCtx->close();
    setMediaState(MediaState::StoppedState);
}

void Player::onSeek(int timestamp)
{
    qDebug() << "Seek: " << timestamp;

    if(d_ptr->seek)
        return;
    blockSignals(true);
    d_ptr->seek = true;
    d_ptr->seekTime = timestamp;
}

void Player::onReadyRead(const QImage &image)
{
    if(image.isNull() || !d_ptr->show){
        static qint64 count = 0;
        qDebug() << "onReadyRead" << ++count;;
        return;
    }
    emit readyRead(image);
}

bool Player::isOpen()
{
    return d_ptr->isopen;
}

QString Player::lastError() const
{
    return d_ptr->error;
}

void Player::setVolume(qreal volume)
{
    if(volume < 0 || volume > 1)
        return;
    d_ptr->audioDecoder->setVolume(volume);
}

void Player::setSpeed(double speed)
{
    if(speed <= 0){
        qWarning() << tr("Speed cannot be less than or equal to 0!");
        return;
    }
    d_ptr->audioDecoder->setSpeed(speed);
    d_ptr->videoDecoder->setSpeed(speed);
}

double Player::speed()
{
    return d_ptr->audioDecoder->speed();
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
    emit positionChanged(0);

    //获取音视频流数据信息
    if(!d_ptr->formatCtx->findStream()){
        qWarning() << d_ptr->formatCtx->error();
        return false;
    }

    if(d_ptr->formatCtx->audioIndexs().isEmpty()
        || d_ptr->formatCtx->videoIndexs().isEmpty()){
        d_ptr->error = tr("Didn't find a video and audio stream.");
        emit error(d_ptr->error);
        return false;
    }

    d_ptr->audioInfo->resetIndex();
    if(!d_ptr->formatCtx->audioIndexs().isEmpty()){
        int audioIndex = d_ptr->formatCtx->audioIndexs().first();
        d_ptr->audioInfo->setIndex(audioIndex);
        d_ptr->audioInfo->setStream(d_ptr->formatCtx->stream(audioIndex));
        if(!d_ptr->audioInfo->findDecoder()){
            return false;
        }
    }

    d_ptr->videoInfo->resetIndex();
    if(!d_ptr->formatCtx->videoIndexs().isEmpty()){
        int videoIndex = d_ptr->formatCtx->videoIndexs().first();
        d_ptr->videoInfo->setIndex(videoIndex);
        d_ptr->videoInfo->setStream(d_ptr->formatCtx->stream(videoIndex));
        if(!d_ptr->videoInfo->findDecoder()){
            return false;
        }
    }

    qDebug() << d_ptr->audioInfo->index() << d_ptr->videoInfo->index();

    d_ptr->isopen = true;

    d_ptr->formatCtx->dumpFormat();

    return true;
}

void Player::playVideo()
{
    Packet packet;
    d_ptr->audioDecoder->startDecoder(d_ptr->formatCtx, d_ptr->audioInfo);
    d_ptr->videoDecoder->startDecoder(d_ptr->formatCtx, d_ptr->videoInfo);

    checkSeek();

    while (d_ptr->runing && d_ptr->formatCtx->readFrame(&packet)){
        if(d_ptr->formatCtx->checkPktPlayRange(&packet) <= 0){

        }else if(d_ptr->audioInfo->isIndexVaild() && packet.avPacket()->stream_index == d_ptr->audioInfo->index()){ // 如果是音频数据
            d_ptr->audioDecoder->append(packet);
        }else if(d_ptr->videoInfo->isIndexVaild() && packet.avPacket()->stream_index == d_ptr->videoInfo->index()
                   && !(d_ptr->videoInfo->stream()->disposition &AV_DISPOSITION_ATTACHED_PIC)){ // 如果是视频数据
            d_ptr->videoDecoder->append(packet);
        }
        packet.clear();

        checkSeek();

        while(d_ptr->videoDecoder->size() > 20 && !d_ptr->seek)
            msleep(1);
    }

    while(d_ptr->runing && d_ptr->videoDecoder->size() != 0)
        msleep(1);
    d_ptr->videoDecoder->stopDecoder();
    d_ptr->audioDecoder->stopDecoder();

    qDebug() << "play finish";
}

void Player::checkSeek()
{
    if(!d_ptr->seek)
        return;
    QElapsedTimer timer;
    timer.start();
    d_ptr->videoDecoder->seek(d_ptr->seekTime);
    d_ptr->audioDecoder->seek(d_ptr->seekTime);
    d_ptr->seek = false;
    blockSignals(false);
    setMediaState(MediaState::PlayingState);
    qDebug() << "Seek ElapsedTimer: " << timer.elapsed();
}

void Player::setMediaState(Player::MediaState mediaState)
{
    d_ptr->mediaState = mediaState;
    emit stateChanged(d_ptr->mediaState);
}

void Player::pause(bool status)
{
    d_ptr->audioDecoder->pause(status);
    d_ptr->videoDecoder->pause(status);
    if(status){
        setMediaState(MediaState::PausedState);
    }else if(isRunning()){
        setMediaState(MediaState::PlayingState);
    }else{
        setMediaState(MediaState::StoppedState);
    }
}

Player::MediaState Player::mediaState()
{
    return d_ptr->mediaState;
}

void Player::setVideoOutputWidget(VideoOutputWidget *widget)
{
    connect(this, &Player::readyRead, widget, &VideoOutputWidget::onReadyRead, Qt::UniqueConnection);
    connect(this, &Player::finished, widget, &VideoOutputWidget::onFinish, Qt::UniqueConnection);
}

void Player::run()
{
    if(!d_ptr->isopen)
        return;
    // test code
    //onSeek(1000);
    playVideo();
}

void Player::buildConnect(bool state)
{
    if(state){
        connect(d_ptr->videoDecoder, &VideoDecoder::readyRead, this, &Player::onReadyRead, Qt::UniqueConnection);
        connect(d_ptr->audioDecoder, &AudioDecoder::positionChanged, this, &Player::positionChanged, Qt::UniqueConnection);
    }else{
        disconnect(d_ptr->videoDecoder, &VideoDecoder::readyRead, this, &Player::onReadyRead);
        disconnect(d_ptr->audioDecoder, &AudioDecoder::positionChanged, this, &Player::positionChanged);
    }
}

}
