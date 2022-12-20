#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mainwindow_global.h"

#include <QMainWindow>

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
    void onStarted();
    void onFinished();
    void onHoverSlider(int pos, int value);
    void onLeaveSlider();
    void onShowCurrentFPS();

    void onOpenLocalMedia();
    void onOpenWebMedia();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;

private:
    void setupUI();
    void buildConnect();
    void initShortcut();
    void initMenu();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};
#endif // MAINWINDOW_H
