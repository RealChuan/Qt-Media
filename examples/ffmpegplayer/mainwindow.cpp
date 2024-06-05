#include "mainwindow.h"
#include "colorspacedialog.hpp"

#include <examples/common/controlwidget.hpp>
#include <examples/common/openwebmediadialog.hpp>
#include <examples/common/playlistmodel.h>
#include <examples/common/playlistview.hpp>
#include <examples/common/qmediaplaylist.h>
#include <examples/common/titlewidget.hpp>
#include <ffmpeg/averror.h>
#include <ffmpeg/event/errorevent.hpp>
#include <ffmpeg/event/seekevent.hpp>
#include <ffmpeg/event/trackevent.hpp>
#include <ffmpeg/event/valueevent.hpp>
#include <ffmpeg/ffmpegutils.hpp>
#include <ffmpeg/player.h>
#include <ffmpeg/videorender/videopreviewwidget.hpp>
#include <ffmpeg/videorender/videorendercreate.hpp>
#include <ffmpeg/widgets/mediainfodialog.hpp>

#include <QtWidgets>

static auto isPlaylist(const QUrl &url) -> bool // Check for ".m3u" playlists.
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists()
           && (fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive) == 0);
}

class MainWindow::MainWindowPrivate
{
public:
    explicit MainWindowPrivate(MainWindow *q)
        : q_ptr(q)
        , playerPtr(new Ffmpeg::Player)
    {
        controlWidget = new ControlWidget(q_ptr);
        controlWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        titleWidget = new TitleWidget(q_ptr);
        titleWidget->setMinimumHeight(80);
        titleWidget->setVisible(false);
        auto *bottomLayout = new QHBoxLayout;
        bottomLayout->addStretch();
        bottomLayout->addWidget(controlWidget);
        bottomLayout->addStretch();
        controlLayout = new QVBoxLayout;
        controlLayout->addWidget(titleWidget);
        controlLayout->addStretch();
        controlLayout->addLayout(bottomLayout);

        playlistModel = new PlaylistModel(q_ptr);
        playlistView = new PlayListView(q_ptr);
        playlistView->setModel(playlistModel);
        playlistView->setCurrentIndex(
            playlistModel->index(playlistModel->playlist()->currentIndex(), 0));
        //playlistView->setMaximumWidth(250);

        menu = new QMenu(q_ptr);
        audioTracksGroup = new QActionGroup(q_ptr);
        audioTracksGroup->setExclusive(true);
        videoTracksGroup = new QActionGroup(q_ptr);
        videoTracksGroup->setExclusive(true);
        subTracksGroup = new QActionGroup(q_ptr);
        subTracksGroup->setExclusive(true);
        mediaInfoAction = new QAction(QCoreApplication::translate("MainWindowPrivate", "Media Info"),
                                      q_ptr);

        playListMenu = new QMenu(q_ptr);

        fpsTimer = new QTimer(q_ptr);

        splitter = new QSplitter(q_ptr);
        splitter->addWidget(playlistView);

        initShortcut();
    }

    ~MainWindowPrivate() = default;

    void resetTrackMenu()
    {
        auto actions = audioTracksGroup->actions();
        for (auto *action : actions) {
            audioTracksGroup->removeAction(action);
            delete action;
        }
        actions = videoTracksGroup->actions();
        for (auto *action : actions) {
            videoTracksGroup->removeAction(action);
            delete action;
        }
        actions = subTracksGroup->actions();
        for (auto *action : actions) {
            subTracksGroup->removeAction(action);
            delete action;
        }

        if (!audioTracksMenuPtr.isNull()) {
            delete audioTracksMenuPtr.data();
        }
        if (!videoTracksMenuPtr.isNull()) {
            delete videoTracksMenuPtr.data();
        }
        if (!subTracksMenuPtr.isNull()) {
            delete subTracksMenuPtr.data();
        }
        audioTracksMenuPtr = new QMenu(QCoreApplication::translate("MainWindowPrivate",
                                                                   "Select audio track"),
                                       q_ptr);
        videoTracksMenuPtr = new QMenu(QCoreApplication::translate("MainWindowPrivate",
                                                                   "Select video track"),
                                       q_ptr);
        subTracksMenuPtr = new QMenu(QCoreApplication::translate("MainWindowPrivate",
                                                                 "Select subtitle track"),
                                     q_ptr);
        menu->addMenu(videoTracksMenuPtr.data());
        menu->addMenu(audioTracksMenuPtr.data());
        menu->addMenu(subTracksMenuPtr.data());

        menu->removeAction(mediaInfoAction);
        menu->addAction(mediaInfoAction);
    }

