#include "mainwindow.h"
#include "controlwidget.hpp"
#include "openwebmediadialog.hpp"
#include "playlistmodel.h"
#include "playlistview.hpp"
#include "qmediaplaylist.h"
#include "titlewidget.hpp"

#include <ffmpeg/averror.h>
#include <ffmpeg/ffmpegutils.hpp>
#include <ffmpeg/player.h>
#include <ffmpeg/videorender/videopreviewwidget.hpp>
#include <ffmpeg/videorender/videorendercreate.hpp>

#include <QtWidgets>

static bool isPlaylist(const QUrl &url) // Check for ".m3u" playlists.
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists()
           && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

class MainWindow::MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent)
        , playerPtr(new Ffmpeg::Player)
    {
        controlWidget = new ControlWidget(owner);
        controlWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        titleWidget = new TitleWidget(owner);

        playlistModel = new PlaylistModel(owner);
        playlistView = new PlayListView(owner);
        playlistView->setModel(playlistModel);
        playlistView->setCurrentIndex(
            playlistModel->index(playlistModel->playlist()->currentIndex(), 0));
        //playlistView->setMaximumWidth(250);

        menu = new QMenu(owner);
        audioTracksMenu = new QMenu(QObject::tr("Select audio track"), owner);
        subTracksMenu = new QMenu(QObject::tr("Select subtitle track"), owner);
        audioTracksGroup = new QActionGroup(owner);
        audioTracksGroup->setExclusive(true);
        subTracksGroup = new QActionGroup(owner);
        subTracksGroup->setExclusive(true);

        playListMenu = new QMenu(owner);

        fpsTimer = new QTimer(owner);

        splitter = new QSplitter(owner);
        splitter->addWidget(playlistView);

        initShortcut();
    }

    ~MainWindowPrivate() {}

    void initShortcut()
    {
        new QShortcut(QKeySequence::MoveToNextChar, owner, owner, [this] {
            playerPtr->onSeek(controlWidget->position() + 5);
            titleWidget->setText(tr("Fast forward: 5 seconds"));
            titleWidget->setAutoHide(3000);
            setTitleWidgetGeometry(true);
        });
        new QShortcut(QKeySequence::MoveToPreviousChar, owner, owner, [this] {
            auto value = controlWidget->position() - 10;
            if (value < 0) {
                value = 0;
            }
            playerPtr->onSeek(value);
            titleWidget->setText(tr("Fast return: 10 seconds"));
            titleWidget->setAutoHide(3000);
            setTitleWidgetGeometry(true);
        });
        new QShortcut(QKeySequence::MoveToPreviousLine, owner, owner, [this] {
            controlWidget->setVolume(controlWidget->volume() + 5);
        });
        new QShortcut(QKeySequence::MoveToNextLine, owner, owner, [this] {
            controlWidget->setVolume(controlWidget->volume() - 5);
        });
        new QShortcut(Qt::Key_Space, owner, owner, [this] { controlWidget->clickPlayButton(); });
    }

    void setControlWidgetGeometry(bool show = true)
    {
        if (videoRender.isNull()) {
            return;
        }
        controlWidget->setMinimumWidth(videoRender->widget()->width() / 2);
        controlWidget->adjustSize();
        if (controlWidget->width() * 2 < videoRender->widget()->width()) {
            controlWidget->setMinimumWidth(videoRender->widget()->width() / 2);
        }
        auto margain = 10;
        auto geometry = videoRender->widget()->geometry();
        auto p1 = QPoint(geometry.x() + (geometry.width() - controlWidget->width()) / 2.0,
                         geometry.bottomLeft().y() - controlWidget->height() - margain);
        globalControlWidgetGeometry = {owner->mapToGlobal(p1), controlWidget->size()};
        controlWidget->setFixedSize(globalControlWidgetGeometry.size());
        controlWidget->setGeometry(globalControlWidgetGeometry);
        controlWidget->setVisible(show);
    }

    void setTitleWidgetGeometry(bool show = true)
    {
        if (videoRender.isNull()) {
            return;
        }
        auto margain = 10;
        auto geometry = videoRender->widget()->geometry();
        auto p1 = QPoint(geometry.x() + margain, geometry.y() + margain);
        auto p2 = QPoint(geometry.topRight().x() - margain, geometry.y() + margain + 80);
        globalTitlelWidgetGeometry = {owner->mapToGlobal(p1), owner->mapToGlobal(p2)};
        titleWidget->setFixedSize(globalTitlelWidgetGeometry.size());
        titleWidget->setGeometry(globalTitlelWidgetGeometry);
        titleWidget->setVisible(show);
    }

    QWidget *owner;
    QScopedPointer<Ffmpeg::Player> playerPtr;
    QScopedPointer<Ffmpeg::VideoRender> videoRender;
    QScopedPointer<Ffmpeg::VideoPreviewWidget> videoPreviewWidgetPtr;

    ControlWidget *controlWidget;
    QRect globalControlWidgetGeometry;
    TitleWidget *titleWidget;
    QRect globalTitlelWidgetGeometry;

    PlayListView *playlistView;
    PlaylistModel *playlistModel;

    QMenu *menu;
    QMenu *audioTracksMenu;
    QMenu *subTracksMenu;
    QActionGroup *audioTracksGroup;
    QActionGroup *subTracksGroup;

    QMenu *playListMenu;

    QTimer *fpsTimer;

    QSplitter *splitter;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    Ffmpeg::Utils::printFfmpegInfo();

    setupUI();
    buildConnect();
    initMenu();
    initPlayListMenu();

    setAttribute(Qt::WA_Hover);
    d_ptr->playlistView->installEventFilter(this);
    installEventFilter(this);

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
                            .arg(QString::number(avError.errorCode()), avError.errorString());
    qWarning() << str;
}

