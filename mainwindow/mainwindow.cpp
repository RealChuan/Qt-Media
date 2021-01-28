#include "mainwindow.h"
#include "playerwidget.h"
#include "slider.h"

#include <ffmpeg/player.h>

#include <QtWidgets>

class MainWindowPrivate{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent){
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
    d_ptr->player->stop();
}

void MainWindow::onError(const QString &error)
{
    qWarning() << error;
}

void MainWindow::onDurationChanged(qint64 duration)
{
    d_ptr->durationLabel->setText("/" + QTime::fromMSecsSinceStartOfDay(duration).toString("hh:mm:ss"));
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
    QPushButton *playButton = new QPushButton(tr("play"), this);
    playButton->setCheckable(true);
    connect(d_ptr->player, &Ffmpeg::Player::readyRead, playWidget, &PlayerWidget::onReadyRead);
    connect(playWidget, &PlayerWidget::openFile, d_ptr->player, &Ffmpeg::Player::onSetFilePath);
    connect(playButton, &QPushButton::clicked, [this](bool checked){
        if(checked && !d_ptr->player->isRunning())
            d_ptr->player->onPlay();
        else{
            d_ptr->player->pause(!checked);
        }
    });

    QWidget *processWidget = new QWidget(this);
    processWidget->setMaximumHeight(100);
    QHBoxLayout *processLayout = new QHBoxLayout(processWidget);
    processLayout->addWidget(d_ptr->slider);
    processLayout->addWidget(d_ptr->positionLabel);
    processLayout->addWidget(d_ptr->durationLabel);

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->addWidget(playWidget);
    layout->addWidget(processWidget);
    layout->addWidget(playButton);
    setCentralWidget(widget);
}

void MainWindow::buildConnect()
{
    connect(d_ptr->player, &Ffmpeg::Player::error, this, &MainWindow::onError);
    connect(d_ptr->player, &Ffmpeg::Player::durationChanged, this, &MainWindow::onDurationChanged);
    connect(d_ptr->player, &Ffmpeg::Player::positionChanged, this, &MainWindow::onPositionChanged);
    connect(d_ptr->slider, &QSlider::sliderMoved, d_ptr->player, &Ffmpeg::Player::onSeek);
}

