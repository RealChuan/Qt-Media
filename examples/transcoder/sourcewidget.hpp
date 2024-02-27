#ifndef SOURCEWIDGET_HPP
#define SOURCEWIDGET_HPP

#include <QWidget>

class SourceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SourceWidget(QWidget *parent = nullptr);
    ~SourceWidget() override;

    void setSource(const QString &source);
    [[nodiscard]] auto source() const -> QString;

    void setDuration(qint64 duration); // microseconds

    void setFileInfo(const QString &info);

    [[nodiscard]] auto range() const -> QPair<qint64, qint64>;

private slots:
    void onRangeChanged();

signals:
    void showMediaInfo();

private:
    void setupUI();
    void buildConnect();

    class SourceWidgetPrivate;
    QScopedPointer<SourceWidgetPrivate> d_ptr;
};

#endif // SOURCEWIDGET_HPP