    void initShortcut()
    {
        new QShortcut(QKeySequence::MoveToNextChar, q_ptr, q_ptr, [this] {
            playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::SeekRelativeEvent(5)));
        });
        new QShortcut(QKeySequence::MoveToPreviousChar, q_ptr, q_ptr, [this] {
            playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::SeekRelativeEvent(-5)));
        });
        new QShortcut(QKeySequence::MoveToPreviousLine, q_ptr, q_ptr, [this] {
            controlWidget->setVolume(controlWidget->volume() + 5);
        });
        new QShortcut(QKeySequence::MoveToNextLine, q_ptr, q_ptr, [this] {
            controlWidget->setVolume(controlWidget->volume() - 5);
        });
        new QShortcut(Qt::Key_Space, q_ptr, q_ptr, [this] { controlWidget->clickPlayButton(); });
    }

    void setControlWidgetVisible(bool visible) const
    {
        if (videoRender.isNull()) {
            return;
        }
        controlWidget->setVisible(visible);
    }

    [[nodiscard]] auto setTitleWidgetVisible(bool visible) const -> bool
    {
        if (videoRender.isNull()) {
            return false;
        }
        titleWidget->setVisible(visible);
        return true;
    }

    void setTitleWidgetText(const QString &text) const
    {
        if (!setTitleWidgetVisible(true)) {
            return;
        }
        titleWidget->setText(text);
        titleWidget->setAutoHide(3000);
    }

    void started() const
    {
        controlWidget->setSourceFPS(playerPtr->fps());

        auto size = playerPtr->resolutionRatio();
        q_ptr->setWindowTitle(QString("%1[%2x%3]")
                                  .arg(playlistModel->playlist()->currentMedia().fileName(),
                                       QString::number(size.width()),
                                       QString::number(size.height())));

        fpsTimer->start(1000);
    }

    void finished() const
    {
        fpsTimer->stop();
        controlWidget->setDuration(0);
        controlWidget->setPosition(0);
    }

    MainWindow *q_ptr;

    QScopedPointer<Ffmpeg::Player> playerPtr;
    QScopedPointer<Ffmpeg::VideoRender> videoRender;
    QScopedPointer<Ffmpeg::VideoPreviewWidget> videoPreviewWidgetPtr;

    ControlWidget *controlWidget;
    TitleWidget *titleWidget;
    QVBoxLayout *controlLayout;

    PlayListView *playlistView;
    PlaylistModel *playlistModel;

    QMenu *menu;
    QPointer<QMenu> audioTracksMenuPtr;
    QPointer<QMenu> videoTracksMenuPtr;
    QPointer<QMenu> subTracksMenuPtr;
    QActionGroup *audioTracksGroup;
    QActionGroup *videoTracksGroup;
    QActionGroup *subTracksGroup;
    QAction *mediaInfoAction;

    QMenu *playListMenu;

    QTimer *fpsTimer;

    QSplitter *splitter;

    Ffmpeg::Tonemap::Type tonemapType = Ffmpeg::Tonemap::Type::AUTO;
    Ffmpeg::ColorUtils::Primaries::Type primarisType = Ffmpeg::ColorUtils::Primaries::Type::AUTO;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    Ffmpeg::printFfmpegInfo();

    setupUI();
    buildConnect();
    initMenu();
    initPlayListMenu();

    setAttribute(Qt::WA_Hover);
    d_ptr->playlistView->installEventFilter(this);
    installEventFilter(this);

    resize(1000, 618);
}

