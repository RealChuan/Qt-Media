#ifndef TITLEWIDGET_HPP
#define TITLEWIDGET_HPP

#include <QWidget>

class TitleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TitleWidget(QWidget *parent = nullptr);
    ~TitleWidget();

    void setText(const QString &text);
    void setAutoHide(int msec);

private slots:
    void onHide();

private:
    void setupUI();
    void buildConnect();

    class TitleWidgetPrivate;
    QScopedPointer<TitleWidgetPrivate> d_ptr;
};

#endif // TITLEWIDGET_HPP
