#include "mainwindow.h"
#include "playerwidget.h"
#include "slider.h"

#include <ffmpeg/averror.h>
#include <ffmpeg/player.h>

#include <QtWidgets>

class MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent)
    {
        player = new Ffmpeg::Player(owner);
        slider = new Slider(owner);
        positionLabel = new QLabel("00:00:00", owner);
        durationLabel = new QLabel("/00:00:00", owner);
    }
    QWidget *owner;
    Ffmpeg::Player *player;

    Slider *slider;
    QLabel *positionLabel;
    QLabel *durationLabel;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
    buildConnect();
    resize(1000, 600);
}

MainWindow::~MainWindow()
{
    d_ptr->player->onStop();
}

void MainWindow::onError(const AVError &avError)
{
    const QString str = tr("Error[%1]:%2.")
                            .arg(QString::number(avError.error()), avError.errorString());
    qWarning() << str;
}

void MainWindow::onDurationChanged(qint64 duration)
{
    d_ptr->durationLabel->setText("/"
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

void MainWindow::setupUI()
{
    PlayerWidget *playWidget = new PlayerWidget(this);
    d_ptr->player->setVideoOutputWidget(playWidget);
    QPushButton *playButton = new QPushButton(tr("play"), this);
    playButton->setCheckable(true);
    connect(playWidget, &PlayerWidget::openFile, d_ptr->player, &Ffmpeg::Player::onSetFilePath);
    connect(playButton, &QPushButton::clicked, [this](bool checked) {
        if (checked && !d_ptr->player->isRunning())
            d_ptr->player->onPlay();
        else {
            d_ptr->player->pause(!checked);
        }
    });
    connect(d_ptr->player,
            &Ffmpeg::Player::stateChanged,
            [playButton](Ffmpeg::Player::MediaState state) {
                switch (state) {
                case Ffmpeg::Player::MediaState::StoppedState:
                case Ffmpeg::Player::MediaState::PausedState: playButton->setChecked(false); break;
                case Ffmpeg::Player::MediaState::PlayingState: playButton->setChecked(true); break;
                default: break;
                }
            });

    Slider *volumeSlider = new Slider(this);
    connect(volumeSlider, &QSlider::sliderMoved, [this](int value) {
        d_ptr->player->setVolume(value / 100.0);
    });
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);

    QComboBox *speedComboBox = new QComboBox(this);
    connect(speedComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, speedComboBox](int index) {
                d_ptr->player->setSpeed(speedComboBox->itemData(index).toDouble());
            });
    for (double i = 0.5; i < 5.5; i += 0.5) {
        speedComboBox->addItem(QString::number(i), i);
    }
    speedComboBox->setCurrentIndex(1);

    QComboBox *audioTracksComboBox = new QComboBox(this);
    audioTracksComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(d_ptr->player,
            &Ffmpeg::Player::audioTracksChanged,
            [audioTracksComboBox](const QStringList &tracks) {
                audioTracksComboBox->blockSignals(true);
                audioTracksComboBox->clear();
                audioTracksComboBox->addItems(tracks);
                audioTracksComboBox->blockSignals(false);
            });
    connect(d_ptr->player,
            &Ffmpeg::Player::audioTrackChanged,
            [audioTracksComboBox](const QString &track) {
                audioTracksComboBox->blockSignals(true);
                audioTracksComboBox->setCurrentText(track);
                audioTracksComboBox->blockSignals(false);
            });
    connect(audioTracksComboBox,
            &QComboBox::currentTextChanged,
            d_ptr->player,
            &Ffmpeg::Player::onSetAudioTracks);

    QComboBox *subtitleStreamsComboBox = new QComboBox(this);
    subtitleStreamsComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ;
    connect(d_ptr->player,
            &Ffmpeg::Player::subtitleStreamsChanged,
            [subtitleStreamsComboBox](const QStringList &streams) {
                subtitleStreamsComboBox->blockSignals(true);
                subtitleStreamsComboBox->clear();
                subtitleStreamsComboBox->addItems(streams);
                subtitleStreamsComboBox->blockSignals(false);
            });
    connect(d_ptr->player,
            &Ffmpeg::Player::subtitleStreamChanged,
            [subtitleStreamsComboBox](const QString &stream) {
                subtitleStreamsComboBox->blockSignals(true);
                subtitleStreamsComboBox->setCurrentText(stream);
                subtitleStreamsComboBox->blockSignals(false);
            });
    connect(subtitleStreamsComboBox,
            &QComboBox::currentTextChanged,
            d_ptr->player,
            &Ffmpeg::Player::onSetSubtitleStream);

    QWidget *processWidget = new QWidget(this);
    processWidget->setMaximumHeight(100);
    QHBoxLayout *processLayout = new QHBoxLayout(processWidget);
    processLayout->addWidget(d_ptr->slider);
    processLayout->addWidget(d_ptr->positionLabel);
    processLayout->addWidget(d_ptr->durationLabel);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addWidget(playButton);
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
    connect(d_ptr->player, &Ffmpeg::Player::error, this, &MainWindow::onError);
    connect(d_ptr->player, &Ffmpeg::Player::durationChanged, this, &MainWindow::onDurationChanged);
    connect(d_ptr->player, &Ffmpeg::Player::positionChanged, this, &MainWindow::onPositionChanged);
    connect(d_ptr->slider, &QSlider::sliderMoved, d_ptr->player, &Ffmpeg::Player::onSeek);
}