MainWindow::~MainWindow()
{
    d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::CloseMediaEvent()));
    d_ptr->playerPtr->setVideoRenders({});
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
    qint64 position = value;
    qint64 duration = d_ptr->controlWidget->duration();
    d_ptr->videoPreviewWidgetPtr->startPreview(filePath,
                                               index,
                                               position * AV_TIME_BASE,
                                               duration * AV_TIME_BASE);

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
        return;
    }
    d_ptr->controlWidget->setCurrentFPS(renders[0]->fps());
}

void MainWindow::onOpenLocalMedia()
{
    const auto path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                          .value(0, QDir::homePath());
    const auto filter = tr("Media (*)");
    const auto urls = QFileDialog::getOpenFileUrls(this,
                                                   tr("Open Media"),
                                                   QUrl::fromUserInput(path),
                                                   filter);
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
    auto renderType = Ffmpeg::VideoRenderCreate::Opengl;
    auto type = action->data().toInt();
    switch (type) {
    case 1: renderType = Ffmpeg::VideoRenderCreate::Widget; break;
    default: renderType = Ffmpeg::VideoRenderCreate::Opengl; break;
    }
    auto *videoRender = Ffmpeg::VideoRenderCreate::create(renderType);
    videoRender->widget()->setLayout(d_ptr->controlLayout);
    videoRender->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoRender->widget()->setAcceptDrops(true);
    videoRender->widget()->installEventFilter(this);
    d_ptr->playerPtr->setVideoRenders({videoRender});
    d_ptr->splitter->insertWidget(0, videoRender->widget());
    // 为什么切换成widget还是有使用GPU 0-3D，而且使用量是切换为opengl的两倍！！！
    d_ptr->videoRender.reset(videoRender);
    d_ptr->videoRender->setTonemapType(d_ptr->tonemapType);
    d_ptr->videoRender->setDestPrimaries(d_ptr->primarisType);
}

void MainWindow::playlistPositionChanged(int currentItem)
{
    if (currentItem < 0) {
        return;
    }
    auto url = d_ptr->playlistModel->playlist()->currentMedia();
    d_ptr->playlistView->setCurrentIndex(d_ptr->playlistModel->index(currentItem, 0));

    d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(
        new Ffmpeg::OpenMediaEvent(url.isLocalFile() ? url.toLocalFile() : url.toString())));
}

void MainWindow::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        d_ptr->playlistModel->playlist()->setCurrentIndex(index.row());
    }
}

void MainWindow::onShowColorSpace()
{
    if (d_ptr->videoRender.isNull()) {
        return;
    }
    ColorSpaceDialog dialog(this);
    dialog.setColorSpace(d_ptr->videoRender->colorSpaceTrc());
    connect(&dialog, &ColorSpaceDialog::colorSpaceChanged, this, [&] {
        d_ptr->videoRender->setColorSpaceTrc(dialog.colorSpace());
    });
    dialog.exec();
}

void MainWindow::onShowMediaInfo()
{
    Ffmpeg::MediaInfoDialog dialog(this);
    dialog.setMediaInfo(d_ptr->playerPtr->mediaInfo());
    dialog.exec();
}

