#include "mainwindow.h"
#include "openwebmediadialog.hpp"
#include "slider.h"

#include <ffmpeg/averror.h>
#include <ffmpeg/player.h>
#include <ffmpeg/videorender/videopreviewwidget.hpp>
#include <ffmpeg/videorender/videorendercreate.hpp>

#include <QtWidgets>

class MainWindow::MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent)
        , playerPtr(new Ffmpeg::Player)
    {
        menu = new QMenu(owner);

        slider = new Slider(owner);
        positionLabel = new QLabel("00:00:00", owner);
        durationLabel = new QLabel("/ 00:00:00", owner);
        sourceFPSLabel = new QLabel("FPS: 00.00->", owner);
        currentFPSLabel = new QLabel("00.00", owner);
        playButton = new QPushButton(QObject::tr("play", "MainWindow"), owner);
        playButton->setCheckable(true);

        fpsTimer = new QTimer(owner);
    }
    ~MainWindowPrivate() {}

    QWidget *owner;
    QScopedPointer<Ffmpeg::Player> playerPtr;
    QScopedPointer<Ffmpeg::VideoRender> videoRender;
    QScopedPointer<Ffmpeg::VideoPreviewWidget> videoPreviewWidgetPtr;

    QMenu *menu;

    Slider *slider;
    QLabel *positionLabel;
    QLabel *durationLabel;
    QLabel *sourceFPSLabel;
    QLabel *currentFPSLabel;
    QPushButton *playButton;
    QTimer *fpsTimer;

    QVBoxLayout *layout;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
    buildConnect();
    initShortcut();
    initMenu();
    resize(1000, 650);
}

MainWindow::~MainWindow()
{
    d_ptr->playerPtr->onStop();
    d_ptr->playerPtr->setVideoRenders({});
}

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
    auto fpsStr = QString("FPS: %1->").arg(QString::number(fps, 'f', 2));
    d_ptr->sourceFPSLabel->setText(fpsStr);
    d_ptr->sourceFPSLabel->setToolTip(fpsStr);

    auto size = d_ptr->playerPtr->resolutionRatio();
    auto url = d_ptr->playerPtr->filePath();
    auto filename = QFile::exists(url) ? QFileInfo(url).fileName()
                                       : QFileInfo(QUrl(url).toString()).fileName();
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
    if (d_ptr->videoPreviewWidgetPtr.isNull()) {
        d_ptr->videoPreviewWidgetPtr.reset(new Ffmpeg::VideoPreviewWidget);
        d_ptr->videoPreviewWidgetPtr->setWindowFlags(d_ptr->videoPreviewWidgetPtr->windowFlags()
                                                     | Qt::Tool | Qt::FramelessWindowHint
                                                     | Qt::WindowStaysOnTopHint);
    }
    d_ptr->videoPreviewWidgetPtr->startPreview(filePath, index, value, d_ptr->slider->maximum());

    int w = 320;
    int h = 200;
    d_ptr->videoPreviewWidgetPtr->setFixedSize(w, h);
    auto gpos = d_ptr->slider->mapToGlobal(d_ptr->slider->pos() + QPoint(pos, 0));
    d_ptr->videoPreviewWidgetPtr->move(gpos - QPoint(w / 2, h + 15));
    d_ptr->videoPreviewWidgetPtr->show();
}

