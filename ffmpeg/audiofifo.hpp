#ifndef AUDIOFIFO_HPP
#define AUDIOFIFO_HPP

#include <QObject>

namespace Ffmpeg {

class CodecContext;
class AudioFifo : public QObject
{
public:
    explicit AudioFifo(CodecContext *ctx, QObject *parent = nullptr);
    ~AudioFifo() override;

    auto realloc(int nb_samples) -> bool;

    auto write(void **data, int nb_samples) -> bool;
    auto read(void **data, int nb_samples) -> bool;

    auto size() const -> int;

private:
    class AudioFifoPrivtate;
    QScopedPointer<AudioFifoPrivtate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIOFIFO_HPP