void MainWindow::onProcessEvents()
{
    while (d_ptr->playerPtr->propertyChangeEventSize() > 0) {
        auto eventPtr = d_ptr->playerPtr->takePropertyChangeEvent();
        switch (eventPtr->type()) {
        case Ffmpeg::PropertyChangeEvent::EventType::Duration: {
            auto *durationEvent = dynamic_cast<Ffmpeg::DurationEvent *>(eventPtr.data());
            d_ptr->controlWidget->setDuration(durationEvent->duration() / AV_TIME_BASE);
            d_ptr->controlWidget->setChapters(d_ptr->playerPtr->mediaInfo().chapters);
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::Position: {
            auto *positionEvent = dynamic_cast<Ffmpeg::PositionEvent *>(eventPtr.data());
            d_ptr->controlWidget->setPosition(positionEvent->position() / AV_TIME_BASE);
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::MediaState: {
            auto *stateEvent = dynamic_cast<Ffmpeg::MediaStateEvent *>(eventPtr.data());
            switch (stateEvent->state()) {
            case Ffmpeg::MediaState::Stopped:
                d_ptr->controlWidget->setPlayButtonChecked(false);
                d_ptr->finished();
                break;
            case Ffmpeg::MediaState::Pausing:
                d_ptr->controlWidget->setPlayButtonChecked(false);
                break;
            case Ffmpeg::MediaState::Opening:
                d_ptr->controlWidget->setPlayButtonChecked(true);
                break;
            case Ffmpeg::MediaState::Playing:
                d_ptr->controlWidget->setPlayButtonChecked(true);
                d_ptr->started();
                break;
            default: break;
            }
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::CacheSpeed: {
            auto *speedEvent = dynamic_cast<Ffmpeg::CacheSpeedEvent *>(eventPtr.data());
            d_ptr->controlWidget->setCacheSpeed(speedEvent->speed());
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::MediaTrack: {
            d_ptr->resetTrackMenu();

            auto *tracksEvent = dynamic_cast<Ffmpeg::MediaTrackEvent *>(eventPtr.data());
            auto tracks = tracksEvent->tracks();
            for (const auto &track : std::as_const(tracks)) {
                std::unique_ptr<QAction> actionPtr(new QAction(track.info(), this));
                actionPtr->setProperty("index", track.index);
                actionPtr->setCheckable(true);
                if (track.selected) {
                    actionPtr->setChecked(true);
                }
                switch (track.mediaType) {
                case AVMEDIA_TYPE_AUDIO: {
                    auto *action = actionPtr.release();
                    d_ptr->audioTracksMenuPtr->addAction(action);
                    d_ptr->audioTracksGroup->addAction(action);
                } break;
                case AVMEDIA_TYPE_VIDEO: {
                    auto *action = actionPtr.release();
                    d_ptr->videoTracksMenuPtr->addAction(action);
                    d_ptr->videoTracksGroup->addAction(action);
                } break;
                case AVMEDIA_TYPE_SUBTITLE: {
                    auto *action = actionPtr.release();
                    d_ptr->subTracksMenuPtr->addAction(action);
                    d_ptr->subTracksGroup->addAction(action);
                } break;
                default: break;
                }
            }
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::SeekChanged: {
            auto *seekEvent = dynamic_cast<Ffmpeg::SeekChangedEvent *>(eventPtr.data());
            int value = seekEvent->position() * 100 / d_ptr->playerPtr->duration();
            auto text = tr("Seeked To %1 (key frame) / %2 (%3%)")
                            .arg(QTime::fromMSecsSinceStartOfDay(seekEvent->position() / 1000)
                                     .toString("hh:mm:ss"),
                                 QTime::fromMSecsSinceStartOfDay(d_ptr->playerPtr->duration() / 1000)
                                     .toString("hh:mm:ss"),
                                 QString::number(value));
            d_ptr->setTitleWidgetText(text);
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::AVError: {
            auto *errorEvent = dynamic_cast<Ffmpeg::AVErrorEvent *>(eventPtr.data());
            const auto text = tr("Error[%1]:%2.")
                                  .arg(QString::number(errorEvent->error().errorCode()),
                                       errorEvent->error().errorString());
            qWarning() << text;
            d_ptr->setTitleWidgetText(text);
        } break;
        default: break;
        }
    }
}

auto MainWindow::eventFilter(QObject *watched, QEvent *event) -> bool
{
    if (!d_ptr->videoRender.isNull() && watched == d_ptr->videoRender->widget()) {
        switch (event->type()) {
        case QEvent::DragEnter: {
            auto *ev = dynamic_cast<QDragEnterEvent *>(event);
            ev->acceptProposedAction();
        } break;
        case QEvent::DragMove: {
            auto *ev = dynamic_cast<QDragMoveEvent *>(event);
            ev->acceptProposedAction();
        } break;
        case QEvent::Drop: {
            auto *ev = dynamic_cast<QDropEvent *>(event);
            auto urls = ev->mimeData()->urls();
            if (!urls.isEmpty()) {
                addToPlaylist(urls);
            }
        } break;
        case QEvent::ContextMenu: {
            auto *ev = dynamic_cast<QContextMenuEvent *>(event);
            d_ptr->menu->exec(ev->globalPos());
        } break;
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
            auto *ev = dynamic_cast<QContextMenuEvent *>(event);
            d_ptr->playListMenu->exec(ev->globalPos());
        } break;
        default: break;
        }
    } else if (watched == this) {
        switch (event->type()) {
        case QEvent::HoverMove: {
            d_ptr->controlWidget->show();
            d_ptr->controlWidget->hide();
            auto controlWidgetGeometry = d_ptr->controlWidget->geometry();
            auto *ev = dynamic_cast<QHoverEvent *>(event);
            bool contain = controlWidgetGeometry.contains(ev->position().toPoint());
            d_ptr->setControlWidgetVisible(contain);
            if (isFullScreen()) {
                d_ptr->titleWidget->show();
                d_ptr->titleWidget->hide();
                auto titleWidgetGeometry = d_ptr->titleWidget->geometry();
                contain = titleWidgetGeometry.contains(ev->position().toPoint());
                d_ptr->setTitleWidgetVisible(contain);
            }
            break;
        }
        default: break;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);

    qInfo() << "MainWindow Pressed key:" << event->key();
    switch (event->key()) {
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
    connect(d_ptr->playerPtr.data(),
            &Ffmpeg::Player::eventIncrease,
            this,
            &MainWindow::onProcessEvents);

    connect(d_ptr->controlWidget,
            &ControlWidget::previous,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::previous);
    connect(d_ptr->controlWidget,
            &ControlWidget::next,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::next);
    connect(d_ptr->controlWidget, &ControlWidget::hoverPosition, this, &MainWindow::onHoverSlider);
    connect(d_ptr->controlWidget, &ControlWidget::leavePosition, this, &MainWindow::onLeaveSlider);
    connect(d_ptr->controlWidget, &ControlWidget::seek, d_ptr->playerPtr.data(), [this](int value) {
        qint64 position = value;
        d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::SeekEvent(position * AV_TIME_BASE)));
    });
    connect(d_ptr->controlWidget, &ControlWidget::play, this, [this](bool checked) {
        if (checked && !d_ptr->playerPtr->isRunning()) {
            d_ptr->playerPtr->onPlay();
        } else {
            d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::PauseEvent(!checked)));
        }
    });
    connect(d_ptr->controlWidget,
            &ControlWidget::volumeChanged,
            d_ptr->playerPtr.data(),
            [this](int value) {
                d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::VolumeEvent(value / 100.0)));
                d_ptr->setTitleWidgetText(tr("Volume: %1").arg(value));
            });
    d_ptr->controlWidget->setVolume(50);
    connect(d_ptr->controlWidget,
            &ControlWidget::speedChanged,
            d_ptr->playerPtr.data(),
            [this](double value) {
                d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::SpeedEvent(value)));
                d_ptr->setTitleWidgetText(tr("Speed: %1").arg(value));
            });
    connect(d_ptr->controlWidget,
            &ControlWidget::modelChanged,
            d_ptr->playlistModel->playlist(),
            [this](int model) {
                d_ptr->playlistModel->playlist()->setPlaybackMode(
                    static_cast<QMediaPlaylist::PlaybackMode>(model));
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

    auto *hwAction = new QAction("H/W", this);
    hwAction->setCheckable(true);
    connect(hwAction, &QAction::toggled, this, [this](bool checked) {
        d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(new Ffmpeg::GpuEvent(checked)));
    });
    hwAction->setChecked(true);
    d_ptr->menu->addAction(hwAction);

    auto *colorSpaceAction = new QAction(tr("Color Space"), this);
    connect(colorSpaceAction, &QAction::triggered, this, &MainWindow::onShowColorSpace);
    d_ptr->menu->addAction(colorSpaceAction);

    tonemapMenu();
    destPrimarisMenu();
    renderMenu();

    connect(d_ptr->audioTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(
            new Ffmpeg::SelectedMediaTrackEvent(action->property("index").toInt(),
                                                Ffmpeg::Event::EventType::AudioTarck)));
    });
    connect(d_ptr->videoTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(
            new Ffmpeg::SelectedMediaTrackEvent(action->property("index").toInt(),
                                                Ffmpeg::Event::EventType::VideoTrack)));
    });
    connect(d_ptr->subTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->playerPtr->addEvent(Ffmpeg::EventPtr(
            new Ffmpeg::SelectedMediaTrackEvent(action->property("index").toInt(),
                                                Ffmpeg::Event::EventType::SubtitleTrack)));
    });

    connect(d_ptr->mediaInfoAction, &QAction::triggered, this, &MainWindow::onShowMediaInfo);
}

