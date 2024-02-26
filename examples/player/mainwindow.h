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
    void onShowColorSpace();
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
    void setupUI();
    void buildConnect();
    void initMenu();
    void tonemapMenu();
    void destPrimarisMenu();
    void renderMenu();
    void initPlayListMenu();
    void addToPlaylist(const QList<QUrl> &urls);

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};

#endif // MAINWINDOW_H
