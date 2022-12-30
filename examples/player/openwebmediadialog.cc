#include "openwebmediadialog.hpp"

#include <QtWidgets>

class OpenWebMediaDialog::OpenWebMediaDialogPrivate
{
public:
    OpenWebMediaDialogPrivate(QWidget *parent)
        : owner(parent)
    {
        textEdit = new QTextEdit(owner);
        textEdit->setPlaceholderText(QObject::tr("http://.....", "OpenWebMediaDialog"));
    }

    QWidget *owner;
    QTextEdit *textEdit;
};

OpenWebMediaDialog::OpenWebMediaDialog(QWidget *parent)
    : QDialog(parent)
    , d_ptr(new OpenWebMediaDialogPrivate(this))
{
    setupUI();
}

OpenWebMediaDialog::~OpenWebMediaDialog() {}

QString OpenWebMediaDialog::url() const
{
    return d_ptr->textEdit->toPlainText().trimmed();
}

void OpenWebMediaDialog::onOk()
{
    auto str = url();
    if (str.isEmpty()) {
        QMessageBox::warning(this, tr("Empty"), tr("Please fill in the correct URL"));
        return;
    }
    QUrl url(str);
    if (!url.isValid()) {
        QMessageBox::warning(this, tr("Invalid"), tr("Please fill in the correct URL"));
        return;
    }

    accept();
}

void OpenWebMediaDialog::setupUI()
{
    auto cancelBtn = new QToolButton(this);
    cancelBtn->setText(tr("Cancel"));
    cancelBtn->setMinimumSize(100, 35);
    connect(cancelBtn, &QToolButton::clicked, this, &OpenWebMediaDialog::reject);
    auto okBtn = new QToolButton(this);
    okBtn->setText(tr("OK"));
    okBtn->setMinimumSize(100, 35);
    connect(okBtn, &QToolButton::clicked, this, &OpenWebMediaDialog::onOk);

    auto btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(d_ptr->textEdit);
    layout->addLayout(btnLayout);
}
