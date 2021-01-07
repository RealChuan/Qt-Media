#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindowPrivate;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onError(const QString &error);

private:
    void setupUI();

    QScopedPointer<MainWindowPrivate> d_ptr;
};
#endif // MAINWINDOW_H
