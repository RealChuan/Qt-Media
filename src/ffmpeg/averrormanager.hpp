#ifndef AVERRORMANAGER_HPP
#define AVERRORMANAGER_HPP

#include "ffmepg_global.h"

#include <utils/singleton.hpp>

#define SET_ERROR_CODE(errorCode) \
    AVErrorManager::instance()->setFuncInfo(Q_FUNC_INFO).setErrorCode(errorCode)

#define ERROR_RETURN(errorCode) \
    if ((errorCode) < 0) { \
        SET_ERROR_CODE(errorCode); \
        return false; \
    } \
    return true;

namespace Ffmpeg {

class AVError;

class FFMPEG_EXPORT AVErrorManager : public QObject
{
    Q_OBJECT
public:
    void setPrint(bool print);
    void setMaxCaches(int max);

    auto setFuncInfo(const QString &funcInfo) -> AVErrorManager &;
    auto setErrorCode(int errorCode) -> AVErrorManager &;
    [[nodiscard]] auto lastErrorString() const -> QString;
    [[nodiscard]] auto errorCodes() const -> QList<int>;

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    explicit AVErrorManager(QObject *parent = nullptr);
    ~AVErrorManager() override;

    class AVErrorManagerPrivate;
    QScopedPointer<AVErrorManagerPrivate> d_ptr;

    SINGLETON(AVErrorManager)
};

} // namespace Ffmpeg

#endif // AVERRORMANAGER_HPP
