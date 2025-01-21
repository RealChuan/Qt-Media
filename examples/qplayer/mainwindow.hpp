#pragma once

#include <QMainWindow>
#include <QMediaPlayer>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenLocalMedia();
    void onOpenWebMedia();

    void onTrackChanged();
    void onBufferingProgress(float progress);
    void onDisplayError();
    void onStatusChanged(QMediaPlayer::MediaStatus status);

    void onAudioOutputsChanged();

    void playlistPositionChanged(int /*currentItem*/);
    void jump(const QModelIndex &index);

protected:
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildConnect();
    void initMenu();
    void initPlayListMenu();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};