void MainWindow::tonemapMenu()
{
    auto *tonemapGroup = new QActionGroup(this);
    tonemapGroup->setExclusive(true);
    auto *tonemapMenu = new QMenu(tr("Tonemap"), this);
    auto tonemaps = QMetaEnum::fromType<Ffmpeg::Tonemap::Type>();
    for (int i = 0; i < tonemaps.keyCount(); ++i) {
        auto value = tonemaps.value(i);
        auto *action = new QAction(tonemaps.key(i), this);
        action->setCheckable(true);
        action->setData(value);
        tonemapGroup->addAction(action);
        tonemapMenu->addAction(action);
        if (value == Ffmpeg::Tonemap::Type::AUTO) {
            action->setChecked(true);
        }
    }
    d_ptr->menu->addMenu(tonemapMenu);
    connect(tonemapGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->tonemapType = static_cast<Ffmpeg::Tonemap::Type>(action->data().toInt());
        if (d_ptr->videoRender.isNull()) {
            return;
        }
        d_ptr->videoRender->setTonemapType(d_ptr->tonemapType);
    });
    tonemapGroup->checkedAction()->trigger();
}

void MainWindow::destPrimarisMenu()
{
    auto *destPrimarisGroup = new QActionGroup(this);
    destPrimarisGroup->setExclusive(true);
    auto *destPrimarisMenu = new QMenu(tr("Dest Primaris"), this);
    auto destPrimaris = QMetaEnum::fromType<Ffmpeg::ColorUtils::Primaries::Type>();
    for (int i = 0; i < destPrimaris.keyCount(); ++i) {
        auto value = destPrimaris.value(i);
        auto *action = new QAction(destPrimaris.key(i), this);
        action->setCheckable(true);
        action->setData(value);
        destPrimarisGroup->addAction(action);
        destPrimarisMenu->addAction(action);
        if (value == Ffmpeg::ColorUtils::Primaries::Type::AUTO) {
            action->setChecked(true);
        }
    }
    d_ptr->menu->addMenu(destPrimarisMenu);
    connect(destPrimarisGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->primarisType = static_cast<Ffmpeg::ColorUtils::Primaries::Type>(
            action->data().toInt());
        if (d_ptr->videoRender.isNull()) {
            return;
        }
        d_ptr->videoRender->setDestPrimaries(d_ptr->primarisType);
    });
    destPrimarisGroup->checkedAction()->trigger();
}

