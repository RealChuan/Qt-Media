#ifndef TONEMAPPING_H
#define TONEMAPPING_H

#include <ffmpeg/ffmepg_global.h>

#include <QObject>

namespace Ffmpeg {

class FFMPEG_EXPORT ToneMapping : public QObject
{
    Q_OBJECT
public:
    enum Type { NONE = 0, AUTO, CLIP, LINEAR, GAMMA, REINHARD, HABLE, MOBIUS, ACES, FILMIC };
    Q_ENUM(Type);

    using QObject::QObject;

    static void toneMapping(QByteArray &header, QByteArray &frag, Type type = NONE);
};

} // namespace Ffmpeg

#endif // TONEMAPPING_H
