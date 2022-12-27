#ifndef TRANSCODE_HPP
#define TRANSCODE_HPP

#include <QObject>

namespace Ffmpeg {

class AVError;
class Transcode : public QObject
{
    Q_OBJECT
public:
    explicit Transcode(QObject *parent = nullptr);
    ~Transcode();

    bool openInputFile(const QString &filepath);
    bool openOutputFile(const QString &filepath);

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    class TranscodePrivate;
    QScopedPointer<TranscodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // TRANSCODE_HPP
