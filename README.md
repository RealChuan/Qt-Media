# Qt-Ffmpeg

## Requires a robust opengl and vulkan yuv rendering module!

1. Opengl中的shader有太多if else导致GPU空跑，影响GPU解码和av_hwframe_transfer_data速度，这个现象在4K视频图像上尤为明显；
2. 在WidgetRender中，尽可能使用QImage::Format_RGB32和QImage::Format_ARGB32_Premultiplied图像格式。如下原因：
   1. Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

## Ffmpeg (version 5.0) has problems decoding ass subtitles

Decoded subtitles(ffmpeg-n5.0):

```
0,,en,,0000,0000,0000,,Peek-a-boo!
```

You must use ```ass_process_chunk``` and set pts and duration, as in libavfilter/vf_subtitles.c.

The ASS standard format should be (ffmpeg-n4.4.3) :

```
Dialogue: 0,0:01:06.77,0:01:08.00,en,,0000,0000,0000,,Peek-a-boo!\r\n
```

use ```ass_process_data```;

## With the subtitle filter, there is a problem with the subtitle display time

```
subtitles=filename='%1':original_size=%2x%3
```

## How to set the encoding parameters to get smaller file size and better quality video?

1. Set a very high bit rate;
2. Setting encoder global_quality is invalid.The code is as follows:

   ```C++
   d_ptr->codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
   d_ptr->codecCtx->global_quality = FF_QP2LAMBDA * quailty;
   ```
3. Setting crf is invalid. The code is as follows：

   ```C++
   av_opt_set_int(d_ptr->codecCtx, "crf", crf, AV_OPT_SEARCH_CHILDREN);
   ```

## How to compute pts from frames taken from AVAudioFifo?

```C++
// fix me?
frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->sampleRate();
transcodeCtx->audioPts += frame->nb_samples;
```

## Failed to set up resampler
[it's a bug in Qt 6.4.1 on Windows](https://forum.qt.io/topic/140523/qt-6-x-error-message-qt-multimedia-audiooutput-failed-to-setup-resampler)
https://bugreports.qt.io/browse/QTBUG-108383 (johnco3's bug report)
https://bugreports.qt.io/browse/QTBUG-108669 (a duplicate bug report; I filed it before I found any of this) 
### solution:  
https://stackoverflow.com/questions/74500509/failed-to-setup-resampler-when-starting-qaudiosink

### 动态切换Video Render，从opengl切换到widget，还是有GPU 0-3D占用，而且使用量是opengl的2倍！！！QT-BUG？

## QOpenGLWidget memory leak, move zoom-in and zoom-out window, code as follows:

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

## SwsContext is awesome! Compared to QImage convertTo and scaled.

## QFfmpegPlayer
### BUG：
1. 全屏状态下，右键菜单及其他控件不显示，需要还原成默认窗口大小，再全屏，之后正常？  

<div align=center><img src="doc/player.png"></div>
