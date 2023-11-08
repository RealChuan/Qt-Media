#ifndef OPENGLSHADER_HPP
#define OPENGLSHADER_HPP

#include <QObject>

namespace Ffmpeg {

class OpenglShader : public QObject
{
public:
    explicit OpenglShader(QObject *parent = nullptr);
    ~OpenglShader() override;

    QByteArray generate(int format);

private:
    class OpenglShaderPrivate;
    QScopedPointer<OpenglShaderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // OPENGLSHADER_HPP
