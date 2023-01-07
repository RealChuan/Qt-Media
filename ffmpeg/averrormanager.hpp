#ifndef AVERRORMANAGER_HPP
#define AVERRORMANAGER_HPP

#include "ffmepg_global.h"

#include <utils/singleton.hpp>

namespace Ffmpeg {

class AVError;

class FFMPEG_EXPORT AVErrorManager : public QObject
{
    Q_OBJECT
public:
    void setPrint(bool print);
    void setMaxCaches(int max);

    void setErrorCode(int errorCode);
    QString lastErrorString() const;
    QVector<int> errorCodes() const;

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    AVErrorManager(QObject *parent = nullptr);
    ~AVErrorManager();

    class AVErrorManagerPrivate;
    QScopedPointer<AVErrorManagerPrivate> d_ptr;

    SINGLETON(AVErrorManager)
};

} // namespace Ffmpeg

#endif // AVERRORMANAGER_HPP
