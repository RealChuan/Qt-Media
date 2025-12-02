#include "mainwindow.hpp"

#include <utils/logasync.h>
#include <utils/utils.hpp>

#include <QApplication>

int main(int argc, char *argv[])
{
    Utils::setHighDpiEnvironmentVariable();

    QApplication a(argc, argv);

    // 异步日志
    Utils::LogAsync *log = Utils::LogAsync::instance();
    log->setOrientation(Utils::LogAsync::Orientation::StandardAndFile);
    log->setLogLevel(QtDebugMsg);
    log->startWork();

    MainWindow w;
    w.show();

    auto ret = a.exec();
    log->stop();

    return ret;
}
