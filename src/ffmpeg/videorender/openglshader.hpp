#ifndef OPENGLSHADER_HPP
#define OPENGLSHADER_HPP

#include "tonemapping.hpp"

#include <ffmpeg/colorutils.hpp>

#include <QGenericMatrix>

namespace Ffmpeg {

class Frame;
class OpenglShader : public QObject
{
public:
    explicit OpenglShader(QObject *parent = nullptr);
    ~OpenglShader() override;

    auto generate(Frame *frame,
                  ToneMapping::Type type = ToneMapping::Type::NONE,
                  ColorUtils::Primaries::Type destPrimaries = ColorUtils::Primaries::Type::AUTO)
        -> QByteArray;

    [[nodiscard]] auto isConvertPrimaries() const -> bool;
    [[nodiscard]] auto convertPrimariesMatrix() const -> QMatrix3x3;

private:
    class OpenglShaderPrivate;
    QScopedPointer<OpenglShaderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // OPENGLSHADER_HPP
