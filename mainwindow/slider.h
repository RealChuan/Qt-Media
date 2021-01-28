#ifndef SLIDER_H
#define SLIDER_H

#include <QSlider>

class Slider : public QSlider
{
    Q_OBJECT
public:
    Slider(QWidget *parent = 0);
    ~Slider();

signals:
    void onEnter();
    void onLeave();
    void onHover(int pos, int value);

protected:
    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent *e);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent *event);
    //#if CODE_FOR_CLICK == 1
    inline int pick(const QPoint &pt) const;
    int pixelPosToRangeValue(int pos) const;
    void initStyleOption_Qt430(QStyleOptionSlider *option) const;
    //#endif
};

#endif // SLIDER_H
