#include "mainwindow.h"
#include "playerwidget.h"
#include "slider.h"

#include <ffmpeg/averror.h>
#include <ffmpeg/player.h>
#include <ffmpeg/videooutput/videopreviewwidget.hpp>

#include <QtWidgets>

class MainWindow::MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent)
        , playerPtr(new Ffmpeg::Player)
    {
        slider = new Slider(owner);
        positionLabel = new QLabel("00:00:00", owner);
        durationLabel = new QLabel("/ 00:00:00", owner);
        sourceFPSLabel = new QLabel(owner);
        currentFPSLabel = new QLabel(owner);
        playButton = new QPushButton(QObject::tr("play", "MainWindow"), owner);
        playButton->setCheckable(true);

        fpsTimer = new QTimer(owner);
    }
    ~MainWindowPrivate() {}

    QWidget *owner;
    QScopedPointer<Ffmpeg::Player> playerPtr;
    QScopedPointer<Ffmpeg::VideoPreviewWidget> videoPreviewWidgetPtr;

    Slider *slider;
    QLabel *positionLabel;
    QLabel *durationLabel;
    QLabel *sourceFPSLabel;
    QLabel *currentFPSLabel;
    QPushButton *playButton;
    QTimer *fpsTimer;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
    buildConnect();
    resize(1000, 650);
}

MainWindow::~MainWindow() {}

void MainWindow::onError(const Ffmpeg::AVError &avError)
{
    const QString str = tr("Error[%1]:%2.")
                            .arg(QString::number(avError.error()), avError.errorString());
    qWarning() << str;
}

