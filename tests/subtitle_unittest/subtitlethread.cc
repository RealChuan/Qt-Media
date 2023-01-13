#include "subtitlethread.hpp"

#include <ffmpeg/formatcontext.h>
#include <ffmpeg/packet.h>

#include <QMap>

extern "C" {
#include <libavcodec/packet.h>
}

class SubtitleThread::SubtitleThreadPrivate
{
public:
    SubtitleThreadPrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;

    QString filepath;
    std::atomic_bool runing = false;
};

SubtitleThread::SubtitleThread(QObject *parent)
    : QThread{parent}
    , d_ptr(new SubtitleThreadPrivate(this))
{}

SubtitleThread::~SubtitleThread()
{
    stopRead();
}

void SubtitleThread::setFilePath(const QString &filepath)
{
    d_ptr->filepath = filepath;
}

void SubtitleThread::startRead()
{
    stopRead();
    d_ptr->runing.store(true);
    start();
}

void SubtitleThread::stopRead()
{
    d_ptr->runing.store(false);
    if (isRunning()) {
        quit();
        wait();
    }
}

void SubtitleThread::run()
{
    QScopedPointer<Ffmpeg::FormatContext> formatCtxPtr(new Ffmpeg::FormatContext);
    if (!formatCtxPtr->openFilePath(d_ptr->filepath)) {
        return;
    }
    if (!formatCtxPtr->findStream()) {
        return;
    }
    formatCtxPtr->seekFirstFrame();
    auto indexs = formatCtxPtr->subtitleMap().keys();
    formatCtxPtr->discardStreamExcluded(indexs);
    while (d_ptr->runing.load()) {
        QScopedPointer<Ffmpeg::Packet> packetPtr(new Ffmpeg::Packet);
        if (!formatCtxPtr->readFrame(packetPtr.data())) {
            break;
        }
        auto stream_index = packetPtr->streamIndex();
        if (!indexs.contains(stream_index)) {
            continue;
        }
        qInfo() << "SubTitle text: " << stream_index
                << QString::fromUtf8(packetPtr->avPacket()->data);
        sleep(1);
    }
}
