#ifndef LOGASYNC_H
#define LOGASYNC_H

#include <QThread>

#include "utils_global.h"

namespace Utils {

struct FileUtilPrivate;
class FileUtil : public QObject
{
    Q_OBJECT
public:
    explicit FileUtil(qint64 days = 30, QObject *parent = nullptr);
    ~FileUtil();

public slots:
    void onWrite(const QString&);

private slots:
    void onFlush();

private:
    void newDir(const QString &);
    QString getFileName(qint64 *now) const;
    bool rollFile(int);
    void autoDelFile();
    void setTimer();

    QScopedPointer<FileUtilPrivate> d;
};

struct LogAsyncPrivate;
class UTILS_EXPORT LogAsync : public QThread
{
    Q_OBJECT
public:
    enum Orientation { Std = 1, File, StdAndFile};

    static LogAsync* instance();

    void setOrientation(Orientation);
    void setLogLevel(QtMsgType);    //日志级别
    void startWork();
    void stop();

signals:
    void appendBuf(const QString&);

protected:
    void run() override;

private:
    LogAsync(QObject *parent = nullptr);
    ~LogAsync() override;

    QScopedPointer<LogAsyncPrivate> d;
};

}

#endif // LOGASYNC_H