void MainWindow::onLeaveSlider()
{
    if (!d_ptr->videoPreviewWidgetPtr.isNull()) {
        d_ptr->videoPreviewWidgetPtr->hide();
        d_ptr->videoPreviewWidgetPtr->clearAllTask();
    }
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

void MainWindow::onOpenLocalMedia()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath
        = QFileDialog::getOpenFileName(this,
                                       tr("Open File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->playerPtr->onSetFilePath(filePath);
}

void MainWindow::onOpenWebMedia()
{
    OpenWebMediaDialog dialog(this);
    dialog.setMinimumSize(size() / 2.0);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    d_ptr->playerPtr->onSetFilePath(QUrl(dialog.url()).toEncoded());
}

void MainWindow::onRenderChanged()
{
    auto action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    Ffmpeg::VideoRenderCreate::RenderType renderType = Ffmpeg::VideoRenderCreate::Opengl;
    auto type = action->data().toInt();
    switch (type) {
    case 1: renderType = Ffmpeg::VideoRenderCreate::Widget; break;
    default: renderType = Ffmpeg::VideoRenderCreate::Opengl; break;
    }
    auto videoRender = Ffmpeg::VideoRenderCreate::create(renderType);
    videoRender->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoRender->widget()->setAcceptDrops(true);
    videoRender->widget()->installEventFilter(this);
    d_ptr->playerPtr->setVideoRenders({videoRender});
    d_ptr->layout->insertWidget(0, videoRender->widget());
    // 为什么切换成widget还是有使用GPU 0-3D，而且使用量是切换为opengl的两倍！！！
    d_ptr->videoRender.reset(videoRender);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (!d_ptr->videoRender.isNull() && watched == d_ptr->videoRender->widget()) {
        switch (event->type()) {
        case QEvent::DragEnter: {
            auto e = static_cast<QDragEnterEvent *>(event);
            e->acceptProposedAction();
        } break;
        case QEvent::DragMove: {
            auto e = static_cast<QDragMoveEvent *>(event);
            e->acceptProposedAction();
        } break;
        case QEvent::Drop: {
            auto e = static_cast<QDropEvent *>(event);
            QList<QUrl> urls = e->mimeData()->urls();
            if (!urls.isEmpty()) {
                d_ptr->playerPtr->onSetFilePath(urls.first().toLocalFile());
            }
        } break;
        case QEvent::ContextMenu: {
            auto e = static_cast<QContextMenuEvent *>(event);
            d_ptr->menu->exec(e->globalPos());
        } break;
        default: break;
        }
    }
    return QMainWindow::eventFilter(watched, event);
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
    auto useGpuCheckBox = new QCheckBox(tr("GPU Decode"), this);
    useGpuCheckBox->setToolTip(tr("GPU Decode"));
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
    auto audioTracksView = new QListView(audioTracksComboBox);
    audioTracksView->setTextElideMode(Qt::ElideRight);
    audioTracksView->setAlternatingRowColors(true);
    audioTracksComboBox->setView(audioTracksView);
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
    auto subtitleStreamsView = new QListView(subtitleStreamsComboBox);
    subtitleStreamsView->setTextElideMode(Qt::ElideRight);
    subtitleStreamsView->setAlternatingRowColors(true);
    subtitleStreamsComboBox->setView(subtitleStreamsView);
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
    d_ptr->layout = new QVBoxLayout(widget);
    d_ptr->layout->addWidget(processWidget);
    d_ptr->layout->addLayout(controlLayout);
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

    connect(d_ptr->fpsTimer, &QTimer::timeout, this, &MainWindow::onShowCurrentFPS);
}

void MainWindow::initShortcut()
{
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
}

void MainWindow::initMenu()
{
    d_ptr->menu->addAction(tr("Open Local Media"), this, &MainWindow::onOpenLocalMedia);
    d_ptr->menu->addAction(tr("Open Web Media"), this, &MainWindow::onOpenWebMedia);

    auto widgetAction = new QAction(tr("Widget"), this);
    widgetAction->setCheckable(true);
    widgetAction->setData(Ffmpeg::VideoRenderCreate::Widget);
    connect(widgetAction,
            &QAction::triggered,
            this,
            &MainWindow::onRenderChanged,
            Qt::QueuedConnection);
    auto openglAction = new QAction(tr("Opengl"), this);
    openglAction->setCheckable(true);
    openglAction->setData(Ffmpeg::VideoRenderCreate::Opengl);
    connect(openglAction,
            &QAction::triggered,
            this,
            &MainWindow::onRenderChanged,
            Qt::QueuedConnection);
    auto actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);
    actionGroup->addAction(widgetAction);
    actionGroup->addAction(openglAction);

    auto renderMenu = new QMenu(tr("Video Render"), this);
    renderMenu->addAction(widgetAction);
    renderMenu->addAction(openglAction);
    d_ptr->menu->addMenu(renderMenu);

    openglAction->trigger();
}
