#ifndef SLIDER_H
#define SLIDER_H

#include <QSlider>

class Slider : public QSlider
{
    Q_OBJECT
public:
    explicit Slider(QWidget *parent = nullptr);
    ~Slider() override;

    void setNodes(const QList<qint64> &nodes);
    void clearNodes();

signals:
    void onEnter();
    void onLeave();
    void onHover(int pos, int value);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    [[nodiscard]] inline auto pick(const QPoint &point) const -> int;
    [[nodiscard]] auto pixelPosToRangeValue(int pos) const -> int;

private:
    class SliderPrivate;
    QScopedPointer<SliderPrivate> d_ptr;
};

#endif // SLIDER_H
