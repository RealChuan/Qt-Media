#include "sourcewidget.hpp"

#include <QtWidgets>

class SourceWidget::SourceWidgetPrivate
{
public:
    explicit SourceWidgetPrivate(SourceWidget *q)
        : q_ptr(q)
    {
        inLineEdit = new QLineEdit(q_ptr);
        infoLabel = new QLabel(q_ptr);
        infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        startTimeEdit = new QTimeEdit(q_ptr);
        startTimeEdit->setDisplayFormat("hh:mm:ss");
        endTimeEdit = new QTimeEdit(q_ptr);
        endTimeEdit->setDisplayFormat("hh:mm:ss");
        outDurationLabel = new QLabel("-", q_ptr);
    }

    SourceWidget *q_ptr;

    QLineEdit *inLineEdit;
    QLabel *infoLabel;
    QTimeEdit *startTimeEdit;
    QTimeEdit *endTimeEdit;
    QLabel *outDurationLabel;
};

SourceWidget::SourceWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new SourceWidgetPrivate(this))
{
    setupUI();
    buildConnect();
}

SourceWidget::~SourceWidget() = default;

void SourceWidget::setSource(const QString &source)
{
    d_ptr->inLineEdit->setText(source);
}

auto SourceWidget::source() const -> QString
{
    return d_ptr->inLineEdit->text().trimmed();
}

void SourceWidget::setDuration(qint64 duration)
{
    auto durationTime = QTime::fromMSecsSinceStartOfDay(duration);
    // auto durationText = duration.toString("hh:mm:ss");
    d_ptr->startTimeEdit->setTime({});
    d_ptr->startTimeEdit->setMaximumTime(durationTime);
    d_ptr->endTimeEdit->setTime(durationTime);
    d_ptr->endTimeEdit->setMaximumTime(durationTime);
}

void SourceWidget::setFileInfo(const QString &info)
{
    d_ptr->infoLabel->setText(info);
    d_ptr->infoLabel->setToolTip(info);
}

auto SourceWidget::range() const -> QPair<qint64, qint64>
{
    return {d_ptr->startTimeEdit->time().msecsSinceStartOfDay() * 1000,
            d_ptr->endTimeEdit->time().msecsSinceStartOfDay() * 1000};
}

void SourceWidget::onRangeChanged()
{
    d_ptr->startTimeEdit->setMaximumTime(d_ptr->endTimeEdit->time());

    auto time = QTime::fromMSecsSinceStartOfDay(
        d_ptr->startTimeEdit->time().msecsTo(d_ptr->endTimeEdit->time()));
    auto text = tr("Out Duration: %1").arg(time.toString("hh:mm:ss"));
    d_ptr->outDurationLabel->setText(text);
    d_ptr->outDurationLabel->setToolTip(text);
}

void SourceWidget::setupUI()
{
    auto *fileInfoButton = new QPushButton(tr("File Info"), this);
    connect(fileInfoButton, &QPushButton::clicked, this, &SourceWidget::showMediaInfo);

    auto *layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel(tr("Input File:"), this));
    layout1->addWidget(d_ptr->inLineEdit);

    auto *layout2 = new QHBoxLayout;
    layout2->addWidget(fileInfoButton);
    layout2->addWidget(d_ptr->infoLabel);

    auto *layout3 = new QHBoxLayout;
    layout3->addWidget(new QLabel(tr("Range:"), this));
    layout3->addWidget(d_ptr->startTimeEdit);
    layout3->addWidget(new QLabel(tr("-"), this));
    layout3->addWidget(d_ptr->endTimeEdit);
    layout3->addWidget(d_ptr->outDurationLabel);
    layout3->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(layout1);
    layout->addLayout(layout2);
    layout->addLayout(layout3);
}

void SourceWidget::buildConnect()
{
    connect(d_ptr->startTimeEdit, &QTimeEdit::timeChanged, this, &SourceWidget::onRangeChanged);
    connect(d_ptr->endTimeEdit, &QTimeEdit::timeChanged, this, &SourceWidget::onRangeChanged);
}
