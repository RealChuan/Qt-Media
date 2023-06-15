# Qt-Ffmpeg

-   [Simplified Chinese](README.md)
-   [English](README.en.md)

## QFfmpegPlayer

<div align=center><img src="doc/player.jpeg"></div>

### Need a powerful opengl and vulkan yuv rendering module!

1.  There are too many if else in the shader in Opengl, causing the GPU to run empty, affecting GPU decoding and av_hwframe_transfer_data speed, this phenomenon is especially obvious on 4K video images;
2.  In WidgetRender, use QImage::Format_RGB32 and QImage::Format_ARGB32_Premultiplied image formats as much as possible. The following reasons:
    1.  Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

### Ffmpeg (5.0) is not the same as 4.4.3 in decoding subtitles

### Decoding subtitles (ffmpeg-n5.0):

    0,,en,,0000,0000,0000,,Peek-a-boo!

you must use`ass_process_chunk`And set pts and duration as in libavfilter/vf_subtitles.c.

ASS standard format should be (ffmpeg-n4.4.3):

    Dialogue: 0,0:01:06.77,0:01:08.00,en,,0000,0000,0000,,Peek-a-boo!\r\n

use`ass_process_data`;

### Issue with subtitle display timing when using subtitle filter

    subtitles=filename='%1':original_size=%2x%3

## QFfmpegTranscoder

How to set encoding parameters to get smaller files and better video quality?

1.  set a very high bitrate;
2.  set encoder`global_quality`invalid. code show as below:

    ```C++
    d_ptr->codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
    d_ptr->codecCtx->global_quality = FF_QP2LAMBDA * quailty;
    ```
3.  set up`crf`invalid. code show as below:

    ```C++
    av_opt_set_int(d_ptr->codecCtx, "crf", crf, AV_OPT_SEARCH_CHILDREN);
    ```

### How to calculate pts from frames acquired by AVAudioFifo?

```C++
// fix me?
frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->sampleRate();
transcodeCtx->audioPts += frame->nb_samples;
```

### [New BING's Video Transcoding Recommendations](./doc/bing_transcode.md)

## SwsContext is great! Compared to QImage converted to and scaled.

## QT-BUG

### Failed to set up resampler

[it's a bug in Qt 6.4.1 on Windows](https://forum.qt.io/topic/140523/qt-6-x-error-message-qt-multimedia-audiooutput-failed-to-setup-resampler)<https://bugreports.qt.io/browse/QTBUG-108383>(johnco3's bug report)<https://bugreports.qt.io/browse/QTBUG-108669>(a duplicate bug report; I filed it before I found any of this)

#### solution:

<https://stackoverflow.com/questions/74500509/failed-to-setup-resampler-when-starting-qaudiosink>

#### Dynamically switch Video Render, switch from opengl to widget, there is still GPU 0-3D occupation, and the usage is twice that of opengl! ! ! QT-BUG?

### QOpenGLWidget memory leak, move zoom in and zoom out window, the code is as follows:

```C++
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setCentralWidget(new QOpenGLWidget(this));
}

```
