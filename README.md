# Qt-Ffmpeg

- [简体中文](README.md)
- [English](README.en.md)

## QFfmpegPlayer

<div align=center><img src="doc/player.jpeg"></div>

### 需要一个强大的opengl和vulkan yuv渲染模块

1. Opengl的片段着色器目前支持的图像格式有限；
2. 在WidgetRender中，尽可能使用QImage::Format_RGB32和QImage::Format_ARGB32_Premultiplied图像格式。如下原因：
   1. Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

### 如何根据AVColorTransferCharacteristic调整图像？

#### 1. opengl 渲染的情况下，该怎么样修改shader？

#### 2. 非opengl渲染的情况下，又该怎么样添加filter实现图像补偿？

1. 参考[MPV video_shaders](https://github.com/mpv-player/mpv/blob/master/video/out/gpu/video_shaders.c#L341)，效果也不是很好；应该是哪里有遗漏。

```cpp
void pass_linearize(struct gl_shader_cache *sc, enum mp_csp_trc trc);
void pass_delinearize(struct gl_shader_cache *sc, enum mp_csp_trc trc);
```

2. 在NV12的shader上，参考MPV实现的对SMPTE2084进行图像调整shader

```glsl
#version 330 core

in vec2 TexCord;    // 纹理坐标
out vec4 FragColor; // 输出颜色

uniform sampler2D tex_y;
uniform sampler2D tex_uv;

uniform vec3 offset;
uniform mat3 colorConversion;

const float PQ_M1 = 2610. / 4096 * 1. / 4, PQ_M2 = 2523. / 4096 * 128, PQ_C1 = 3424. / 4096,
            PQ_C2 = 2413. / 4096 * 32, PQ_C3 = 2392. / 4096 * 32;

#define MP_REF_WHITE 203.0
#define MP_REF_WHITE_HLG 3.17955

void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(tex_y, TexCord).r;
    yuv.yz = texture(tex_uv, TexCord).rg;

    yuv += offset;
    rgb = yuv * colorConversion;

    vec4 color = vec4(rgb, 1.0);
    // ------------------
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = pow(color.rgb, vec3(1.0 / PQ_M2));
    color.rgb = max(color.rgb - vec3(PQ_C1), vec3(0.0)) / (vec3(PQ_C2) - vec3(PQ_C3) * color.rgb);
    color.rgb = pow(color.rgb, vec3(1.0 / PQ_M1));
    // ------------------
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = pow(color.rgb, vec3(PQ_M1));
    color.rgb = (vec3(PQ_C1) + vec3(PQ_C2) * color.rgb) / (vec3(1.0) + vec3(PQ_C3) * color.rgb);
    color.rgb = pow(color.rgb, vec3(PQ_M2));
    // ------------------
    rgb = color.rgb;

    FragColor = vec4(rgb, 1.0);
}
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
