#include "controlwidget.hpp"
#include "qmediaplaylist.h"
#include "slider.h"

#include <utils/utils.h>

#include <QtWidgets>

class ControlWidget::ControlWidgetPrivate
{
public:
    explicit ControlWidgetPrivate(ControlWidget *q)
        : q_ptr(q)
    {
        slider = new Slider(q_ptr);
        slider->setRange(0, 100);
        positionLabel = new QLabel("00:00:00", q_ptr);
        durationLabel = new QLabel("00:00:00", q_ptr);
        sourceFPSLabel = new QLabel("FPS: 00.00", q_ptr);
        currentFPSLabel = new QLabel("00.00", q_ptr);
        playButton = new QToolButton(q_ptr);
        playButton->setCheckable(true);
        setPlayButtonIcon();

        readSpeedLabel = new QLabel(q_ptr);

        volumeSlider = new Slider(q_ptr);
        volumeSlider->setMinimumWidth(60);
        volumeSlider->setRange(0, 100);

        speedCbx = new QComboBox(q_ptr);
        auto *speedCbxView = new QListView(speedCbx);
        speedCbxView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        speedCbxView->setTextElideMode(Qt::ElideRight);
        speedCbxView->setAlternatingRowColors(true);
        speedCbx->setView(speedCbxView);
        double i = 0.5;
        double step = 0.5;
        while (i <= 3) {
            speedCbx->addItem(QString::number(i), i);
            i += step;
        }
        speedCbx->setCurrentText("1");

        modelButton = new QPushButton(q_ptr);
    }

    void setupUI()
    {
        auto *processLayout = new QHBoxLayout;
        processLayout->setSpacing(10);
        processLayout->addWidget(slider);
        processLayout->addWidget(positionLabel);
        processLayout->addWidget(new QLabel("/", q_ptr));
        processLayout->addWidget(durationLabel);

        auto *skipBackwardButton = new QToolButton(q_ptr);
        skipBackwardButton->setToolTip(QObject::tr("Previous"));
        skipBackwardButton->setIcon(q_ptr->style()->standardIcon(QStyle::SP_MediaSkipBackward));
        QObject::connect(skipBackwardButton, &QToolButton::clicked, q_ptr, &ControlWidget::previous);

        auto *skipForwardButton = new QToolButton(q_ptr);
        skipForwardButton->setToolTip(QObject::tr("Next"));
        skipForwardButton->setIcon(q_ptr->style()->standardIcon(QStyle::SP_MediaSkipForward));
        QObject::connect(skipForwardButton, &QToolButton::clicked, q_ptr, &ControlWidget::next);

        auto *volumeBotton = new QToolButton(q_ptr);
        volumeBotton->setIcon(q_ptr->style()->standardIcon(QStyle::SP_MediaVolume));

        auto *listButton = new QToolButton(q_ptr);
        listButton->setText(tr("List"));
        QObject::connect(listButton, &QToolButton::clicked, q_ptr, &ControlWidget::showList);

        auto *controlLayout = new QHBoxLayout;
        controlLayout->setSpacing(10);
        controlLayout->addWidget(skipBackwardButton);
        controlLayout->addWidget(playButton);
        controlLayout->addWidget(skipForwardButton);
        controlLayout->addStretch();
        controlLayout->addWidget(readSpeedLabel);
        controlLayout->addWidget(volumeBotton);
        controlLayout->addWidget(volumeSlider);
        controlLayout->addWidget(new QLabel(tr("Speed: "), q_ptr));
        controlLayout->addWidget(speedCbx);
        controlLayout->addWidget(sourceFPSLabel);
        controlLayout->addWidget(new QLabel("->", q_ptr));
        controlLayout->addWidget(currentFPSLabel);
        controlLayout->addWidget(modelButton);
        controlLayout->addWidget(listButton);

        auto *widget = new QWidget(q_ptr);
        widget->setObjectName("wid");
        widget->setStyleSheet("QWidget#wid{background: rgba(255,255,255,0.3); border-radius:5px;}"
                              "QLabel{ color: white; }");
        auto *layout = new QVBoxLayout(widget);
        layout->setSpacing(15);
        layout->addLayout(processLayout);
        layout->addLayout(controlLayout);

        auto *l = new QHBoxLayout(q_ptr);
        l->addWidget(widget);
    }

    void setPlayButtonIcon()
    {
        playButton->setIcon(playButton->style()->standardIcon(
            playButton->isChecked() ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
    }

    void initModelButton()
    {
        modelButton->setProperty("model", QMediaPlaylist::Sequential);
        QMetaObject::invokeMethod(q_ptr, &ControlWidget::onModelChanged, Qt::QueuedConnection);
    }

    ControlWidget *q_ptr;

    Slider *slider;
    QLabel *positionLabel;
    QLabel *durationLabel;
    QLabel *sourceFPSLabel;
    QLabel *currentFPSLabel;
    QToolButton *playButton;
    QLabel *readSpeedLabel;
    Slider *volumeSlider;
    QComboBox *speedCbx;
    QPushButton *modelButton;
};

ControlWidget::ControlWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new ControlWidgetPrivate(this))
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground); //设置窗口背景透明

    d_ptr->setupUI();
    buildConnect();
    d_ptr->initModelButton();
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

void ControlWidget::setDuration(int value)
{
    auto str = QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss");
    d_ptr->durationLabel->setText(str);
    d_ptr->slider->blockSignals(true);
    d_ptr->slider->setRange(0, value);
    d_ptr->slider->blockSignals(false);
    setPosition(0);
}

void ControlWidget::setPosition(int value)
{
    auto str = QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss");
    d_ptr->positionLabel->setText(str);

    d_ptr->slider->blockSignals(true);
    d_ptr->slider->setValue(value);
    d_ptr->slider->blockSignals(false);
}

void ControlWidget::setCacheSpeed(qint64 speed)
{
    d_ptr->readSpeedLabel->setText(Utils::convertBytesToString(speed) + "/S");
}

void ControlWidget::onSpeedChanged()
{
    auto data = d_ptr->speedCbx->currentData().toDouble();
    emit speedChanged(data);
}

void ControlWidget::onModelChanged()
{
    auto model = d_ptr->modelButton->property("model").toInt();

    auto metaEnum = QMetaEnum::fromType<QMediaPlaylist::PlaybackMode>();
    model += 1;
    if (model > QMediaPlaylist::Random) {
        model = QMediaPlaylist::CurrentItemOnce;
    }
    const auto *text = metaEnum.valueToKey(model);
    d_ptr->modelButton->setText(text);
    d_ptr->modelButton->setToolTip(text);
    d_ptr->modelButton->setProperty("model", model);

    emit modelChanged(model);
}

void ControlWidget::buildConnect()
{
    connect(d_ptr->slider, &Slider::valueChanged, this, &ControlWidget::seek);
    connect(d_ptr->slider, &Slider::onHover, this, &ControlWidget::hoverPosition);
    connect(d_ptr->slider, &Slider::onLeave, this, &ControlWidget::leavePosition);
    connect(d_ptr->playButton, &QToolButton::clicked, this, &ControlWidget::play);
    connect(d_ptr->volumeSlider, &QSlider::valueChanged, this, &ControlWidget::volumeChanged);
    connect(d_ptr->speedCbx, &QComboBox::currentIndexChanged, this, &ControlWidget::onSpeedChanged);
    connect(d_ptr->modelButton, &QPushButton::clicked, this, &ControlWidget::onModelChanged);
}
