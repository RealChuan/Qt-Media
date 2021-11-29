#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "mainwindow_global.h"

namespace Ffmpeg {
class AVError;
}

class MAINWINDOW_EXPORT MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onError(const Ffmpeg::AVError &averror);
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);

private:
    void setupUI();
    void buildConnect();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};
#endif // MAINWINDOW_H
