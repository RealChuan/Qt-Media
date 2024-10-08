#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ffmpeg {
class AVError;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onHoverSlider(int pos, int value);
    void onLeaveSlider();
    void onShowCurrentFPS();
    void onEqualizer();
    void onShowMediaInfo();

    void onOpenLocalMedia();
    void onOpenWebMedia();
    void onRenderChanged(QAction *action);

    void playlistPositionChanged(int /*currentItem*/);
    void jump(const QModelIndex &index);

    void onProcessEvents();

protected:
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildConnect();
    void initMenu();
    void renderMenu();
    void initPlayListMenu();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};

#endif // MAINWINDOW_H
