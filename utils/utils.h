#ifndef UTILS_H
#define UTILS_H

#include "utils_global.h"

#define TIMEMS qPrintable(QTime::currentTime().toString("HH:mm:ss zzz"))
#define TIME qPrintable(QTime::currentTime().toString("HH:mm:ss"))
#define QDATE qPrintable(QDate::currentDate().toString("yyyy-MM-dd"))
#define QTIME qPrintable(QTime::currentTime().toString("HH-mm-ss"))
#define DATETIME qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
#define STRDATETIME qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"))
#define STRDATETIMEMS qPrintable(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))

#define AppPath qApp->applicationDirPath()

#define ConfigFile "./config/config.ini"

class QWidget;

namespace Utils{

enum Language{
    Chinese,
    English
};

UTILS_EXPORT void printBuildInfo();
UTILS_EXPORT void setHighDpiEnvironmentVariable();
UTILS_EXPORT void setUTF8Code();
UTILS_EXPORT void setQSS();
UTILS_EXPORT void loadFonts();
UTILS_EXPORT void windowCenter(QWidget *window);
UTILS_EXPORT void msleep(int msec);
UTILS_EXPORT void reboot();
UTILS_EXPORT void saveLanguage(Language);
UTILS_EXPORT void loadLanguage();
UTILS_EXPORT Language getCurrentLanguage();
UTILS_EXPORT bool createPath(const QString& path);

}


#endif // UTILS_H
