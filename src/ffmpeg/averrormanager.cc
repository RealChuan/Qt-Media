#include "averrormanager.hpp"
#include "averror.h"

namespace Ffmpeg {

class AVErrorManager::AVErrorManagerPrivate
{
public:
    explicit AVErrorManagerPrivate(AVErrorManager *q)
        : q_ptr(q)
    {}

    AVErrorManager *q_ptr;

    bool print = false;
    int max = 100;
    QList<int> errorCodes{};
    AVError error;
};

void AVErrorManager::setPrint(bool print)
{
    d_ptr->print = print;
}

void AVErrorManager::setMaxCaches(int max)
{
    Q_ASSERT(max > 0);
    d_ptr->max = max;
}

auto AVErrorManager::setFuncInfo(const QString &funcInfo) -> AVErrorManager &
{
    d_ptr->error.setFuncInfo(funcInfo);
    return *this;
}

auto AVErrorManager::setErrorCode(int errorCode) -> AVErrorManager &
{
    d_ptr->errorCodes.append(errorCode);
    d_ptr->error.setErrorCode(errorCode);
    if (d_ptr->errorCodes.size() > d_ptr->max) {
        d_ptr->errorCodes.takeFirst();
    }
    if (d_ptr->print) {
        qWarning() << QString("Error[%1]:%2.")
                          .arg(QString::number(d_ptr->error.errorCode()),
                               d_ptr->error.errorString());
    }
    emit error(d_ptr->error);
    return *this;
}

auto AVErrorManager::lastErrorString() const -> QString
{
    return d_ptr->error.errorString();
}

auto AVErrorManager::errorCodes() const -> QList<int>
{
    return d_ptr->errorCodes;
}

AVErrorManager::AVErrorManager(QObject *parent)
    : QObject{parent}
    , d_ptr(new AVErrorManagerPrivate(this))
{
    qRegisterMetaType<Ffmpeg::AVError>("Ffmpeg::AVError");
}

AVErrorManager::~AVErrorManager() = default;

} // namespace Ffmpeg
