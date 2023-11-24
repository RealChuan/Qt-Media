#ifndef UTILS_H
#define UTILS_H

#include "utils_global.h"

#include <QJsonObject>
#include <QtCore>

class QWidget;

namespace Utils {

UTILS_EXPORT void printBuildInfo();
UTILS_EXPORT void setHighDpiEnvironmentVariable();
UTILS_EXPORT void setUTF8Code();
UTILS_EXPORT void setQSS();
UTILS_EXPORT void loadFonts();
UTILS_EXPORT void setGlobalThreadPoolMaxSize(int maxSize = -1);
UTILS_EXPORT void windowCenter(QWidget *child, QWidget *parent);
UTILS_EXPORT void windowCenter(QWidget *window);
UTILS_EXPORT void reboot();
UTILS_EXPORT auto fileSize(const QString &localPath) -> qint64;
UTILS_EXPORT auto generateDirectorys(const QString &directory) -> bool;
UTILS_EXPORT void removeDirectory(const QString &path);
UTILS_EXPORT auto convertBytesToString(qint64 bytes) -> QString;
UTILS_EXPORT auto jsonFromFile(const QString &filePath) -> QJsonObject;
UTILS_EXPORT auto jsonFromBytes(const QByteArray &bytes) -> QJsonObject;
UTILS_EXPORT auto getConfigPath() -> QString;
UTILS_EXPORT auto readAllFile(const QString &filePath) -> QByteArray;
UTILS_EXPORT auto rangeMap(float value, float min, float max, float newMin, float newMax) -> float;

} // namespace Utils

#endif // UTILS_H
