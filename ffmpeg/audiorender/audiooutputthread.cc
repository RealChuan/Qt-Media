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

    Audio::Config config;
};

AudioOutputThread::AudioOutputThread(QObject *parent)
    : QThread{parent}
    , d_ptr(new AudioOutputThreadPrivate(this))
{}

AudioOutputThread::~AudioOutputThread()
{
    closeOutput();
}

void AudioOutputThread::openOutput(const Audio::Config &config)
{
    closeOutput();
    d_ptr->config = config;
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
    QScopedPointer<AudioOutput> audioOutputPtr(new AudioOutput(d_ptr->config));
    connect(this, &AudioOutputThread::wirteData, audioOutputPtr.data(), &AudioOutput::onWrite);
    connect(this,
            &AudioOutputThread::volumeChanged,
            audioOutputPtr.data(),
            &AudioOutput::onSetVolume);
    exec();
}

} // namespace Ffmpeg