void MainWindow::onStarted()
{
    d_ptr->controlWidget->setSourceFPS(d_ptr->playerPtr->fps());

    auto size = d_ptr->playerPtr->resolutionRatio();
    setWindowTitle(QString("%1[%2x%3]")
                       .arg(d_ptr->playlistModel->playlist()->currentMedia().fileName(),
                            QString::number(size.width()),
                            QString::number(size.height())));

    d_ptr->fpsTimer->start(1000);
}

void MainWindow::onFinished()
{
    d_ptr->fpsTimer->stop();
    d_ptr->controlWidget->onDurationChanged(0);
    d_ptr->controlWidget->onPositionChanged(0);
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
    d_ptr->videoPreviewWidgetPtr->startPreview(filePath,
                                               index,
                                               value,
                                               d_ptr->controlWidget->duration());

    int w = 320;
    int h = 200;
    d_ptr->videoPreviewWidgetPtr->setFixedSize(w, h);
    auto gpos = d_ptr->controlWidget->sliderGlobalPos() + QPoint(pos, 0);
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
    d_ptr->controlWidget->setCurrentFPS(renders[0]->fps());
}

void MainWindow::onOpenLocalMedia()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const auto urls = QFileDialog::getOpenFileUrls(this,
                                                   tr("Open File"),
                                                   path,
                                                   tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (urls.isEmpty()) {
        return;
    }
    addToPlaylist(urls);
}

void MainWindow::onOpenWebMedia()
{
    OpenWebMediaDialog dialog(this);
    dialog.setMinimumSize(size() / 2.0);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    addToPlaylist({QUrl(dialog.url())});
}

void MainWindow::onRenderChanged(QAction *action)
{
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
    d_ptr->splitter->insertWidget(0, videoRender->widget());
    // 为什么切换成widget还是有使用GPU 0-3D，而且使用量是切换为opengl的两倍！！！
    d_ptr->videoRender.reset(videoRender);
}

