#ifndef SUBTITLETHREAD_HPP
#define SUBTITLETHREAD_HPP

#include <QThread>

class SubtitleThread : public QThread
{
    Q_OBJECT
public:
    explicit SubtitleThread(QObject *parent = nullptr);
    ~SubtitleThread();

    void setFilePath(const QString &filepath);

    void startRead();
    void stopRead();

protected:
    void run() override;

private:
    class SubtitleThreadPrivate;
    QScopedPointer<SubtitleThreadPrivate> d_ptr;
};

#endif // SUBTITLETHREAD_HPP
