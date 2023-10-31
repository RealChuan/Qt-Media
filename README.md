# Qt-Ffmpeg

- [简体中文](README.md)
- [English](README.en.md)

## QFfmpegPlayer

<div align=center><img src="doc/player.jpeg"></div>

### 需要一个强大的opengl和vulkan yuv渲染模块

1. Opengl中的shader有太多if else导致GPU空跑，影响GPU解码和av_hwframe_transfer_data速度，这个现象在4K视频图像上尤为明显；
2. 在WidgetRender中，尽可能使用QImage::Format_RGB32和QImage::Format_ARGB32_Premultiplied图像格式。如下原因：
   1. Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

### 如何根据AVColorTransferCharacteristic调整图像？

#### 1. opengl 渲染的情况下，该怎么样修改shader？

#### 2. 非opengl渲染的情况下，又该怎么样添加filter实现图像补偿？

1. 现在对AVCOL_TRC_SMPTE2084调整了对比度，饱和度，亮度；效果并不是很好。
2. 如果不调整，就跟ffplay输出图像效果一致，整体颜色偏暗。Netflix的视频，视频开头的N，显示的颜色偏暗黄色。

```bash
contrast = 1.4;
saturation = 0.9;
brightness = 0;
```

3. 参考[MPV video_shaders](https://github.com/mpv-player/mpv/blob/master/video/out/gpu/video_shaders.c#L341)，效果也不是很好；应该是哪里有遗漏。

```cpp
void pass_linearize(struct gl_shader_cache *sc, enum mp_csp_trc trc);
void pass_delinearize(struct gl_shader_cache *sc, enum mp_csp_trc trc);
```

### OpenGL 渲染图像，怎么实现画质增强的效果？

### Ffmpeg（5.0）在解码字幕与4.4.3不太一样

#### 解码字幕(ffmpeg-n5.0)

```bash
0,,en,,0000,0000,0000,,Peek-a-boo!
```

你必须使用 ``ass_process_chunk`` 并设置 pts 和持续时间, 如在 libavfilter/vf_subtitles.c 中一样。

#### ASS 标准格式应为(ffmpeg-n4.4.3)

```bash
Dialogue: 0,0:01:06.77,0:01:08.00,en,,0000,0000,0000,,Peek-a-boo!\r\n
```

使用 ``ass_process_data``;

### 使用字幕过滤器时，字幕显示时间有问题

```bash
subtitles=filename='%1':original_size=%2x%3
```

## QFfmpegTranscoder

如何设置编码参数以获得更小的文件和更好的视频质量？

1. 设置非常高的比特率;
2. 设置编码器 ``global_quality`` 无效。代码如下：

   ```C++
   d_ptr->codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
   d_ptr->codecCtx->global_quality = FF_QP2LAMBDA * quailty;
   ```

3. 设置 ``crf`` 无效。代码如下：

   ```C++
   av_opt_set_int(d_ptr->codecCtx, "crf", crf, AV_OPT_SEARCH_CHILDREN);
   ```

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

#### 动态切换Video Render，从opengl切换到widget，还是有GPU 0-3D占用，而且使用量是opengl的2倍！！！QT-BUG？

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