void MainWindow::playlistPositionChanged(int currentItem)
{
    auto url = d_ptr->playlistModel->playlist()->currentMedia();
    d_ptr->playlistView->setCurrentIndex(d_ptr->playlistModel->index(currentItem, 0));
    d_ptr->playerPtr->openMedia(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

void MainWindow::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        d_ptr->playlistModel->playlist()->setCurrentIndex(index.row());
    }
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
                addToPlaylist(urls);
            }
        } break;
        case QEvent::ContextMenu: {
            auto e = static_cast<QContextMenuEvent *>(event);
            d_ptr->menu->exec(e->globalPos());
        } break;
        case QEvent::Resize:
            QMetaObject::invokeMethod(
                this,
                [=] {
                    d_ptr->setControlWidgetGeometry(d_ptr->controlWidget->isVisible());
                    d_ptr->setTitleWidgetGeometry(d_ptr->titleWidget->isVisible());
                },
                Qt::QueuedConnection);
            break;
            //        case QEvent::MouseButtonPress: {
            //            auto e = static_cast<QMouseEvent *>(event);
            //            if (e->button() & Qt::LeftButton) {
            //                d_ptr->mpvPlayer->pause();
            //            }
            //        } break;
        case QEvent::MouseButtonDblClick:
            if (isFullScreen()) {
                showNormal();
            } else {
                d_ptr->playlistView->hide();
                showFullScreen();
            }
            break;
        default: break;
        }
    } else if (watched == d_ptr->playlistView) {
        switch (event->type()) {
        case QEvent::ContextMenu: {
            auto e = static_cast<QContextMenuEvent *>(event);
            d_ptr->playListMenu->exec(e->globalPos());
        } break;
        default: break;
        }
    } else if (watched == this) {
        switch (event->type()) {
        case QEvent::Show:
        case QEvent::Move:
            QMetaObject::invokeMethod(
                this,
                [=] {
                    d_ptr->setControlWidgetGeometry(d_ptr->controlWidget->isVisible());
                    d_ptr->setTitleWidgetGeometry(d_ptr->titleWidget->isVisible());
                },
                Qt::QueuedConnection);
            break;
        case QEvent::Hide: d_ptr->controlWidget->hide(); break;
        case QEvent::HoverMove:
            if (d_ptr->globalControlWidgetGeometry.isValid()) {
                auto e = static_cast<QHoverEvent *>(event);
                bool contain = d_ptr->globalControlWidgetGeometry.contains(
                    e->globalPosition().toPoint());
                bool isVisible = d_ptr->controlWidget->isVisible();
                if (contain && !isVisible) {
                    d_ptr->controlWidget->show();
                } else if (!contain && isVisible) {
                    d_ptr->controlWidget->hide();
                }
            }
            if (d_ptr->globalTitlelWidgetGeometry.isValid() && isFullScreen()) {
                auto e = static_cast<QHoverEvent *>(event);
                bool contain = d_ptr->globalTitlelWidgetGeometry.contains(
                    e->globalPosition().toPoint());
                bool isVisible = d_ptr->titleWidget->isVisible();
                if (contain && !isVisible) {
                    d_ptr->titleWidget->setText(windowTitle());
                    d_ptr->titleWidget->show();
                } else if (!contain && isVisible) {
                    d_ptr->titleWidget->hide();
                }
            }
            break;
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
    case Qt::Key_Escape:
        if (isFullScreen()) {
            showNormal();
        } else {
            showMinimized();
        }
        break;
    case Qt::Key_Q: qApp->quit(); break;
    default: break;
    }
}

void MainWindow::setupUI()
{
    setCentralWidget(d_ptr->splitter);
}

