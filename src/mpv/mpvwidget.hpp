#ifndef MPVWIDGET_HPP
#define MPVWIDGET_HPP

#include "mpv_global.h"

#include <QWidget>

namespace Mpv {

class MPV_LIB_EXPORT MpvWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget() override;
};

} // namespace Mpv

#endif // MPVWIDGET_HPP
