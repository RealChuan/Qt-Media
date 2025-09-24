#include "audiooutputthread.hpp"
#include "audiooutput.hpp"

namespace Ffmpeg {

class AudioOutputThread::AudioOutputThreadPrivate
{
public:
    AudioOutputThreadPrivate(AudioOutputThread *q)
        : q_ptr(q)
    {}

    AudioOutputThread *q_ptr;

    AVContextInfo *contextInfo;
    qreal volume = 0.5;
};

AudioOutputThread::AudioOutputThread(QObject *parent)
    : QThread{parent}
    , d_ptr(new AudioOutputThreadPrivate(this))
{
    qRegisterMetaType<FramePtr>("FramePtr");
}

AudioOutputThread::~AudioOutputThread()
{
    closeOutput();
}

void AudioOutputThread::openOutput(AVContextInfo *contextInfo, qreal volume)
{
    closeOutput();
    d_ptr->contextInfo = contextInfo;
    d_ptr->volume = volume;
    start();
}

void AudioOutputThread::closeOutput()
{
    if (isRunning()) {
        quit();
        wait();
    }
}

void AudioOutputThread::run()
{
    QScopedPointer<AudioOutput> audioOutputPtr(new AudioOutput(d_ptr->contextInfo, d_ptr->volume));
    connect(this,
            &AudioOutputThread::convertData,
            audioOutputPtr.data(),
            &AudioOutput::onConvertData);
    connect(this, &AudioOutputThread::wirteData, audioOutputPtr.data(), &AudioOutput::onWrite);
    connect(this,
            &AudioOutputThread::volumeChanged,
            audioOutputPtr.data(),
            &AudioOutput::onSetVolume);
    exec();
}

} // namespace Ffmpeg
