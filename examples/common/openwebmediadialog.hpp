#ifndef OPENWEBMEDIADIALOG_HPP
#define OPENWEBMEDIADIALOG_HPP

#include <QDialog>

class OpenWebMediaDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OpenWebMediaDialog(QWidget *parent = nullptr);
    ~OpenWebMediaDialog() override;

    [[nodiscard]] auto url() const -> QString;

private slots:
    void onOk();

private:
    void setupUI();

    class OpenWebMediaDialogPrivate;
    QScopedPointer<OpenWebMediaDialogPrivate> d_ptr;
};

#endif // OPENWEBMEDIADIALOG_HPP