void MainWindow::onDurationChanged(qint64 duration)
{
    d_ptr->durationLabel->setText("/ "
                                  + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss"));
    d_ptr->slider->blockSignals(true);
    d_ptr->slider->setRange(0, duration / 1000);
    d_ptr->slider->blockSignals(false);
}

void MainWindow::onPositionChanged(qint64 position)
{
    d_ptr->positionLabel->setText(QTime::fromMSecsSinceStartOfDay(position).toString("hh:mm:ss"));
    d_ptr->slider->setValue(position / 1000);
}

void MainWindow::onStarted()
{
    auto fps = d_ptr->playerPtr->fps();
    auto fpsStr = QString("FPS:%1->").arg(QString::number(fps, 'f', 2));
    d_ptr->sourceFPSLabel->setText(fpsStr);
    d_ptr->sourceFPSLabel->setToolTip(fpsStr);

    auto size = d_ptr->playerPtr->resolutionRatio();
    auto filename = QFileInfo(d_ptr->playerPtr->filePath()).fileName();
    setWindowTitle(
        QString("%1[%2x%3]")
            .arg(filename, QString::number(size.width()), QString::number(size.height())));

    d_ptr->fpsTimer->start(1000);
}

void MainWindow::onFinished()
{
    d_ptr->fpsTimer->stop();
    onDurationChanged(0);
    onPositionChanged(0);
}

void MainWindow::onHoverSlider(int pos, int value)
{
    auto index = d_ptr->playerPtr->videoIndex();
    if (index < 0) {
        return;
    }
    auto filePath = d_ptr->playerPtr->filePath();
    if (filePath.isEmpty()) {
        return;
    }
    if (d_ptr->playerPtr->isFinished()) {
        return;
    }
    d_ptr->videoPreviewWidgetPtr.reset(
        new Ffmpeg::VideoPreviewWidget(filePath, index, value, d_ptr->slider->maximum()));
    d_ptr->videoPreviewWidgetPtr->setWindowFlags(d_ptr->videoPreviewWidgetPtr->windowFlags()
                                                 | Qt::Tool | Qt::FramelessWindowHint
                                                 | Qt::WindowStaysOnTopHint);
    int w = 320;
    int h = 200;
    d_ptr->videoPreviewWidgetPtr->setFixedSize(w, h);
    auto gpos = d_ptr->slider->mapToGlobal(d_ptr->slider->pos() + QPoint(pos, 0));
    d_ptr->videoPreviewWidgetPtr->move(gpos - QPoint(w / 2, h + 15));
    d_ptr->videoPreviewWidgetPtr->show();
}

void MainWindow::onLeaveSlider()
{
    d_ptr->videoPreviewWidgetPtr.reset();
}

void MainWindow::onShowCurrentFPS()
{
    auto renders = d_ptr->playerPtr->videoRenders();
    if (renders.isEmpty()) {
        d_ptr->fpsTimer->stop();
    }
    auto fps = renders[0]->fps();
    auto fpsStr = QString::number(fps, 'f', 2);
    d_ptr->currentFPSLabel->setText(fpsStr);
    d_ptr->currentFPSLabel->setToolTip(fpsStr);
}

void MainWindow::keyPressEvent(QKeyEvent *ev)
{
    QMainWindow::keyPressEvent(ev);

    qDebug() << ev->key();
    switch (ev->key()) {
    case Qt::Key_Space:
        d_ptr->playButton->click();
        break;
        //    useless
        //    case Qt::Key_Right: d_ptr->player->onSeek(d_ptr->slider->value() + 5); break;
        //    case Qt::Key_Left: d_ptr->player->onSeek(d_ptr->slider->value() - 5); break;
    case Qt::Key_Q: qApp->quit(); break;
    default: break;
    }
}

void MainWindow::setupUI()
{
    auto playWidget = new PlayerWidget(this);
    playWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d_ptr->playerPtr->setVideoOutputWidget(QVector<Ffmpeg::VideoRender *>{playWidget});
    connect(playWidget,
            &PlayerWidget::openFile,
            d_ptr->playerPtr.data(),
            &Ffmpeg::Player::onSetFilePath);

    auto useGpuCheckBox = new QCheckBox(tr("GPU Decode"), this);
    useGpuCheckBox->setToolTip(tr("The pre-play Settings are valid"));
    useGpuCheckBox->setChecked(true);
    connect(useGpuCheckBox, &QCheckBox::clicked, this, [this, useGpuCheckBox] {
        d_ptr->playerPtr->setUseGpuDecode(useGpuCheckBox->isChecked());
    });
    d_ptr->playerPtr->setUseGpuDecode(useGpuCheckBox->isChecked());

    auto volumeSlider = new Slider(this);
    connect(volumeSlider, &QSlider::sliderMoved, this, [this](int value) {
        d_ptr->playerPtr->setVolume(value / 100.0);
    });
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);

    auto speedComboBox = new QComboBox(this);
    connect(speedComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this, speedComboBox](int index) {
                d_ptr->playerPtr->setSpeed(speedComboBox->itemData(index).toDouble());
            });
    double i = 0.5;
    while (i <= 2) {
        speedComboBox->addItem(QString::number(i), i);
        i += 0.5;
    }
    speedComboBox->setCurrentIndex(1);

    auto audioTracksComboBox = new QComboBox(this);
    audioTracksComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::audioTracksChanged,
            this,
            [audioTracksComboBox](const QStringList &tracks) {
                audioTracksComboBox->blockSignals(true);
                audioTracksComboBox->clear();
                audioTracksComboBox->addItems(tracks);
                audioTracksComboBox->blockSignals(false);
            });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::audioTrackChanged,
            this,
            [audioTracksComboBox](const QString &track) {
                audioTracksComboBox->blockSignals(true);
                audioTracksComboBox->setCurrentText(track);
                audioTracksComboBox->blockSignals(false);
            });
    connect(audioTracksComboBox,
            &QComboBox::currentTextChanged,
            d_ptr->playerPtr.data(),
            &Ffmpeg::Player::onSetAudioTracks);

    QComboBox *subtitleStreamsComboBox = new QComboBox(this);
    subtitleStreamsComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::subtitleStreamsChanged,
            this,
            [subtitleStreamsComboBox](const QStringList &streams) {
                subtitleStreamsComboBox->blockSignals(true);
                subtitleStreamsComboBox->clear();
                subtitleStreamsComboBox->addItems(streams);
                subtitleStreamsComboBox->blockSignals(false);
            });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::subtitleStreamChanged,
            this,
            [subtitleStreamsComboBox](const QString &stream) {
                subtitleStreamsComboBox->blockSignals(true);
                subtitleStreamsComboBox->setCurrentText(stream);
                subtitleStreamsComboBox->blockSignals(false);
            });
    connect(subtitleStreamsComboBox,
            &QComboBox::currentTextChanged,
            d_ptr->playerPtr.data(),
            &Ffmpeg::Player::onSetSubtitleStream);

    QWidget *processWidget = new QWidget(this);
    //processWidget->setMaximumHeight(70);
    QHBoxLayout *processLayout = new QHBoxLayout(processWidget);
    processLayout->addWidget(d_ptr->slider);
    processLayout->addWidget(d_ptr->positionLabel);
    processLayout->addWidget(d_ptr->durationLabel);
    processLayout->addWidget(d_ptr->sourceFPSLabel);
    processLayout->addWidget(d_ptr->currentFPSLabel);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(d_ptr->playButton);
    controlLayout->addWidget(useGpuCheckBox);
    controlLayout->addWidget(new QLabel(tr("Volume: "), this));
    controlLayout->addWidget(volumeSlider);
    controlLayout->addWidget(new QLabel(tr("Speed: "), this));
    controlLayout->addWidget(speedComboBox);
    controlLayout->addWidget(new QLabel(tr("Audio Tracks: "), this));
    controlLayout->addWidget(audioTracksComboBox);
    controlLayout->addWidget(new QLabel(tr("Subtitle: "), this));
    controlLayout->addWidget(subtitleStreamsComboBox);

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->addWidget(playWidget);
    layout->addWidget(processWidget);
    layout->addLayout(controlLayout);
    setCentralWidget(widget);
}

