#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "mainwindow_global.h"

class MainWindowPrivate;
class MAINWINDOW_EXPORT MainWindow : public QMainWindow
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
