#include "controlwidget.hpp"
#include "slider.h"

#include <utils/utils.h>

#include <QtWidgets>

class ControlWidget::ControlWidgetPrivate
{
public:
    ControlWidgetPrivate(ControlWidget *parent)
        : owner(parent)
    {
        slider = new Slider(owner);
        slider->setRange(0, 100);
        positionLabel = new QLabel("00:00:00", owner);
        durationLabel = new QLabel("00:00:00", owner);
        sourceFPSLabel = new QLabel("FPS: 00.00->", owner);
        currentFPSLabel = new QLabel("00.00", owner);
        playButton = new QToolButton(owner);
        playButton->setCheckable(true);
        setPlayButtonIcon();

        readSpeedLabel = new QLabel(owner);

        volumeSlider = new Slider(owner);
        volumeSlider->setRange(0, 100);

        speedCbx = new QComboBox(owner);
        auto speedCbxView = new QListView(speedCbx);
        speedCbxView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        speedCbxView->setTextElideMode(Qt::ElideRight);
        speedCbxView->setAlternatingRowColors(true);
        speedCbx->setView(speedCbxView);
        double i = 0.25;
        while (i <= 2) {
            speedCbx->addItem(QString::number(i), i);
            i += 0.25;
        }
        speedCbx->setCurrentText("1");
    }

    void setPlayButtonIcon()
    {
        playButton->setIcon(playButton->style()->standardIcon(
            playButton->isChecked() ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
    }

    ControlWidget *owner;

    Slider *slider;
    QLabel *positionLabel;
    QLabel *durationLabel;
    QLabel *sourceFPSLabel;
    QLabel *currentFPSLabel;
    QToolButton *playButton;
    QLabel *readSpeedLabel;
    Slider *volumeSlider;
    QComboBox *speedCbx;
};

ControlWidget::ControlWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new ControlWidgetPrivate(this))
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground); //设置窗口背景透明

    setupUI();
    buildConnect();
}

ControlWidget::~ControlWidget() {}

int ControlWidget::position() const
{
    return d_ptr->slider->value();
}

int ControlWidget::duration() const
{
    return d_ptr->slider->maximum();
}

QPoint ControlWidget::sliderGlobalPos() const
{
    return d_ptr->slider->mapToGlobal(d_ptr->slider->pos());
}

void ControlWidget::setSourceFPS(float fps)
{
    auto fpsStr = QString("FPS: %1").arg(QString::number(fps, 'f', 2));
    d_ptr->sourceFPSLabel->setText(fpsStr);
    d_ptr->sourceFPSLabel->setToolTip(fpsStr);
}

void ControlWidget::setCurrentFPS(float fps)
{
    auto fpsStr = QString::number(fps, 'f', 2);
    d_ptr->currentFPSLabel->setText(fpsStr);
    d_ptr->currentFPSLabel->setToolTip(fpsStr);
}

void ControlWidget::clickPlayButton()
{
    d_ptr->playButton->click();
    d_ptr->setPlayButtonIcon();
}

void ControlWidget::setPlayButtonChecked(bool checked)
{
    d_ptr->playButton->setChecked(checked);
    d_ptr->setPlayButtonIcon();
}

void ControlWidget::setVolume(int value)
{
    if (value < d_ptr->volumeSlider->minimum()) {
        value = d_ptr->volumeSlider->minimum();
    } else if (value > d_ptr->volumeSlider->maximum()) {
        value = d_ptr->volumeSlider->maximum();
    }
    d_ptr->volumeSlider->setValue(value);
}

int ControlWidget::volume() const
{
    return d_ptr->volumeSlider->value();
}

void ControlWidget::onDurationChanged(double value)
{
    auto str = QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss");
    d_ptr->durationLabel->setText(str);
    d_ptr->slider->blockSignals(true);
    d_ptr->slider->setRange(0, value);
    d_ptr->slider->blockSignals(false);
    onPositionChanged(0);
}

void ControlWidget::onPositionChanged(double value)
{
    auto str = QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss");
    d_ptr->positionLabel->setText(str);

    d_ptr->slider->blockSignals(true);
    d_ptr->slider->setValue(value);
    d_ptr->slider->blockSignals(false);
}

void ControlWidget::onReadSpeedChanged(qint64 speed)
{
    d_ptr->readSpeedLabel->setText(Utils::convertBytesToString(speed) + "/S");
}

void ControlWidget::onSpeedChanged()
{
    auto data = d_ptr->speedCbx->currentData().toDouble();
    emit speedChanged(data);
}

void ControlWidget::setupUI()
{
    QHBoxLayout *processLayout = new QHBoxLayout;
    processLayout->setSpacing(10);
    processLayout->addWidget(d_ptr->slider);
    processLayout->addWidget(d_ptr->positionLabel);
    processLayout->addWidget(new QLabel("/", this));
    processLayout->addWidget(d_ptr->durationLabel);

    auto volumeBotton = new QToolButton(this);
    volumeBotton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    auto listButton = new QToolButton(this);
    listButton->setText(tr("List"));
    connect(listButton, &QToolButton::clicked, this, &ControlWidget::showList);

    auto controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(10);
    controlLayout->addWidget(d_ptr->playButton);
    controlLayout->addStretch();
    controlLayout->addWidget(d_ptr->readSpeedLabel);
    controlLayout->addWidget(volumeBotton);
    controlLayout->addWidget(d_ptr->volumeSlider);
    controlLayout->addWidget(new QLabel(tr("Speed: "), this));
    controlLayout->addWidget(d_ptr->speedCbx);
    controlLayout->addWidget(d_ptr->sourceFPSLabel);
    controlLayout->addWidget(new QLabel("->", this));
    controlLayout->addWidget(d_ptr->currentFPSLabel);
    controlLayout->addWidget(listButton);

    auto widget = new QWidget(this);
    widget->setObjectName("wid");
    widget->setStyleSheet("QWidget#wid{background: rgba(255,255,255,0.3); border-radius:5px;}"
                          "QLabel{ color: white; }");
    auto layout = new QVBoxLayout(widget);
    layout->setSpacing(15);
    layout->addLayout(processLayout);
    layout->addLayout(controlLayout);

    auto l = new QHBoxLayout(this);
    l->addWidget(widget);
}

void ControlWidget::buildConnect()
{
    connect(d_ptr->slider, &Slider::valueChanged, this, &ControlWidget::seek);
    connect(d_ptr->slider, &Slider::onHover, this, &ControlWidget::hoverPosition);
    connect(d_ptr->slider, &Slider::onLeave, this, &ControlWidget::leavePosition);
    connect(d_ptr->playButton, &QToolButton::clicked, this, &ControlWidget::play);
    connect(d_ptr->volumeSlider, &QSlider::valueChanged, this, &ControlWidget::volumeChanged);
    connect(d_ptr->speedCbx, &QComboBox::currentIndexChanged, this, &ControlWidget::onSpeedChanged);
}