void MainWindow::buildConnect()
{
    connect(d_ptr->playerPtr.data(), &Ffmpeg::Player::error, this, &MainWindow::onError);
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::durationChanged,
            d_ptr->controlWidget,
            [this](qint64 duration) { d_ptr->controlWidget->onDurationChanged(duration / 1000); });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::positionChanged,
            d_ptr->controlWidget,
            [this](qint64 position) { d_ptr->controlWidget->onPositionChanged(position / 1000); });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::audioTracksChanged,
            d_ptr->controlWidget,
            [this](const QStringList &tracks) {
                qDeleteAll(d_ptr->audioTracksGroup->actions());
                if (tracks.size() < 2) {
                    return;
                }
                for (const auto &item : qAsConst(tracks)) {
                    auto action = new QAction(item, this);
                    action->setCheckable(true);
                    d_ptr->audioTracksMenu->addAction(action);
                    d_ptr->audioTracksGroup->addAction(action);
                }
            });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::audioTrackChanged,
            d_ptr->controlWidget,
            [this](const QString &track) {
                auto actions = d_ptr->audioTracksGroup->actions();
                for (const auto action : qAsConst(actions)) {
                    if (action->text() != track) {
                        continue;
                    }
                    action->setChecked(true);
                    break;
                }
            });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::subTracksChanged,
            d_ptr->controlWidget,
            [this](const QStringList &tracks) {
                qDeleteAll(d_ptr->subTracksGroup->actions());
                if (tracks.size() < 2) {
                    return;
                }
                for (const auto &item : qAsConst(tracks)) {
                    auto action = new QAction(item, this);
                    action->setCheckable(true);
                    d_ptr->subTracksMenu->addAction(action);
                    d_ptr->subTracksGroup->addAction(action);
                }
            });
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::subTrackChanged,
            d_ptr->controlWidget,
            [this](const QString &track) {
                auto actions = d_ptr->subTracksGroup->actions();
                for (const auto action : qAsConst(actions)) {
                    if (action->text() != track) {
                        continue;
                    }
                    action->setChecked(true);
                    break;
                }
            });
    connect(d_ptr->playerPtr.data(), &Ffmpeg::Player::playStarted, this, &MainWindow::onStarted);
    connect(d_ptr->playerPtr.data(), &Ffmpeg::Player::finished, this, &MainWindow::onFinished);
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::stateChanged,
            d_ptr->controlWidget,
            [this](Ffmpeg::Player::MediaState state) {
                switch (state) {
                case Ffmpeg::Player::MediaState::StoppedState:
                case Ffmpeg::Player::MediaState::PausedState:
                    d_ptr->controlWidget->setPlayButtonChecked(false);
                    break;
                case Ffmpeg::Player::MediaState::PlayingState:
                    d_ptr->controlWidget->setPlayButtonChecked(true);
                    break;
                default: break;
                }
            });

    connect(d_ptr->controlWidget, &ControlWidget::hoverPosition, this, &MainWindow::onHoverSlider);
    connect(d_ptr->controlWidget, &ControlWidget::leavePosition, this, &MainWindow::onLeaveSlider);
    connect(d_ptr->controlWidget, &ControlWidget::seek, d_ptr->playerPtr.data(), [this](int value) {
        d_ptr->playerPtr->onSeek(value);
        d_ptr->titleWidget->setText(
            tr("Fast forward to: %1")
                .arg(QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss")));
        d_ptr->titleWidget->setAutoHide(3000);
        d_ptr->setTitleWidgetGeometry(true);
    });
    connect(d_ptr->controlWidget, &ControlWidget::play, this, [this](bool checked) {
        if (checked && !d_ptr->playerPtr->isRunning())
            d_ptr->playerPtr->onPlay();
        else {
            d_ptr->playerPtr->pause(!checked);
        }
    });
    connect(d_ptr->controlWidget,
            &ControlWidget::useGpu,
            d_ptr->playerPtr.data(),
            [this](bool useGpu) { d_ptr->playerPtr->setUseGpuDecode(useGpu); });
    d_ptr->controlWidget->setUseGpu(true);
    connect(d_ptr->controlWidget,
            &ControlWidget::volumeChanged,
            d_ptr->playerPtr.data(),
            [this](int value) {
                d_ptr->playerPtr->setVolume(value / 100.0);
                d_ptr->titleWidget->setText(tr("Volume: %1").arg(value));
                d_ptr->titleWidget->setAutoHide(3000);
                d_ptr->setTitleWidgetGeometry(true);
            });
    d_ptr->controlWidget->setVolume(50);
    connect(d_ptr->controlWidget,
            &ControlWidget::speedChanged,
            d_ptr->playerPtr.data(),
            [this](double value) {
                d_ptr->playerPtr->setSpeed(value);
                d_ptr->titleWidget->setText(tr("Speed: %1").arg(value));
                d_ptr->titleWidget->setAutoHide(3000);
                d_ptr->setTitleWidgetGeometry(true);
            });
    connect(d_ptr->controlWidget, &ControlWidget::showList, d_ptr->playlistView, [this] {
        d_ptr->playlistView->setVisible(!d_ptr->playlistView->isVisible());
    });
    connect(d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::currentIndexChanged,
            this,
            &MainWindow::playlistPositionChanged);
    connect(d_ptr->playlistView, &QAbstractItemView::activated, this, &MainWindow::jump);

    connect(d_ptr->fpsTimer, &QTimer::timeout, this, &MainWindow::onShowCurrentFPS);
}

void MainWindow::initMenu()
{
    d_ptr->menu->addAction(tr("Open Local Media"), this, &MainWindow::onOpenLocalMedia);
    d_ptr->menu->addAction(tr("Open Web Media"), this, &MainWindow::onOpenWebMedia);

    auto widgetAction = new QAction(tr("Widget"), this);
    widgetAction->setCheckable(true);
    widgetAction->setData(Ffmpeg::VideoRenderCreate::Widget);
    auto openglAction = new QAction(tr("Opengl"), this);
    openglAction->setCheckable(true);
    openglAction->setData(Ffmpeg::VideoRenderCreate::Opengl);
    openglAction->setChecked(true);
    auto actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);
    actionGroup->addAction(widgetAction);
    actionGroup->addAction(openglAction);
    connect(actionGroup,
            &QActionGroup::triggered,
            this,
            &MainWindow::onRenderChanged,
            Qt::QueuedConnection);
    openglAction->trigger();

    auto renderMenu = new QMenu(tr("Video Render"), this);
    renderMenu->addAction(widgetAction);
    renderMenu->addAction(openglAction);
    d_ptr->menu->addMenu(renderMenu);

    d_ptr->menu->addMenu(d_ptr->audioTracksMenu);
    d_ptr->menu->addMenu(d_ptr->subTracksMenu);

    connect(d_ptr->audioTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->playerPtr->setAudioTrack(action->text());
    });
    connect(d_ptr->subTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->playerPtr->setSubtitleTrack(action->text());
    });
}

