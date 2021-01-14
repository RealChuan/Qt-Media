#include "json.h"

#include <QJsonParseError>
#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QRegularExpression>

namespace Utils {

struct JsonPrivate{
    bool jsonLoad = false;
    QJsonDocument jsonDoc;  // Json的文档对象
    QJsonObject rootObj;    // 根节点
    QString errorString;
};

Json::Json(const QString &jsonOrFilePath, bool jsonfile, QObject *parent)
    : QObject(parent)
    , d(new JsonPrivate)
{
    loadJson(jsonOrFilePath, jsonfile);
}

Json::~Json()
{
}

QString Json::errorString() const
{
    return d->errorString;
}

void Json::setValue(const QString &path, const QVariant &value)
{
    setJsonValue(d->rootObj, path, value.toJsonValue());
}

QVariant Json::getValue(const QString &path, const QJsonObject &fromNode) const
{
    if(!d->jsonLoad)
        return QVariant();
    return getJsonValue(path, fromNode).toVariant();
}

QString Json::toString(bool pretty) const
{
    return QJsonDocument(d->rootObj).toJson(pretty ? QJsonDocument::Indented : QJsonDocument::Compact);
}

bool Json::save(const QString &path, bool pretty) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly
                   | QIODevice::Truncate
                   | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << toString(pretty);
    out.flush();
    file.close();
    return true;
}

void Json::loadJson(const QString &jsonOrFilePath, bool jsonfile)
{
    QByteArray json;
    if(jsonfile){
        QFile file(jsonOrFilePath);
        if (!file.open(QIODevice::ReadOnly
                       | QIODevice::Text)){
            d->errorString = QString(tr("Cannot open the file: %1")).
                    arg(jsonOrFilePath);
            qDebug() << d->errorString;
            return;
        }
        json = file.readAll();
        file.close();
    }else
        json = jsonOrFilePath.toUtf8();

    // 解析 Json
    QJsonParseError jsonError;
    d->jsonDoc = QJsonDocument::fromJson(json, &jsonError);
    if (QJsonParseError::NoError == jsonError.error){
        d->rootObj = d->jsonDoc.object();
        d->jsonLoad = true;
    }else{
        d->errorString = QString(tr("%1\nOffset: %2"))
                .arg(jsonError.errorString())
                .arg(jsonError.offset);
        qDebug() << d->errorString;
    }
}

QJsonArray Json::getJsonArray(const QString &path, const QJsonObject &fromNode) const
{
    // 如果根节点是数组时特殊处理
    if (("." == path || "" == path) && fromNode.isEmpty())
        return d->jsonDoc.array();
    return getJsonValue(path, fromNode).toArray();
}

QJsonValue Json::getJsonValue(const QString &path, const QJsonObject &fromNode) const
{
    QJsonObject parent = fromNode.isEmpty() ? d->rootObj : fromNode;
    QStringList names = path.split(QRegularExpression("\\."));

    int size = names.size();
    for (int i=0; i<size-1; ++i) {
        if (parent.isEmpty()){
            d->errorString = QString(tr("%1 is empty!").arg(names.at(i)));
            qDebug() << d->errorString;
            return QJsonValue();
        }
        parent = parent.value(names.at(i)).toObject();
    }
    return parent.value(names.last());
}

void Json::setJsonValue(QJsonObject &parent, const QString &path, const QJsonValue &newValue)
{
    const int indexOfDot = path.indexOf('.');     // 第一个 . 的位置
    const QString property = path.left(indexOfDot); // 第一个 . 之前的内容，如果 indexOfDot 是 -1 则返回整个字符串
    const QString restPath = (indexOfDot>0) ? path.mid(indexOfDot+1) : QString(); // 第一个 . 后面的内容

    QJsonValue fieldValue = parent[property];

    if(restPath.isEmpty()) {
        // 找到要设置的属性
        fieldValue = newValue;
    } else {
        // 路径中间的属性，递归访问它的子属性
        QJsonObject obj = fieldValue.toObject();
        setJsonValue(obj, restPath, newValue);
        fieldValue = obj; // 因为 QJsonObject 操作的都是对象的副本，所以递归结束后需要保存起来再次设置回 parent
    }

    parent[property] = fieldValue; // 如果不存在则会创建
}

}
