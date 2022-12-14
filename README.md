# Qt-Ffmpeg

## I need a strong opengl yuv rendering module!

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
