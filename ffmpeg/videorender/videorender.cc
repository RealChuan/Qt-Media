#include "videorender.hpp"

#include <ffmpeg/frame.hpp>

#include <QDebug>
#include <QElapsedTimer>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

int fps(int deltaTime) // ms
{
    static float avgDuration = 0.f;
    static float alpha = 1.f / 100.f; // 采样数设置为100
    static int frameCount = 0;

    ++frameCount;

    int fps = 0;
    if (1 == frameCount) {
        avgDuration = static_cast<float>(deltaTime);
    } else {
        avgDuration = avgDuration * (1 - alpha) + deltaTime * alpha;
    }

    fps = static_cast<int>(1.f / avgDuration * 1000);
    return fps;
}

void printFps()
{
    static QElapsedTimer timer;
    if (timer.isValid()) {
        qDebug() << "FPS: " << fps(timer.elapsed());
    }
    timer.restart();
}

VideoRender::VideoRender() {}

VideoRender::~VideoRender() {}

void VideoRender::setFrame(QSharedPointer<Frame> frame)
{
    if (!isSupportedOutput_pix_fmt(AVPixelFormat(frame->avFrame()->format))) {
        frame = convertSupported_pix_fmt(frame);
    }
    updateFrame(frame);
    //qDebug() << frame->avFrame()->format;

    //printFps();
}

} // namespace Ffmpeg
