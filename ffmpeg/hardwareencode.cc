#include "hardwareencode.hpp"

namespace Ffmpeg {

class HardWareEncode::HardWareEncodePrivate
{
public:
    HardWareEncodePrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;
};

HardWareEncode::HardWareEncode(QObject *parent)
    : QObject{parent}
    , d_ptr(new HardWareEncodePrivate(this))
{}

HardWareEncode::~HardWareEncode() {}

} // namespace Ffmpeg
