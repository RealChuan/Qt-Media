#ifndef TONEMAP_HPP
#define TONEMAP_HPP

#include <ffmpeg/ffmepg_global.h>

#include <QObject>

namespace Ffmpeg {

class FFMPEG_EXPORT Tonemap : public QObject
{
    Q_OBJECT
public:
    enum Type { NONE = 0, AUTO, CLIP, LINEAR, GAMMA, REINHARD, HABLE, MOBIUS, ACES, FILMIC };
    Q_ENUM(Type);

    using QObject::QObject;

    static void toneMap(QByteArray &header, QByteArray &frag, Type type = NONE);
};

} // namespace Ffmpeg

#endif // TONEMAP_HPP
