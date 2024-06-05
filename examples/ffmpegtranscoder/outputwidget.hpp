#ifndef OUTPUTWIDGET_HPP
#define OUTPUTWIDGET_HPP

#include <QWidget>

class OutPutWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OutPutWidget(QWidget *parent = nullptr);
    ~OutPutWidget() override;

    void setOutputFileName(const QString &fileName);
    [[nodiscard]] auto outputFilePath() const -> QString;

private slots:
    void onBrowse();
    void onOpenFolder();

private:
    void setupUI();
    void buildConnect();

    class OutPutWidgetPrivate;
    QScopedPointer<OutPutWidgetPrivate> d_ptr;
};

#endif // OUTPUTWIDGET_HPP
