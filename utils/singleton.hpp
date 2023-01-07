#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <QObject>

namespace Utils {

template<typename T>
class Singleton
{
    Q_DISABLE_COPY_MOVE(Singleton)
public:
    static T *getInstance();

private:
    Singleton() = default;
    ~Singleton() = default;
};

template<typename T>
T *Singleton<T>::getInstance()
{
    static T t;
    return &t;
}

} // namespace Utils

#define SINGLETON(Class) \
private: \
    Q_DISABLE_COPY_MOVE(Class); \
    friend class Utils::Singleton<Class>; \
\
public: \
    static Class *instance() \
    { \
        return Utils::Singleton<Class>::getInstance(); \
    }

#endif // SINGLETON_HPP
