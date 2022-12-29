#ifndef TRANSCODE_HPP
#define TRANSCODE_HPP

#include <QObject>

namespace Ffmpeg {

class AVError;
class Frame;
class Transcode : public QObject
{
    Q_OBJECT
public:
    explicit Transcode(QObject *parent = nullptr);
    ~Transcode();

    bool openInputFile(const QString &filepath);
    bool openOutputFile(const QString &filepath);

    void reset();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void initFilters();
    void loop();
    bool filterEncodeWriteframe(Frame *frame, uint stream_index);
    bool encodeWriteFrame(uint stream_index, int flush);

    void setError(int errorCode);

    class TranscodePrivate;
    QScopedPointer<TranscodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // TRANSCODE_HPP
