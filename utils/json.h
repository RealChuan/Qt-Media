#ifndef JSON_H
#define JSON_H

#include <QJsonObject>
#include <QObject>

#include "utils_global.h"

namespace Utils {

struct JsonPrivate;
class UTILS_EXPORT Json : public QObject
{
    Q_OBJECT
public:
    explicit Json(const QString &jsonOrFilePath,
                  bool jsonfile = false,
                  QObject *parent = nullptr);
    ~Json();

    QString errorString() const;

    void setValue(const QString &path, const QVariant &value);
    QVariant getValue(const QString &path,
                      const QJsonObject &fromNode = QJsonObject()) const;

    QString toString(bool pretty = true) const;
    bool save(const QString &path, bool pretty = true) const;

private:
    void loadJson(const QString &jsonOrFilePath, bool jsonfile = false);
    QJsonArray getJsonArray(const QString &path,
                            const QJsonObject &fromNode = QJsonObject()) const;
    QJsonValue getJsonValue(const QString &path,
                            const QJsonObject &fromNode = QJsonObject()) const;
    void setJsonValue(QJsonObject &parent, const QString &path, const QJsonValue &newValue);

    QScopedPointer<JsonPrivate> d;
};

}

#endif // JSON_H
