#include "titlewidget.hpp"

#include <QtWidgets>

class TitleWidget::TitleWidgetPrivate
{
public:
    TitleWidgetPrivate(TitleWidget *parent)
        : owner(parent)
    {
        label = new QLabel(owner);
        label->setStyleSheet("QLabel{background: rgba(255,255,255,0.3); padding-left: 20px; "
                             "padding-right: 20px; color: white; border-radius:5px;}");
        timer = new QTimer(owner);
        timer->setSingleShot(true);
    }

    TitleWidget *owner;

    QLabel *label;
    QTimer *timer;
};

TitleWidget::TitleWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new TitleWidgetPrivate(this))
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    setupUI();
    buildConnect();
}

TitleWidget::~TitleWidget() {}

void TitleWidget::setText(const QString &text)
{
    d_ptr->label->setText(text);
}

void TitleWidget::setAutoHide(int msec)
{
    d_ptr->timer->start(msec);
}

void TitleWidget::onHide()
{
    hide();
}

void TitleWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->addWidget(d_ptr->label);
}

void TitleWidget::buildConnect()
{
    connect(d_ptr->timer, &QTimer::timeout, this, &TitleWidget::onHide);
}
