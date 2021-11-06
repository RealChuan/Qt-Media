#include <crashhandler/crashhandler.h>
#include <mainwindow/mainwindow.h>
#include <utils/logasync.h>
#include <utils/utils.h>

#include <QApplication>
#include <QDir>
#include <QThreadPool>

int main(int argc, char *argv[])
{
    Utils::setHighDpiEnvironmentVariable();

    QApplication a(argc, argv);

    Utils::setCrashHandler();
    Utils::setGlobalThreadPoolMaxSize();

    QDir::setCurrent(a.applicationDirPath());
    Utils::loadLanguage();

    // 异步日志
    Utils::LogAsync *log = Utils::LogAsync::instance();
    log->setOrientation(Utils::LogAsync::Orientation::StdAndFile);
    log->setLogLevel(QtDebugMsg);
    log->startWork();

    Utils::printBuildInfo();

    a.setApplicationVersion(QObject::tr("0.0.1"));
    a.setApplicationDisplayName(QObject::tr("Ffmpeg Player"));
    a.setApplicationName(QObject::tr("Ffmpeg Player"));
    a.setDesktopFileName(QObject::tr("Ffmpeg Player"));
    a.setOrganizationDomain(QObject::tr("Youth"));
    a.setOrganizationName(QObject::tr("Youth"));

    MainWindow w;
    w.show();

    int result = a.exec();
    log->stop();
    return result;
}
