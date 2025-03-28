#include "slider.h"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOption>
#include <QtWidgets>

class Slider::SliderPrivate
{
public:
    explicit SliderPrivate(Slider *q)
        : q_ptr(q)
    {}

    Slider *q_ptr;

    QList<qint64> nodes;
};

Slider::Slider(QWidget *parent)
    : QSlider(parent)
    , d_ptr(new SliderPrivate(this))
{
    setOrientation(Qt::Horizontal);
    setMouseTracking(true); //mouseMoveEvent without press.
}

Slider::~Slider() = default;

void Slider::setNodes(const QList<qint64> &nodes)
{
    d_ptr->nodes = nodes;
    QMetaObject::invokeMethod(this, [this] { update(); }, Qt::QueuedConnection);
}

void Slider::clearNodes()
{
    d_ptr->nodes.clear();
    QMetaObject::invokeMethod(this, [this] { update(); }, Qt::QueuedConnection);
}

// Function copied from qslider.cpp
inline auto Slider::pick(const QPoint &point) const -> int
{
    return orientation() == Qt::Horizontal ? point.x() : point.y();
}

// // Function copied from qslider.cpp and modified to make it compile
auto Slider::pixelPosToRangeValue(int pos) const -> int
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    int sliderMin = 0;
    int sliderMax = 0;
    int sliderLength = 0;
    if (orientation() == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }
    return QStyle::sliderValueFromPosition(minimum(),
                                           maximum(),
                                           pos - sliderMin,
                                           sliderMax - sliderMin,
                                           opt.upsideDown);
}

void Slider::enterEvent(QEnterEvent *event)
{
    setCursor(Qt::PointingHandCursor);
    emit onEnter();
    QSlider::enterEvent(event);
}

void Slider::leaveEvent(QEvent *event)
{
    QSlider::leaveEvent(event);

    auto pos = QCursor::pos();
    if (rect().contains(mapFromGlobal(pos))) {
        return;
    }
    emit onLeave();
    unsetCursor();
}

void Slider::mouseMoveEvent(QMouseEvent *event)
{
    const int o = style()->pixelMetric(QStyle::PM_SliderLength) - 1;
    int v = QStyle::sliderValueFromPosition(minimum(),
                                            maximum(),
                                            event->pos().x() - o / 2,
                                            width() - o,
                                            false);
    emit onHover(event->position().x(), v);
    QSlider::mouseMoveEvent(event);
}

// Based on code from qslider.cpp
void Slider::mousePressEvent(QMouseEvent *event)
{
    qDebug("pressed (%d, %d)", event->pos().x(), event->pos().y());
    if (event->button() == Qt::LeftButton) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider,
                                                         &opt,
                                                         QStyle::SC_SliderHandle,
                                                         this);
        const QPoint center = sliderRect.center() - sliderRect.topLeft();
        // to take half of the slider off for the setSliderPosition call we use the center - topLeft

        if (!sliderRect.contains(event->pos())) {
            qDebug("accept");
            event->accept();

            int v = pixelPosToRangeValue(pick(event->pos() - center));
            setSliderPosition(v);
            triggerAction(SliderMove);
            setRepeatAction(SliderNoAction);
            emit sliderMoved(v);  //TODO: ok?
            emit sliderPressed(); //TODO: ok?
        } else {
            QSlider::mousePressEvent(event);
        }
    } else {
        QSlider::mousePressEvent(event);
    }
}

void Slider::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);
    if (d_ptr->nodes.isEmpty()) {
        return;
    }

    const auto chapterRadius = 2.5;
    const auto chapterPadding = height() / 5.0;
    const auto chapterHeight = height() - chapterPadding * 2.0;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(QPalette::Highlight).lighter(150));
    for (const auto &node : std::as_const(d_ptr->nodes)) {
        auto xPos = QStyle::sliderPositionFromValue(minimum(), maximum(), node, width());
        QPointF p1(xPos - chapterRadius, chapterPadding);
        QPointF p2(xPos + chapterRadius, chapterPadding);
        QPointF p3(xPos + chapterRadius, chapterPadding + chapterHeight / 3 * 2);
        QPointF p4(xPos, chapterPadding + chapterHeight);
        QPointF p5(xPos - chapterRadius, chapterPadding + chapterHeight / 3 * 2);
        QPolygonF polygonF{p1, p2, p3, p4, p5, p1};
        painter.drawPolygon(polygonF);
    }
}
