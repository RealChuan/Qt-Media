#ifndef AUDIOOUTPUT_HPP
#define AUDIOOUTPUT_HPP

#include "audioconfig.hpp"

#include <QAudio>
#include <QObject>

namespace Ffmpeg {

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(const Audio::Config &config, QObject *parent = nullptr);
    ~AudioOutput();

public slots:
    void onWrite(const QByteArray &audioBuf);
    void onSetVolume(qreal value);

private slots:
    void onStateChanged(QAudio::State state);
    void onAudioOutputsChanged();

private:
    void buildConnect();

    class AudioOutputPrivate;
    QScopedPointer<AudioOutputPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIOOUTPUT_HPP
