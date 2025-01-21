#include "mpvwidget.hpp"

namespace Mpv {

MpvWidget::MpvWidget(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_NativeWindow);

    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("QWidget{ background:black; }");
}

MpvWidget::~MpvWidget() = default;

} // namespace Mpv