void MainWindow::renderMenu()
{
    auto *widgetAction = new QAction(tr("Widget"), this);
    widgetAction->setCheckable(true);
    widgetAction->setData(Ffmpeg::VideoRenderCreate::Widget);
    auto *openglAction = new QAction(tr("Opengl"), this);
    openglAction->setCheckable(true);
    openglAction->setData(Ffmpeg::VideoRenderCreate::Opengl);
    openglAction->setChecked(true);
    auto *actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);
    actionGroup->addAction(widgetAction);
    actionGroup->addAction(openglAction);
    connect(actionGroup,
            &QActionGroup::triggered,
            this,
            &MainWindow::onRenderChanged,
            Qt::QueuedConnection);
    openglAction->trigger();

    auto *renderMenu = new QMenu(tr("Video Render"), this);
    renderMenu->addAction(widgetAction);
    renderMenu->addAction(openglAction);
    d_ptr->menu->addMenu(renderMenu);
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
            for (const auto &index : std::as_const(indexs)) {
                d_ptr->playlistModel->playlist()->removeMedia(index.row());
            }
        });
    d_ptr->playListMenu->addAction(tr("Clear"), d_ptr->playlistView, [this] {
        d_ptr->playlistModel->playlist()->clear();
    });
}

void MainWindow::addToPlaylist(const QList<QUrl> &urls)
{
    auto *playlist = d_ptr->playlistModel->playlist();
    const int previousMediaCount = playlist->mediaCount();
    for (const auto &url : urls) {
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