void MainWindow::initPlayListMenu()
{
    d_ptr->playListMenu->addAction(tr("Open Local Media"), this, &MainWindow::onOpenLocalMedia);
    d_ptr->playListMenu->addAction(tr("Open Web Media"), this, &MainWindow::onOpenWebMedia);
    d_ptr->playListMenu
        ->addAction(tr("Remove the currently selected item"), d_ptr->playlistView, [this] {
            auto indexs = d_ptr->playlistView->selectedAllIndexs();
            std::sort(indexs.begin(), indexs.end(), [&](QModelIndex left, QModelIndex right) {
                return left.row() > right.row();
            });
            for (const auto &index : qAsConst(indexs)) {
                d_ptr->playlistModel->playlist()->removeMedia(index.row());
            }
        });
    d_ptr->playListMenu->addAction(tr("Clear"), d_ptr->playlistView, [this] {
        d_ptr->playlistModel->playlist()->clear();
    });
}

void MainWindow::addToPlaylist(const QList<QUrl> &urls)
{
    auto playlist = d_ptr->playlistModel->playlist();
    const int previousMediaCount = playlist->mediaCount();
    for (auto &url : urls) {
        if (isPlaylist(url)) {
            playlist->load(url);
        } else {
            playlist->addMedia(url);
        }
    }
    if (playlist->mediaCount() > previousMediaCount) {
        auto index = d_ptr->playlistModel->index(previousMediaCount, 0);
        d_ptr->playlistView->setCurrentIndex(index);
        jump(index);
    }
}
