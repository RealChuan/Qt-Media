#ifndef AUDIOFIFO_HPP
#define AUDIOFIFO_HPP

#include <QObject>

namespace Ffmpeg {

class CodecContext;
class AudioFifo : public QObject
{
public:
    explicit AudioFifo(CodecContext *ctx, QObject *parent = nullptr);
    ~AudioFifo();

    bool realloc(int nb_samples);

    bool write(void **data, int nb_samples);
    bool read(void **data, int nb_samples);

    int size() const;

private:
    class AudioFifoPrivtate;
    QScopedPointer<AudioFifoPrivtate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIOFIFO_HPP
