#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

class QGroupBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onVideoEncoderChanged();
    void onOpenInputFile();
    void onCheckInputFile();
    void onOpenOutputFile();
    void onStart();

private:
    void setupUI();
    QGroupBox *initVideoSetting();
    void buildConnect();

    class MainWindowPrivate;
    QScopedPointer<MainWindowPrivate> d_ptr;
};

#endif // MAINWINDOW_HPP