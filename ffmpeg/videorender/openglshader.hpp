#ifndef OPENGLSHADER_HPP
#define OPENGLSHADER_HPP

#include "tonemap.hpp"

namespace Ffmpeg {

class Frame;
class OpenglShader : public QObject
{
public:
    explicit OpenglShader(QObject *parent = nullptr);
    ~OpenglShader() override;

    auto generate(Frame *frame, Tonemap::Type type = Tonemap::Type::NONE) -> QByteArray;

private:
    class OpenglShaderPrivate;
    QScopedPointer<OpenglShaderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // OPENGLSHADER_HPP