void MainWindow::buildConnect()
{
    connect(d_ptr->playerPtr.data(), &Ffmpeg::Player::error, this, &MainWindow::onError);
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::durationChanged,
            this,
            &MainWindow::onDurationChanged);
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::positionChanged,
            this,
            &MainWindow::onPositionChanged);
    connect(d_ptr->playerPtr.data(), &Ffmpeg::Player::playStarted, this, &MainWindow::onStarted);
    connect(d_ptr->playerPtr.data(), &Ffmpeg::Player::finished, this, &MainWindow::onFinished);

    connect(d_ptr->slider, &Slider::sliderMoved, d_ptr->playerPtr.data(), &Ffmpeg::Player::onSeek);
    connect(d_ptr->slider, &Slider::onHover, this, &MainWindow::onHoverSlider);
    connect(d_ptr->slider, &Slider::onLeave, this, &MainWindow::onLeaveSlider);

    connect(d_ptr->playButton, &QPushButton::clicked, this, [this](bool checked) {
        if (checked && !d_ptr->playerPtr->isRunning())
            d_ptr->playerPtr->onPlay();
        else {
            d_ptr->playerPtr->pause(!checked);
        }
    });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::stateChanged,
            d_ptr->playButton,
            [this](Ffmpeg::Player::MediaState state) {
                switch (state) {
                case Ffmpeg::Player::MediaState::StoppedState:
                case Ffmpeg::Player::MediaState::PausedState:
                    d_ptr->playButton->setChecked(false);
                    break;
                case Ffmpeg::Player::MediaState::PlayingState:
                    d_ptr->playButton->setChecked(true);
                    break;
                default: break;
                }
            });

    new QShortcut(QKeySequence::MoveToNextChar, this, this, [this] {
        d_ptr->playerPtr->onSeek(d_ptr->slider->value() + 5);
    });
    new QShortcut(QKeySequence::MoveToPreviousChar, this, this, [this] {
        auto value = d_ptr->slider->value() - 10;
        if (value < 0) {
            value = 0;
        }
        d_ptr->playerPtr->onSeek(value);
    });

    connect(d_ptr->fpsTimer, &QTimer::timeout, this, &MainWindow::onShowCurrentFPS);
}
