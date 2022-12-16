#include "subtitledecoder.h"
#include "decodersubtitleframe.hpp"
#include "subtitle.h"

namespace Ffmpeg {

class SubtitleDecoder::SubtitleDecoderPrivate
{
public:
    SubtitleDecoderPrivate(QObject *parent)
        : owner(parent)
    {
        decoderSubtitleFrame = new DecoderSubtitleFrame(owner);
    }

    QObject *owner;

    DecoderSubtitleFrame *decoderSubtitleFrame;
};

SubtitleDecoder::SubtitleDecoder(QObject *parent)
    : Decoder<Packet *>(parent)
    , d_ptr(new SubtitleDecoderPrivate(this))
{}

SubtitleDecoder::~SubtitleDecoder() {}

void SubtitleDecoder::pause(bool state)
{
    d_ptr->decoderSubtitleFrame->pause(state);
}

void SubtitleDecoder::setVideoResolutionRatio(const QSize &size)
{
    d_ptr->decoderSubtitleFrame->setVideoResolutionRatio(size);
}

void SubtitleDecoder::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->decoderSubtitleFrame->setVideoRenders(videoRenders);
}

void SubtitleDecoder::runDecoder()
{
    d_ptr->decoderSubtitleFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        if (m_seek) {
            clear();
            d_ptr->decoderSubtitleFrame->seek(m_seekTime, m_latchPtr.lock());
            seekFinish();
        }

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());
        if (packetPtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        std::unique_ptr<Subtitle> subtitlePtr(new Subtitle);
        if (!m_contextInfo->decodeSubtitle2(subtitlePtr.get(), packetPtr.data())) {
            continue;
        }

        Ffmpeg::calculateTime(packetPtr.data(), m_contextInfo);
        subtitlePtr->setDefault(packetPtr->pts(),
                                packetPtr->duration(),
                                (const char *) packetPtr->avPacket()->data);

        d_ptr->decoderSubtitleFrame->append(subtitlePtr.release());

        while (m_runing && d_ptr->decoderSubtitleFrame->size() > Max_Frame_Size && !m_seek) {
            msleep(Sleep_Queue_Full_Milliseconds);
        }
    }
    while (m_runing && d_ptr->decoderSubtitleFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }
    d_ptr->decoderSubtitleFrame->stopDecoder();
}

} // namespace Ffmpeg
