#include "mainwindow.h"
#include "playerwidget.h"
#include "player.h"

#include <QtWidgets>

class MainWindowPrivate{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent){
        player = new Player(owner);
    }
    QWidget *owner;
    Player *player;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
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

void MainWindow::setupUI()
{
    PlayerWidget *playWidget = new PlayerWidget(this);
    QPushButton *playButton = new QPushButton(tr("play"), this);
    connect(d_ptr->player, &Player::error, this, &MainWindow::onError);
    connect(d_ptr->player, &Player::readyRead, playWidget, &PlayerWidget::onReadyRead);
    connect(playWidget, &PlayerWidget::openFile, d_ptr->player, &Player::onSetFilePath);
    connect(playButton, &QPushButton::clicked, d_ptr->player, &Player::onPlay);

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->addWidget(playWidget);
    layout->addWidget(playButton);
    setCentralWidget(widget);
}

