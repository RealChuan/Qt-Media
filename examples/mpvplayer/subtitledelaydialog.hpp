#ifndef SUBTITLEDELAYDIALOG_HPP
#define SUBTITLEDELAYDIALOG_HPP

#include <QDialog>

class SubtitleDelayDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SubtitleDelayDialog(QWidget *parent = nullptr);
    ~SubtitleDelayDialog() override;

    void setDelay(double delay);
    [[nodiscard]] auto delay() const -> double;

signals:
    void delayChanged(double delay);

private slots:
    void onDelayChanged(double delay);

private:
    void setupUI();

    class SubtitleDelayDialogPrivate;
    QScopedPointer<SubtitleDelayDialogPrivate> d_ptr;
};

#endif // SUBTITLEDELAYDIALOG_HPP
