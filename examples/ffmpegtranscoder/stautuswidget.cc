#include "stautuswidget.hpp"

#include <QtWidgets>

class StautusWidget::StautusWidgetPrivate
{
public:
    explicit StautusWidgetPrivate(StautusWidget *q)
        : q_ptr(q)
    {
        startButton = new QToolButton(q_ptr);
        startButton->setText(QCoreApplication::translate("StautusWidgetPrivate", "Start"));
        progressBar = new QProgressBar(q_ptr);
        progressBar->setRange(0, 100);
        fpsLabel = new QLabel(q_ptr);
        fpsLabel->setToolTip(
            QCoreApplication::translate("StautusWidgetPrivate", "Video Encoder FPS."));
    }

    StautusWidget *q_ptr = nullptr;

    QToolButton *startButton;
    QProgressBar *progressBar;
    QLabel *fpsLabel;
};

StautusWidget::StautusWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new StautusWidgetPrivate(this))
{
    setupUI();
    buildConnect();
}

StautusWidget::~StautusWidget() {}

void StautusWidget::setStatus(const QString &status)
{
    d_ptr->startButton->setText(status);
}

QString StautusWidget::status() const
{
    return d_ptr->startButton->text();
}

void StautusWidget::setProgress(int progress)
{
    d_ptr->progressBar->setValue(progress);
}

void StautusWidget::setFPS(double fps)
{
    auto text = QString("FPS: %1").arg(QString::number(fps, 'f', 2));
    d_ptr->fpsLabel->setText(text);
    d_ptr->fpsLabel->setToolTip(text);
}

void StautusWidget::click()
{
    d_ptr->startButton->click();
}

void StautusWidget::setupUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->addWidget(d_ptr->startButton);
    layout->addWidget(d_ptr->progressBar);
    layout->addWidget(d_ptr->fpsLabel);
}

void StautusWidget::buildConnect()
{
    connect(d_ptr->startButton, &QToolButton::clicked, this, &StautusWidget::start);
}
