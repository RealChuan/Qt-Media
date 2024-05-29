#ifndef AUDIOOUTPUTTHREAD_HPP
#define AUDIOOUTPUTTHREAD_HPP

#include <QThread>

namespace Ffmpeg {

class Frame;
class AVContextInfo;

class AudioOutputThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioOutputThread(QObject *parent = nullptr);
    ~AudioOutputThread() override;

    void openOutput(AVContextInfo *contextInfo, qreal volume);
    void closeOutput();

signals:
    void convertData(const QSharedPointer<Ffmpeg::Frame> &frameptr);
    void wirteData();
    void volumeChanged(qreal value);

protected:
    void run() override;

private:
    class AudioOutputThreadPrivate;
    QScopedPointer<AudioOutputThreadPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIOOUTPUTTHREAD_HPP
