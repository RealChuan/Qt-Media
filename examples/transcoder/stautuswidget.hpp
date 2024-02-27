#ifndef STAUTUSWIDGET_HPP
#define STAUTUSWIDGET_HPP

#include <QWidget>

class StautusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StautusWidget(QWidget *parent = nullptr);
    ~StautusWidget() override;

    void setStatus(const QString &status);
    [[nodiscard]] auto status() const -> QString;

    void setProgress(int progress);

    void setFPS(double fps);

    void click();

signals:
    void start();

private:
    void setupUI();
    void buildConnect();

    class StautusWidgetPrivate;
    QScopedPointer<StautusWidgetPrivate> d_ptr;
};

#endif // STAUTUSWIDGET_HPP
