#include "videowidget.hpp"

VideoWidget::VideoWidget(QWidget *parent)
    : QVideoWidget{parent}
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::black);
    setPalette(p);
}
