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
    virtual void enterEvent(QEnterEvent *event) override;
    virtual void leaveEvent(QEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    //#if CODE_FOR_CLICK == 1
    inline int pick(const QPoint &pt) const;
    int pixelPosToRangeValue(int pos) const;
    void initStyleOption_Qt430(QStyleOptionSlider *option) const;
    //#endif
};

#endif // SLIDER_H
