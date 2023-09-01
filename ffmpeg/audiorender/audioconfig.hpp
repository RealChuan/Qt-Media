#ifndef AUDIOCONFIG_HPP
#define AUDIOCONFIG_HPP

#include <QAudioFormat>

namespace Ffmpeg {

namespace Audio {

struct Config
{
    QAudioFormat format;
    qsizetype bufferSize = 0;
    qreal volume = 0.5;
};

} // namespace Audio

} // namespace Ffmpeg

#endif // AUDIOCONFIG_HPP
