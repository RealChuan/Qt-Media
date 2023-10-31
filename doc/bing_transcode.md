# 视频编码的基本概念和参数

## 什么是视频编码？

视频编码是将原始视频文件转换为数字格式，以便与网络播放器和移动设备兼容的过程。视频编码的过程由视频编解码器（codec）控制，视频编解码器是一种压缩和解压缩视频的算法。视频编码的目的是在保证视频质量的同时，减少视频文件的大小，节省存储空间和传输带宽。

## 视频编码的常用参数

视频编码的过程中，有一些参数可以影响视频的质量、速度、场景和兼容性。这些参数可以根据不同的需求和场合进行调整，以达到最佳的效果。以下是一些常用的视频编码参数：

### crf

crf（constant rate factor）是一种质量控制模式，可以根据视频内容的复杂度动态调整量化因子，从而达到一致的输出质量。crf的取值范围是0-51，其中0是无损，23是默认值，51是最差质量。一般来说，crf值越低，输出质量越高，输出文件大小也越大。

- 建议：选择一个能够提供可接受质量的最高crf值。如果输出看起来不错，那么尝试一个更高的值；如果输出看起来不好，那么选择一个更低的值。
- 示例代码：

```c
// Set the crf for the output
av_opt_set(enc_ctx->priv_data, "crf", "22", 0);
```

### preset

preset（预设）是一种速度控制模式，可以根据不同的预设名称选择不同的编码速度和压缩效率。预设名称有ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, placebo等，其中ultrafast是最快但压缩效率最低的，placebo是最慢但压缩效率最高的。一般来说，preset值越慢，输出质量越高，输出文件大小也越小。

- 建议：选择一个能够忍受的最慢preset值。忽略placebo选项，因为它提供了微不足道的收益，而增加了很大的编码时间。
- 示例代码：

```c
// Set the preset for the output
av_opt_set(enc_ctx->priv_data, "preset", "slow", 0);
```

### tune

tune（调优）是一种场景控制模式，可以根据不同的视频特点选择不同的编码参数。tune选项有psnr, ssim, grain, zerolatency, fastdecode等，其中psnr和ssim是为了提高客观质量指标，grain是为了保留视频的颗粒感，zerolatency是为了减少编码延迟，fastdecode是为了减少解码复杂度。

- 建议：默认情况下，tune选项是禁用的，并且通常不需要设置tune选项。如果有特殊需求或场景，可以根据实际情况选择合适的tune选项。
- 示例代码：

```c
// Set the tune for the output
av_opt_set(enc_ctx->priv_data, "tune", "zerolatency", 0);
```

### profile

profile（配置文件）是一种兼容性控制模式，可以根据不同的视频标准选择不同的编码特性。profile选项有baseline, main, high等，其中baseline是为了支持低端设备和网络传输，main是为了支持主流设备和网络传输，high是为了支持高端设备和网络传输。

- 建议：选择一个能够满足目标设备和网络的最高profile值。如果不确定，可以使用main或high选项。
- 示例代码：

```c
// Set the profile for the output
av_opt_set(enc_ctx->priv_data, "profile", "high", 0);
```

## 视频编码参数之间的关系和影响

视频编码参数之间有一定的关系和影响，例如：

- crf和preset之间有一个权衡关系，如果想要保持相同的输出质量，那么preset越慢，crf就可以越高；如果想要保持相同的输出文件大小，那么preset越慢，crf就要越低。
- tune和profile之间有一定的限制关系，例如zerolatency只能用于baseline或main profile。
- resolution, bitrate, framerate, codec等参数也会影响视频的质量、速度、场景和兼容性。resolution表示视频的宽度和高度，影响视频的清晰度和细节；bitrate表示视频每秒传输的数据量，影响视频的流畅度和压缩效率；framerate表示视频每秒显示的帧数，影响视频的动态效果和流畅度；codec表示视频的压缩和解压缩算法，影响视频的兼容性和压缩效率。

### resolution

resolution（分辨率）表示视频的宽度和高度，影响视频的清晰度和细节。分辨率越高，视频质量越好，但也需要更多的比特率和存储空间。

- 建议：选择一个能够满足目标设备和网络的最高resolution值。如果不确定，可以使用常用的分辨率，例如720p（1280x720）或1080p（1920x1080）。
- 示例代码：

```c
// Set the resolution for the output
enc_ctx->height = 720;
enc_ctx->width = 1280;
```

### bitrate

bitrate（比特率）表示视频每秒传输的数据量，影响视频的流畅度和压缩效率。比特率越高，视频质量越好，但也需要更多的存储空间和传输带宽。

- 建议：选择一个能够提供可接受质量的最低bitrate值。如果不确定，可以使用一些经验公式或在线计算器来估算合适的bitrate值。
- 示例代码：

```c
// Set the bitrate for the output
enc_ctx->bit_rate = 2000000;
```

### framerate

framerate（帧率）表示视频每秒显示的帧数，影响视频的动态效果和流畅度。帧率越高，视频越流畅，但也需要更多的比特率和处理能力。

- 建议：选择一个能够满足目标设备和网络的最高framerate值。如果不确定，可以使用常用的帧率，例如24, 25, 30, 60等。
- 示例代码：

```c
// Set the framerate for the output
enc_ctx->framerate = (AVRational){30, 1};
```

### codec

codec（编解码器）表示视频的压缩和解压缩算法，影响视频的兼容性和压缩效率。不同的编解码器有不同的特点和优势，例如H.264是最广泛使用的编解码器，H.265是最新的编解码器，可以提供更高的压缩效率。

- 建议：选择一个能够满足目标设备和网络的最优codec值。如果不确定，可以使用H.264或H.265编解码器。
- 示例代码：

```c
// Initialize the encoder context with the codec
AVCodecContext *enc_ctx = avcodec_alloc_context3(enc_codec);
if (!enc_ctx) {
    ret = AVERROR(ENOMEM);
    goto end;
}

ret = avcodec_open2(enc_ctx, enc_codec, NULL);
if (ret < 0) {
    fprintf(stderr, "Cannot open video encoder for stream #%u\n", stream_index);
    return ret;
}
```

## 视频编码的示例命令

除了用C语言编写示例代码之外，也可以用ffmpeg这个工具来执行视频编码的操作。ffmpeg是一个强大的视频处理工具，可以进行视频的转换、剪辑、滤镜等操作。以下是一些用ffmpeg展示使用视频编码参数的示例命令：

- 使用crf, preset, tune, profile参数的示例命令：

```bash
# Encode input.mp4 to output.mkv with crf=22, preset=slow, tune=zerolatency, profile=high
ffmpeg -i input.mp4 -c:v libx264 -crf 22 -preset slow -tune zerolatency -profile:v high output.mkv
```

- 使用resolution, bitrate, framerate, codec参数的示例命令：

```bash
# Encode input.mp4 to output.mkv with resolution=1280x720, bitrate=2000000, framerate=30, codec=libx265
ffmpeg -i input.mp4 -c:v libx265 -s 1280x720 -b:v 2000000 -r 30 output.mkv
```
