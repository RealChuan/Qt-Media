#ifndef AUDIOOUTPUTTHREAD_HPP
#define AUDIOOUTPUTTHREAD_HPP

#include "audioconfig.hpp"

#include <QThread>

namespace Ffmpeg {

class AudioOutputThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioOutputThread(QObject *parent = nullptr);
    ~AudioOutputThread() override;

    void openOutput(const Audio::Config &config);
    void closeOutput();

signals:
    void wirteData(const QByteArray &data);
    void volumeChanged(qreal value);

protected:
    void run() override;

private:
    class AudioOutputThreadPrivate;
    QScopedPointer<AudioOutputThreadPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIOOUTPUTTHREAD_HPP
