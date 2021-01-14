#include "logasync.h"

#include <QDateTime>
#include <QWaitCondition>
#include <QMutex>
#include <QFileInfoList>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>
#include <QTimer>

namespace Utils {

#define ROLLSIZE 1000*1000*1000

const static int kRollPerSeconds_ = 60*60*24;

struct FileUtilPrivate{
    QFile file;
    QTextStream stream; //QTextStream 读写分离的，内部有缓冲区static const int QTEXTSTREAM_BUFFERSIZE = 16384;
    qint64 startTime = 0;
    qint64 lastRoll = 0;
    int count = 0;
    qint64 autoDelFileDays = 30;
};

FileUtil::FileUtil(qint64 days, QObject *parent)
    : QObject(parent)
    , d(new FileUtilPrivate)
{
    d->autoDelFileDays = days;
    newDir("log");
    rollFile(0);
    setTimer();
}

FileUtil::~FileUtil()
{
    onFlush();
}

void FileUtil::onWrite(const QString &msg)
{
    if(d->file.size() > ROLLSIZE){
        rollFile(++d->count);
    }else{
        qint64 now = QDateTime::currentSecsSinceEpoch();
        qint64 thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
        if(thisPeriod != d->startTime){
            d->count = 0;
            rollFile(0);
            autoDelFile();
        }
    }

    d->stream << msg;
    //d->file.write(msg.toUtf8().constData());
}

void FileUtil::onFlush()
{
    d->stream.flush();
}

void FileUtil::newDir(const QString &path)
{
    QDir dir;
    if(!dir.exists(path))
        dir.mkdir(path);
}

QString FileUtil::getFileName(qint64* now) const
{
    *now = QDateTime::currentSecsSinceEpoch();
    QString data = QDateTime::fromSecsSinceEpoch(*now).toString("yyyy-MM-dd-hh-mm-ss");
    QString filename = QString("./log/%1.%2.%3.%4.log").arg(qAppName()).
                       arg(data).arg(QSysInfo::machineHostName()).arg(qApp->applicationPid());
    return filename;
}

bool FileUtil::rollFile(int count)
{
    qint64 now = 0;
    QString filename = getFileName(&now);
    if(count){
        filename += QString(".%1").arg(count);
    }
    qint64 start = now / kRollPerSeconds_ * kRollPerSeconds_;
    if (now > d->lastRoll){
        d->startTime = start;
        d->lastRoll = now;
        if(d->file.isOpen()){
            d->file.flush();
            d->file.close();
        }
        d->file.setFileName(filename);
        d->file.open(QIODevice::WriteOnly| QIODevice::Append| QIODevice::Unbuffered);
        d->stream.setDevice(&d->file);
        fprintf(stderr, "%s\n", filename.toUtf8().constData());
        return true;
    }
    return false;
}

void FileUtil::autoDelFile()
{
    newDir("log");
    QDir dir("./log/");

    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    QDateTime cur = QDateTime::currentDateTime();
    QDateTime pre = cur.addDays(-d->autoDelFileDays);

    for(QFileInfo info : list){
        QDateTime birthTime = info.lastModified();
        if(birthTime <= pre)
            dir.remove(info.fileName());
    }
}

void FileUtil::setTimer()
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FileUtil::onFlush);
    timer->start(5000); // 5秒刷新一次
}

static QtMsgType g_msgType = QtWarningMsg;
static QMutex g_mutex;
static LogAsync::Orientation g_orientation = LogAsync::Orientation::Std;

// 消息处理函数
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if(type < g_msgType)
        return;

    QString level;
    switch(type){
    case QtDebugMsg: level = QLatin1String("Debug"); break;
    case QtWarningMsg: level = QLatin1String("Warning"); break;
    case QtCriticalMsg: level = QLatin1String("Critica"); break;
    case QtFatalMsg: level = QLatin1String("Fatal"); break;
    case QtInfoMsg: level = QLatin1String("Info"); break;
    default: level = QLatin1String("Unknown"); break;
    }

    const QString dataTimeString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
    const QString threadId(QString::number(qulonglong(QThread::currentThreadId())));
    QString contexInfo;
#ifndef QT_NO_DEBUG
    contexInfo = QString("File:(%1) Line:(%2)").arg(context.file).arg(context.line);
#endif
    const QString message = QString("%1 %2 [%3] %4 - %5\n")
                                .arg(dataTimeString, threadId, level, msg, contexInfo);

    switch (g_orientation) {
    case LogAsync::Orientation::Std:
        fprintf(stderr, "%s", message.toLocal8Bit().constData());
        break;
    case LogAsync::Orientation::File:
        LogAsync::instance()->appendBuf(message);
        break;
    case LogAsync::Orientation::StdAndFile:
        fprintf(stderr, "%s", message.toLocal8Bit().constData());
        LogAsync::instance()->appendBuf(message);
        break;
    default:
        fprintf(stderr, "%s", message.toLocal8Bit().constData());
        break;
    }
}

struct LogAsyncPrivate{
    QWaitCondition waitCondition;
    QMutex mutex;
};

LogAsync *LogAsync::instance()
{
    QMutexLocker locker(&g_mutex);
    static LogAsync log;
    return &log;
}

void LogAsync::setOrientation(LogAsync::Orientation orientation)
{
    g_orientation = orientation;
}

void LogAsync::setLogLevel(QtMsgType type)
{
    g_msgType = type;
}

void LogAsync::startWork()
{
    start();
    QMutexLocker lock(&d->mutex);
    d->waitCondition.wait(&d->mutex);
}

void LogAsync::stop()
{
    if(isRunning()){
        QThread::sleep(1);   // 最后一条日志格式化可能来不及进入信号槽
        quit();
        wait();
    }
}

void LogAsync::run()
{
    FileUtil fileUtil;
    connect(this, &LogAsync::appendBuf, &fileUtil, &FileUtil::onWrite);
    d->waitCondition.wakeOne();
    exec();
}

LogAsync::LogAsync(QObject *parent)
    : QThread(parent)
    , d(new LogAsyncPrivate)
{
    qInstallMessageHandler(messageHandler);
}

LogAsync::~LogAsync()
{
    stop();
    qInstallMessageHandler(nullptr);
    fprintf(stderr, "%s\n", "~LogAsync");
}

}
