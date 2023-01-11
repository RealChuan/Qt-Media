#ifndef HARDWAREENCODE_HPP
#define HARDWAREENCODE_HPP

#include <QObject>

namespace Ffmpeg {

class HardWareEncode : public QObject
{
public:
    explicit HardWareEncode(QObject *parent = nullptr);
    ~HardWareEncode();

private:
    class HardWareEncodePrivate;
    QScopedPointer<HardWareEncodePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // HARDWAREENCODE_HPP
