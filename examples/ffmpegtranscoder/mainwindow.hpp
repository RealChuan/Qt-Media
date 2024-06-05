#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

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
    void onOpenInputFile();
    void onResetConfig();
    void onOpenSubtitle();
    void onStart();

    void onShowMediaInfo();

    void onProcessEvents();

private:
    void setupUI();
    void buildConnect();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};

#endif // MAINWINDOW_HPP
