#ifndef SLIDER_H
#define SLIDER_H

#include <QSlider>

class Slider : public QSlider
{
    Q_OBJECT
public:
    explicit Slider(QWidget *parent = nullptr);
    ~Slider() override;

signals:
    void onEnter();
    void onLeave();
    void onHover(int pos, int value);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *e) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    //#if CODE_FOR_CLICK == 1
    [[nodiscard]] inline auto pick(const QPoint &pt) const -> int;
    [[nodiscard]] auto pixelPosToRangeValue(int pos) const -> int;
    void initStyleOption_Qt430(QStyleOptionSlider *option) const;
    //#endif
};

#endif // SLIDER_H
