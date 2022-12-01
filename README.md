# Qt-Ffmpeg

## I need a strong opengl yuv rendering module!

## QFfmpegPlayer


<div align=center><img src="doc/player.png"></div>  

## QOpenGLWidget内存泄漏，移动放大缩小窗口的情况下，代码如下：

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
