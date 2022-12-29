#ifndef OPENWEBMEDIADIALOG_HPP
#define OPENWEBMEDIADIALOG_HPP

#include <QDialog>

class OpenWebMediaDialog : public QDialog
{
    Q_OBJECT
public:
    OpenWebMediaDialog(QWidget *parent = nullptr);
    ~OpenWebMediaDialog();

    QString url() const;

private slots:
    void onOk();

private:
    void setupUI();

    class OpenWebMediaDialogPrivate;
    QScopedPointer<OpenWebMediaDialogPrivate> d_ptr;
};

#endif // OPENWEBMEDIADIALOG_HPP
