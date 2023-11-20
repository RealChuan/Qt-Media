#ifndef TONEMAP_HPP
#define TONEMAP_HPP

#include <ffmpeg/ffmepg_global.h>

#include <QObject>

namespace Ffmpeg {

class FFMPEG_EXPORT Tonemap : public QObject
{
    Q_OBJECT
public:
    enum Type {
        NONE = 0,
        AUTO,
        REINHARD,
        REINHARD_JODIE,
        CONST_LUMINANCE_REINHARD,
        HABLE,
        ACES,
        ACES_FITTED,
        ACES_APPROX,
        FILMIC,
        UNCHARTED2_FILMIC
    };
    Q_ENUM(Type);

    explicit Tonemap(QObject *parent = nullptr);

    static void toneMap(QByteArray &header, QByteArray &frag, Type type = NONE);
};

} // namespace Ffmpeg

#endif // TONEMAP_HPP
