# Qt-Ffmpeg

## Requires a robust opengl and vulkan yuv rendering module!

1. Opengl在传输4K图像时似乎很慢，在同样的情况下使用WidgetRender的效果要比OpenglRender好，当然SwsContext在WidgetRender中起着非常重要的作用。  
2. 在WidgetRender中，使用QImage::Format_RGB32和QImage::Format_ARGB32_Premultiplied图像格式。如下原因：  
   1.  Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

## SwsContext is awesome! Compared to QImage convertTo and scaled.

## QFfmpegPlayer

<div align=center><img src="doc/player.png"></div>

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
