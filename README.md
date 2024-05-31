# Qt-Ffmpeg

- [简体中文](README.md)
- [English](README.en.md)

## Player

<div align=center>
<img src="doc/player.jpeg" width="90%" height="90%">
</div>

### 需要一个强大的opengl和vulkan yuv渲染模块

1. Opengl的片段着色器目前支持的图像格式有限；
2. 在WidgetRender中，尽可能使用QImage::Format_RGB32和QImage::Format_ARGB32_Premultiplied图像格式。如下原因：
   1. Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

### AVFrame 图像调整

1. 根据`AVColorSpace`进行色彩空间转换；
2. 根据`AVColorTransferCharacteristic`进行gamma、PQ、HLG等调整;
3. 根据`AVColorPrimaries`进行色域转换；
4. 根据`AVColorRange`进行色彩范围调整;

#### 1. opengl 渲染的情况下，该怎么样修改shader？

1. 参考[MPV video_shaders](https://github.com/mpv-player/mpv/blob/master/video/out/gpu/video_shaders.c)；

#### 2. 非opengl渲染的情况下，又该怎么样添加filter实现图像补偿？

```bash
zscale=p=709;
```

### OpenGL 渲染图像，怎么实现画质增强的效果？

### Ffmpeg（5.0）在解码字幕与4.4.3不太一样

#### 解码字幕(ffmpeg-n5.0)

```bash
0,,en,,0000,0000,0000,,Peek-a-boo!
```

你必须使用 ``ass_process_chunk`` 并设置 pts 和持续时间, 和在[vf_subtitles.c](https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_subtitles.c#L490) 中一样。

#### ASS 标准格式应为(ffmpeg-n4.4.3)

```bash
Dialogue: 0,0:01:06.77,0:01:08.00,en,,0000,0000,0000,,Peek-a-boo!\r\n
```

使用 ``ass_process_data``;

### 使用字幕过滤器时，字幕显示时间有问题

```bash
subtitles=filename='%1':original_size=%2x%3
```

## Transcoder

如何设置编码参数以获得更小的文件和更好的视频质量？

1. 参考[HandBrake encavcodec](https://github.com/HandBrake/HandBrake/blob/master/libhb/encavcodec.c#L359)

### 如何从AVAudioFifo获取的帧中计算pts？

```C++
// fix me?
frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->sampleRate();
transcodeCtx->audioPts += frame->nb_samples;
```

### [New BING的视频转码建议](./doc/bing_transcode.md)

## SwsContext很棒！与 QImage 转换为和缩放相比

## QT-BUG

### 动态切换Video Render，从opengl切换到widget，还是有GPU 0-3D占用，而且使用量是opengl的2倍！！！QT-BUG？

### QOpenGLWidget内存泄漏，移动放大和缩小窗口，代码如下

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
