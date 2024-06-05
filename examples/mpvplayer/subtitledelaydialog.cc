#include "subtitledelaydialog.hpp"

#include <QtWidgets>

class SubtitleDelayDialog::SubtitleDelayDialogPrivate
{
public:
    explicit SubtitleDelayDialogPrivate(SubtitleDelayDialog *q)
        : q_ptr(q)
    {
        delaySpinBox = new QDoubleSpinBox(q_ptr);
        delaySpinBox->setRange(INT_MIN, INT_MAX);
        delaySpinBox->setSingleStep(0.5);
    }

    SubtitleDelayDialog *q_ptr;

    QDoubleSpinBox *delaySpinBox;
};

SubtitleDelayDialog::SubtitleDelayDialog(QWidget *parent)
    : QDialog(parent)
    , d_ptr(new SubtitleDelayDialogPrivate(this))
{
    setupUI();
    setMinimumSize(350, 100);
    resize(350, 100);
}

SubtitleDelayDialog::~SubtitleDelayDialog() {}

void SubtitleDelayDialog::setDelay(double delay)
{
    d_ptr->delaySpinBox->setValue(delay);
}

auto SubtitleDelayDialog::delay() const -> double
{
    return d_ptr->delaySpinBox->value();
}

void SubtitleDelayDialog::onDelayChanged(double delay)
{
    emit delayChanged(d_ptr->delaySpinBox->value());
}

void SubtitleDelayDialog::setupUI()
{
    auto *button = new QToolButton(this);
    button->setText(tr("Apply"));
    connect(button, &QToolButton::clicked, this, &SubtitleDelayDialog::onDelayChanged);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel(tr("Subtitle delay(seconds):"), this));
    layout->addWidget(d_ptr->delaySpinBox);
    layout->addWidget(button);
}
