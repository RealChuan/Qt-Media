#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpenLocalMedia();
    void onOpenWebMedia();
    void onLoadSubtitleFiles();
    void onShowSubtitleDelayDialog();
    void onFileLoaded();
    void onTrackChanged();
    void onChapterChanged();
    void onRenderChanged(QAction *action);
    void onEqualizer();

    void onPreview(int pos, int value);
    void onPreviewFinish();

    void playlistPositionChanged(int);
    void jump(const QModelIndex &index);

protected:
    auto eventFilter(QObject *watched, QEvent *event) -> bool override;
    void keyPressEvent(QKeyEvent *ev) override;

private:
    void buildConnect();
    void initMenu();
    void initPlayListMenu();
    void addToPlaylist(const QList<QUrl> &urls);

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};
#endif // MAINWINDOW_HPP
