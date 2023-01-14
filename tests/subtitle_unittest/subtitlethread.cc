#include "subtitlethread.hpp"

#include <ffmpeg/avcontextinfo.h>
#include <ffmpeg/formatcontext.h>
#include <ffmpeg/packet.h>
#include <ffmpeg/subtitle.h>

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
    QMap<int, Ffmpeg::AVContextInfo *> subtitleInfos;
    auto clean = qScopeGuard([&] {
        if (subtitleInfos.isEmpty()) {
            return;
        }
        qDeleteAll(subtitleInfos);
        subtitleInfos.clear();
    });
    for (auto i : indexs) {
        auto info = new Ffmpeg::AVContextInfo;
        subtitleInfos[i] = info;
        info->setIndex(i);
        info->setStream(formatCtxPtr->stream(i));
        if (!info->initDecoder(formatCtxPtr->guessFrameRate(i))) {
            return;
        }
        if (!info->openCodec(false)) {
            return;
        }
    }
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
        auto info = subtitleInfos.value(stream_index);
        if (!info) {
            continue;
        }
        QScopedPointer<Ffmpeg::Subtitle> subtitlePtr(new Ffmpeg::Subtitle);
        if (!info->decodeSubtitle2(subtitlePtr.get(), packetPtr.data())) {
            continue;
        }
        subtitlePtr->parse(nullptr);
        auto tests = subtitlePtr->texts();
        for (const auto &test : qAsConst(tests)) {
            qInfo() << QString::fromUtf8(test);
        }
        sleep(1);
    }
}
