#include "controlwidget.hpp"
#include "slider.h"

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
        durationLabel = new QLabel("/ 00:00:00", owner);
        sourceFPSLabel = new QLabel("FPS: 00.00->", owner);
        currentFPSLabel = new QLabel("00.00", owner);
        playButton = new QPushButton(QObject::tr("play"), owner);
        playButton->setCheckable(true);

        volumeSlider = new Slider(owner);
        volumeSlider->setRange(0, 100);

        useGpuCheckBox = new QCheckBox(QObject::tr("GPU Decode"), owner);
        useGpuCheckBox->setToolTip(tr("GPU Decode"));

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

        audioTracksCbx = new QComboBox(owner);
        audioTracksCbx->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        auto audioTracksView = new QListView(audioTracksCbx);
        audioTracksView->setTextElideMode(Qt::ElideRight);
        audioTracksView->setAlternatingRowColors(true);
        audioTracksCbx->setView(audioTracksView);

        subTracksCbx = new QComboBox(owner);
        subTracksCbx->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        auto subtitleStreamsView = new QListView(subTracksCbx);
        subtitleStreamsView->setTextElideMode(Qt::ElideRight);
        subtitleStreamsView->setAlternatingRowColors(true);
        subTracksCbx->setView(subtitleStreamsView);
    }

    ControlWidget *owner;

    Slider *slider;
    QLabel *positionLabel;
    QLabel *durationLabel;
    QLabel *sourceFPSLabel;
    QLabel *currentFPSLabel;
    QPushButton *playButton;
    Slider *volumeSlider;
    QCheckBox *useGpuCheckBox;
    QComboBox *speedCbx;
    QComboBox *audioTracksCbx;
    QComboBox *subTracksCbx;
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
    auto fpsStr = QString("FPS: %1->").arg(QString::number(fps, 'f', 2));
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
}

void ControlWidget::setPlayButtonChecked(bool checked)
{
    d_ptr->playButton->setChecked(checked);
}

void ControlWidget::setUseGpu(bool useGpu)
{
    d_ptr->useGpuCheckBox->setChecked(useGpu);
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

void ControlWidget::setAudioTracks(const QStringList &list)
{
    d_ptr->audioTracksCbx->blockSignals(true);
    d_ptr->audioTracksCbx->clear();
    d_ptr->audioTracksCbx->addItems(list);
    d_ptr->audioTracksCbx->blockSignals(false);
}

void ControlWidget::setCurrentAudioTrack(const QString &track)
{
    d_ptr->audioTracksCbx->blockSignals(true);
    d_ptr->audioTracksCbx->setCurrentText(track);
    d_ptr->audioTracksCbx->blockSignals(false);
}

void ControlWidget::setSubTracks(const QStringList &list)
{
    d_ptr->subTracksCbx->blockSignals(true);
    d_ptr->subTracksCbx->clear();
    d_ptr->subTracksCbx->addItems(list);
    d_ptr->subTracksCbx->blockSignals(false);
}

void ControlWidget::setCurrentSubTrack(const QString &track)
{
    d_ptr->subTracksCbx->blockSignals(true);
    d_ptr->subTracksCbx->setCurrentText(track);
    d_ptr->subTracksCbx->blockSignals(false);
}

void ControlWidget::onDurationChanged(double value)
{
    auto str = QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss");
    d_ptr->durationLabel->setText("/ " + str);
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

void ControlWidget::onSpeedChanged()
{
    auto data = d_ptr->speedCbx->currentData().toDouble();
    emit speedChanged(data);
}

void ControlWidget::setupUI()
{
    auto processWidget = new QWidget(this);
    //processWidget->setMaximumHeight(70);
    QHBoxLayout *processLayout = new QHBoxLayout(processWidget);
    processLayout->addWidget(d_ptr->slider);
    processLayout->addWidget(d_ptr->positionLabel);
    processLayout->addWidget(d_ptr->durationLabel);
    processLayout->addWidget(d_ptr->sourceFPSLabel);
    processLayout->addWidget(d_ptr->currentFPSLabel);

    auto listButton = new QToolButton(this);
    listButton->setText(tr("List"));
    connect(listButton, &QToolButton::clicked, this, &ControlWidget::showList);

    auto controlLayout = new QHBoxLayout;
    controlLayout->addWidget(d_ptr->playButton);
    controlLayout->addWidget(d_ptr->useGpuCheckBox);
    controlLayout->addWidget(new QLabel(tr("Volume: "), this));
    controlLayout->addWidget(d_ptr->volumeSlider);
    controlLayout->addWidget(new QLabel(tr("Speed: "), this));
    controlLayout->addWidget(d_ptr->speedCbx);
    controlLayout->addWidget(new QLabel(tr("Audio Tracks: "), this));
    controlLayout->addWidget(d_ptr->audioTracksCbx);
    controlLayout->addWidget(new QLabel(tr("Subtitle Tracks: "), this));
    controlLayout->addWidget(d_ptr->subTracksCbx);
    controlLayout->addWidget(listButton);

    auto widget = new QWidget(this);
    widget->setObjectName("wid");
    widget->setStyleSheet("QWidget#wid{background: rgba(255,255,255,0.3);}");
    auto layout = new QVBoxLayout(widget);
    layout->addWidget(processWidget);
    layout->addLayout(controlLayout);

    auto l = new QHBoxLayout(this);
    l->addWidget(widget);
}

void ControlWidget::buildConnect()
{
    connect(d_ptr->slider, &Slider::valueChanged, this, &ControlWidget::seek);
    connect(d_ptr->slider, &Slider::onHover, this, &ControlWidget::hoverPosition);
    connect(d_ptr->slider, &Slider::onLeave, this, &ControlWidget::leavePosition);
    connect(d_ptr->playButton, &QPushButton::clicked, this, &ControlWidget::play);
    connect(d_ptr->useGpuCheckBox, &QCheckBox::toggled, this, [this] {
        emit useGpu(d_ptr->useGpuCheckBox->isChecked());
    });
    connect(d_ptr->volumeSlider, &QSlider::valueChanged, this, &ControlWidget::volumeChanged);
    connect(d_ptr->speedCbx, &QComboBox::currentIndexChanged, this, &ControlWidget::onSpeedChanged);

    connect(d_ptr->audioTracksCbx,
            &QComboBox::currentTextChanged,
            this,
            &ControlWidget::audioTrackChanged);
    connect(d_ptr->subTracksCbx,
            &QComboBox::currentTextChanged,
            this,
            &ControlWidget::subTrackChanged);
}
