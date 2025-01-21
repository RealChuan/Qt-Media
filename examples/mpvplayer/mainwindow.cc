#include "mainwindow.hpp"
#include "mpvlogwindow.hpp"
#include "subtitledelaydialog.hpp"

#include <examples/common/commonstr.hpp>
#include <examples/common/controlwidget.hpp>
#include <examples/common/equalizerdialog.h>
#include <examples/common/openwebmediadialog.hpp>
#include <examples/common/playlistmodel.h>
#include <examples/common/playlistview.hpp>
#include <examples/common/qmediaplaylist.h>
#include <examples/common/titlewidget.hpp>
#include <mpv/mpvopenglwidget.hpp>
#include <mpv/mpvplayer.hpp>
#include <mpv/mpvwidget.hpp>
#include <mpv/previewwidget.hpp>
#include <utils/utils.h>

#include <QtWidgets>

class MainWindow::MainWindowPrivate
{
public:
    explicit MainWindowPrivate(MainWindow *q)
        : q_ptr(q)
    {
        mpvPlayer = new Mpv::MpvPlayer(q_ptr);
#ifdef Q_OS_WIN
        mpvWidget = new Mpv::MpvWidget(q_ptr);
        mpvPlayer->initMpv(mpvWidget);
#else
        mpvWidget = new Mpv::MpvOpenglWidget(mpvPlayer, q_ptr);
        mpvPlayer->initMpv(nullptr);
#endif
        mpvWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mpvWidget->setAcceptDrops(true);
        mpvPlayer->setPrintToStd(true);

        auto logFilePath = QString("%1/mpv_%2.log")
                               .arg(Utils::logPath(),
                                    QDateTime::currentDateTime().toString("yyyyMMdd"));
        mpvPlayer->setLogFile(logFilePath);
        mpvPlayer->setConfigDir(Utils::configPath());

        logWindow = new Mpv::MpvLogWindow(q_ptr);
        logWindow->setMinimumSize(500, 325);
        logWindow->show();
        logWindow->move(qApp->primaryScreen()->availableGeometry().topLeft());

        floatingWidget = new QWidget(q_ptr);
        floatingWidget->setWindowFlags(floatingWidget->windowFlags() | Qt::FramelessWindowHint
                                       | Qt::Tool);
        floatingWidget->setAttribute(Qt::WA_TranslucentBackground); //设置窗口背景透明
        floatingWidget->setVisible(true);
        controlWidget = new ControlWidget(q_ptr);
        controlWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        titleWidget = new TitleWidget(q_ptr);
        titleWidget->setMinimumHeight(80);

        playlistModel = new PlaylistModel(q_ptr);
        playlistView = new PlayListView(q_ptr);
        playlistView->setModel(playlistModel);
        playlistView->setCurrentIndex(
            playlistModel->index(playlistModel->playlist()->currentIndex(), 0));
        //playlistView->setMaximumWidth(250);
        playListMenu = new QMenu(q_ptr);

        menu = new QMenu(q_ptr);
        videoMenu = new QMenu(Common::Tr::tr("Video"), q_ptr);
        audioMenu = new QMenu(Common::Tr::tr("Audio"), q_ptr);
        subMenu = new QMenu(Common::Tr::tr("Subtitles"), q_ptr);
        subDelayAction = new QAction(Common::Tr::tr("Delay"), q_ptr);

        loadSubTitlesAction = new QAction(QCoreApplication::translate("MainWindowPrivate",
                                                                      "Load Subtitles"),
                                          q_ptr);
        videoTracksGroup = new QActionGroup(q_ptr);
        videoTracksGroup->setExclusive(true);
        audioTracksGroup = new QActionGroup(q_ptr);
        audioTracksGroup->setExclusive(true);
        subTracksGroup = new QActionGroup(q_ptr);
        subTracksGroup->setExclusive(true);

        initShortcut();
    }

    ~MainWindowPrivate() = default;

    auto createHWMenu() -> QMenu *
    {
        hwdecGroup = new QActionGroup(q_ptr);
        hwdecGroup->setExclusive(true);
        auto *menu = new QMenu(Common::Tr::tr("H/W"), q_ptr);
        auto hwdecs = mpvPlayer->hwdecs();
        for (const auto &hwdec : std::as_const(hwdecs)) {
            auto *action = new QAction(hwdec, q_ptr);
            action->setCheckable(true);
            hwdecGroup->addAction(action);
            menu->addAction(action);
        }
        hwdecGroup->actions().at(1)->setChecked(true);
        q_ptr->connect(hwdecGroup, &QActionGroup::triggered, q_ptr, [this](QAction *action) {
            mpvPlayer->setHwdec(action->text());
        });

        return menu;
    }

    auto createGpuApiMenu() -> QMenu *
    {
        gpuApiGroup = new QActionGroup(q_ptr);
        gpuApiGroup->setExclusive(true);
        auto *menu = new QMenu(Common::Tr::tr("Gpu Api"), q_ptr);
        auto gpuApis = mpvPlayer->gpuApis();
        for (const auto &gpuApi : std::as_const(gpuApis)) {
            auto *action = new QAction(gpuApi, q_ptr);
            action->setCheckable(true);
            gpuApiGroup->addAction(action);
            menu->addAction(action);
        }
        gpuApiGroup->actions().at(0)->setChecked(true);
        q_ptr->connect(gpuApiGroup, &QActionGroup::triggered, q_ptr, [this](QAction *action) {
            mpvPlayer->setGpuApi(action->text());
        });

        return menu;
    }

    auto createToneMappingMenu() -> QMenu *
    {
        auto *group = new QActionGroup(q_ptr);
        group->setExclusive(true);
        auto *menu = new QMenu(Common::Tr::tr("Tone-Mapping"), q_ptr);
        auto toneMappings = mpvPlayer->toneMappings();
        for (const auto &toneMapping : std::as_const(toneMappings)) {
            auto *action = new QAction(toneMapping, q_ptr);
            action->setCheckable(true);
            group->addAction(action);
            menu->addAction(action);
        }
        group->actions().at(0)->setChecked(true);
        q_ptr->connect(group, &QActionGroup::triggered, q_ptr, [this](QAction *action) {
            mpvPlayer->setToneMapping(action->text());
        });

        return menu;
    }

    auto createTargetPrimariesMenu() -> QMenu *
    {
        auto *group = new QActionGroup(q_ptr);
        group->setExclusive(true);
        auto *menu = new QMenu(Common::Tr::tr("Target Primaries"), q_ptr);
        auto targetPrimaries = mpvPlayer->targetPrimaries();
        for (const auto &targetPrimary : std::as_const(targetPrimaries)) {
            auto *action = new QAction(targetPrimary, q_ptr);
            action->setCheckable(true);
            group->addAction(action);
            menu->addAction(action);
        }
        group->actions().at(0)->setChecked(true);
        q_ptr->connect(group, &QActionGroup::triggered, q_ptr, [this](QAction *action) {
            mpvPlayer->setTargetPrimaries(action->text());
        });

        return menu;
    }

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
        subTracksMenuPtr->addAction(loadSubTitlesAction);
        subTracksMenuPtr->addSeparator();

        videoMenu->addMenu(videoTracksMenuPtr.data());
        audioMenu->addMenu(audioTracksMenuPtr.data());
        subMenu->addMenu(subTracksMenuPtr.data());
        subMenu->addAction(subDelayAction);
    }

    void initShortcut()
    {
        new QShortcut(QKeySequence::MoveToNextChar, q_ptr, q_ptr, [this] {
            mpvPlayer->seekRelative(5);
            setTitleWidgetText(Common::Tr::tr("Fast forward: 5 seconds"));
        });
        new QShortcut(QKeySequence::MoveToPreviousChar, q_ptr, q_ptr, [this] {
            mpvPlayer->seekRelative(-5);
            setTitleWidgetText(Common::Tr::tr("Fast return: 5 seconds"));
        });
        new QShortcut(QKeySequence::MoveToPreviousLine, q_ptr, q_ptr, [this] {
            controlWidget->setVolume(controlWidget->volume() + 10);
        });
        new QShortcut(QKeySequence::MoveToNextLine, q_ptr, q_ptr, [this] {
            controlWidget->setVolume(controlWidget->volume() - 10);
        });
        new QShortcut(Qt::Key_Space, q_ptr, q_ptr, [this] { pause(); });
    }

    void setupUI()
    {
        auto *controlLayout = new QHBoxLayout;
        controlLayout->addStretch();
        controlLayout->addWidget(controlWidget);
        controlLayout->addStretch();
        auto *layout = new QVBoxLayout(floatingWidget);
        layout->addWidget(titleWidget);
        layout->addStretch();
        layout->addLayout(controlLayout);

        auto *splitter = new QSplitter(q_ptr);
        splitter->setHandleWidth(0);
        splitter->addWidget(mpvWidget);
        splitter->addWidget(playlistView);
        splitter->setSizes({200, 1});

        q_ptr->setCentralWidget(splitter);
    }

    void setTitleWidgetText(const QString &text)
    {
        setTitleWidgetGeometry(true);
        titleWidget->setText(text);
        titleWidget->setAutoHide(3000);
    }

    void setControlWidgetGeometry(bool show = true)
    {
        controlWidget->setVisible(true);
        setFloatingWidgetGeometry(show);
        controlWidget->setVisible(show);
    }

    void setTitleWidgetGeometry(bool show = true)
    {
        titleWidget->setVisible(true);
        setFloatingWidgetGeometry(show);
        titleWidget->setVisible(show);
    }

    void setFloatingWidgetGeometry(bool show = true)
    {
        auto geometry = mpvWidget->geometry();
        geometry = QRect{q_ptr->mapToGlobal(geometry.topLeft()), geometry.size()};
        floatingWidget->setGeometry(geometry);
    }

    void pause() const { mpvPlayer->pauseAsync(); }

    MainWindow *q_ptr;

    Mpv::MpvPlayer *mpvPlayer;
    QWidget *mpvWidget;
    QScopedPointer<Mpv::PreviewWidget> previewWidgetPtr;
    Mpv::MpvLogWindow *logWindow;

    QWidget *floatingWidget;
    ControlWidget *controlWidget;
    TitleWidget *titleWidget;

    PlayListView *playlistView;
    PlaylistModel *playlistModel;
    QMenu *playListMenu;

    QMenu *menu;
    QActionGroup *hwdecGroup;
    QActionGroup *gpuApiGroup;

    QMenu *videoMenu;
    QPointer<QMenu> videoTracksMenuPtr;
    QActionGroup *videoTracksGroup;

    QMenu *audioMenu;
    QPointer<QMenu> audioTracksMenuPtr;
    QActionGroup *audioTracksGroup;

    QMenu *subMenu;
    QAction *subDelayAction;
    QPointer<QMenu> subTracksMenuPtr;
    QActionGroup *subTracksGroup;
    QAction *loadSubTitlesAction;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    d_ptr->setupUI();
    buildConnect();
    initMenu();
    initPlayListMenu();

    setAttribute(Qt::WA_Hover);
    d_ptr->mpvWidget->installEventFilter(this);
    d_ptr->playlistView->installEventFilter(this);
    installEventFilter(this);

    resize(1000, 618);
}

MainWindow::~MainWindow()
{
    delete d_ptr->mpvWidget;
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

void MainWindow::onLoadSubtitleFiles()
{
    auto path = d_ptr->mpvPlayer->filepath();
    if (path.isEmpty()) {
        return;
    }
    if (QFile::exists(path)) {
        path = QFileInfo(path).absolutePath();
    } else {
        path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                   .value(0, QDir::homePath());
    }
    const auto filePaths = QFileDialog::getOpenFileNames(this,
                                                         tr("Open File"),
                                                         path,
                                                         tr("Subtitle(*)"));
    if (filePaths.isEmpty()) {
        return;
    }
    d_ptr->mpvPlayer->addSub(filePaths);
}

void MainWindow::onShowSubtitleDelayDialog()
{
    SubtitleDelayDialog dialog(this);
    dialog.setDelay(d_ptr->mpvPlayer->subtitleDelay());
    connect(&dialog, &SubtitleDelayDialog::delayChanged, d_ptr->mpvPlayer, [&] {
        d_ptr->mpvPlayer->setSubtitleDelay(dialog.delay());
    });
    dialog.exec();
}

void MainWindow::onFileLoaded()
{
    setWindowTitle(d_ptr->mpvPlayer->filename());
    d_ptr->controlWidget->setDuration(d_ptr->mpvPlayer->duration());
}

void MainWindow::onTrackChanged()
{
    d_ptr->resetTrackMenu();

    Mpv::TraskInfo noTrackInfo;
    noTrackInfo.id = "no";
    noTrackInfo.title = "no";
    noTrackInfo.selected = true;

    auto videoTracks = d_ptr->mpvPlayer->videoTracks();
    if (!videoTracks.isEmpty()) {
        videoTracks.insert(0, noTrackInfo);
    }
    for (const auto &item : std::as_const(videoTracks)) {
        auto *action = new QAction(item.text(), this);
        action->setData(QVariant::fromValue(item));
        action->setCheckable(true);
        d_ptr->videoTracksMenuPtr->addAction(action);
        d_ptr->videoTracksGroup->addAction(action);
        if (item.selected) {
            action->setChecked(true);
        }
    }

    auto audioTracks = d_ptr->mpvPlayer->audioTracks();
    if (!audioTracks.isEmpty()) {
        audioTracks.insert(0, noTrackInfo);
    }
    for (const auto &item : std::as_const(audioTracks)) {
        auto *action = new QAction(item.text(), this);
        action->setData(QVariant::fromValue(item));
        action->setCheckable(true);
        d_ptr->audioTracksMenuPtr->addAction(action);
        d_ptr->audioTracksGroup->addAction(action);
        if (item.selected) {
            action->setChecked(true);
        }
    }

    auto subTracks = d_ptr->mpvPlayer->subTracks();
    if (!subTracks.isEmpty()) {
        subTracks.insert(0, noTrackInfo);
    }
    for (const auto &item : std::as_const(subTracks)) {
        auto *action = new QAction(item.text(), this);
        action->setData(QVariant::fromValue(item));
        action->setCheckable(true);
        d_ptr->subTracksMenuPtr->addAction(action);
        d_ptr->subTracksGroup->addAction(action);
        if (item.selected) {
            action->setChecked(true);
        }
    }
}

void MainWindow::onChapterChanged()
{
    d_ptr->controlWidget->setChapters(d_ptr->mpvPlayer->chapters());
}

void MainWindow::onRenderChanged(QAction *action)
{
    d_ptr->mpvPlayer->initMpv(d_ptr->mpvWidget);
    d_ptr->mpvPlayer->setPrintToStd(true);
    d_ptr->mpvPlayer->setHwdec(d_ptr->hwdecGroup->checkedAction()->text());
    d_ptr->mpvPlayer->setGpuApi(d_ptr->gpuApiGroup->checkedAction()->text());
    auto index = d_ptr->playlistModel->playlist()->currentIndex();
    if (index > -1) {
        playlistPositionChanged(d_ptr->playlistModel->playlist()->currentIndex());
    }
}

void MainWindow::onEqualizer()
{
    MediaConfig::Equalizer equalizer;
    equalizer.contrastRange().setValue(d_ptr->mpvPlayer->contrast());
    equalizer.brightnessRange().setValue(d_ptr->mpvPlayer->brightness());
    equalizer.saturationRange().setValue(d_ptr->mpvPlayer->saturation());
    equalizer.gammaRange().setValue(d_ptr->mpvPlayer->gamma());
    equalizer.hueRange().setValue(d_ptr->mpvPlayer->hue());

    EqualizerDialog dialog(this);
    dialog.setEqualizer(equalizer);
    connect(&dialog, &EqualizerDialog::equalizerChanged, this, [&] {
        auto equalizer = dialog.equalizer();
        d_ptr->mpvPlayer->setContrast(equalizer.contrastRange().value());
        d_ptr->mpvPlayer->setBrightness(equalizer.brightnessRange().value());
        d_ptr->mpvPlayer->setSaturation(equalizer.saturationRange().value());
        d_ptr->mpvPlayer->setGamma(equalizer.gammaRange().value());
        d_ptr->mpvPlayer->setHue(equalizer.hueRange().value());
    });
    dialog.exec();
}

void MainWindow::onPreview(int pos, int value)
{
    auto url = d_ptr->mpvPlayer->filepath();
    if (url.isEmpty()) {
        return;
    }
    if (d_ptr->previewWidgetPtr.isNull()) {
        d_ptr->previewWidgetPtr.reset(new Mpv::PreviewWidget);
        d_ptr->previewWidgetPtr->setWindowFlags(d_ptr->previewWidgetPtr->windowFlags() | Qt::Tool
                                                | Qt::FramelessWindowHint
                                                | Qt::WindowStaysOnTopHint);
    }
    d_ptr->previewWidgetPtr->startPreview(url, value);
    int w = 320;
    int h = 200;
    d_ptr->previewWidgetPtr->setFixedSize(w, h);
    auto gpos = d_ptr->controlWidget->sliderGlobalPos() + QPoint(pos, 0);
    d_ptr->previewWidgetPtr->move(gpos - QPoint(w / 2, h + 15));
    d_ptr->previewWidgetPtr->show();
}

void MainWindow::onPreviewFinish()
{
    if (!d_ptr->previewWidgetPtr.isNull()) {
        d_ptr->previewWidgetPtr->hide();
        d_ptr->previewWidgetPtr->clearAllTask();
    }
}

void MainWindow::playlistPositionChanged(int currentItem)
{
    if (currentItem < 0) {
        return;
    }
    d_ptr->playlistView->setCurrentIndex(d_ptr->playlistModel->index(currentItem, 0));
    auto url = d_ptr->playlistModel->playlist()->currentMedia();
    d_ptr->mpvPlayer->openMedia(url.isLocalFile() ? url.toLocalFile() : url.toString());
    if (d_ptr->mpvPlayer->isPaused()) {
        d_ptr->mpvPlayer->pauseAsync();
    } else {
        d_ptr->controlWidget->setPlayButtonChecked(true);
    }
}

auto MainWindow::eventFilter(QObject *watched, QEvent *event) -> bool
{
    if (watched == d_ptr->mpvWidget) {
        switch (event->type()) {
        case QEvent::DragEnter: {
            auto *e = static_cast<QDragEnterEvent *>(event);
            e->acceptProposedAction();
        } break;
        case QEvent::DragMove: {
            auto *e = static_cast<QDragMoveEvent *>(event);
            e->acceptProposedAction();
        } break;
        case QEvent::Drop: {
            auto *e = static_cast<QDropEvent *>(event);
            QList<QUrl> urls = e->mimeData()->urls();
            if (!urls.isEmpty()) {
                addToPlaylist(urls);
            }
        } break;
        case QEvent::ContextMenu: {
            auto *e = static_cast<QContextMenuEvent *>(event);
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
            auto *e = static_cast<QContextMenuEvent *>(event);
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
        case QEvent::Hide: d_ptr->setFloatingWidgetGeometry(false); break;
        case QEvent::HoverMove: {
            auto controlWidgetGeometry = d_ptr->controlWidget->geometry();
            auto e = static_cast<QHoverEvent *>(event);
            bool contain = controlWidgetGeometry.contains(e->position().toPoint());
            d_ptr->setControlWidgetGeometry(contain);
            if (isFullScreen()) {
                auto titleWidgetGeometry = d_ptr->titleWidget->geometry();
                contain = titleWidgetGeometry.contains(e->position().toPoint());
                d_ptr->setTitleWidgetGeometry(contain);
            }
            break;
        }
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

void MainWindow::buildConnect()
{
    connect(d_ptr->mpvPlayer, &Mpv::MpvPlayer::fileLoaded, this, &MainWindow::onFileLoaded);
    connect(d_ptr->mpvPlayer,
            &Mpv::MpvPlayer::fileFinished,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::next);
    connect(d_ptr->mpvPlayer, &Mpv::MpvPlayer::chapterChanged, this, &MainWindow::onChapterChanged);
    connect(d_ptr->mpvPlayer, &Mpv::MpvPlayer::trackChanged, this, &MainWindow::onTrackChanged);
    connect(d_ptr->mpvPlayer,
            &Mpv::MpvPlayer::positionChanged,
            d_ptr->controlWidget,
            &ControlWidget::setPosition);
    connect(d_ptr->mpvPlayer,
            &Mpv::MpvPlayer::mpvLogMessage,
            d_ptr->logWindow,
            &Mpv::MpvLogWindow::onAppendLog);
    connect(d_ptr->mpvPlayer, &Mpv::MpvPlayer::pauseStateChanged, this, [this](bool pause) {
        d_ptr->controlWidget->setPlayButtonChecked(!pause);
    });
    connect(d_ptr->mpvPlayer,
            &Mpv::MpvPlayer::cacheSpeedChanged,
            d_ptr->controlWidget,
            &ControlWidget::setCacheSpeed);

    connect(d_ptr->controlWidget,
            &ControlWidget::previous,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::previous);
    connect(d_ptr->controlWidget,
            &ControlWidget::next,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::next);
    connect(d_ptr->controlWidget, &ControlWidget::seek, d_ptr->mpvPlayer, [this](int value) {
        d_ptr->mpvPlayer->seek(value);
        d_ptr->setTitleWidgetText(
            tr("Fast forward to: %1")
                .arg(QTime::fromMSecsSinceStartOfDay(value * 1000).toString("hh:mm:ss")));
    });
    connect(d_ptr->controlWidget, &ControlWidget::hoverPosition, this, &MainWindow::onPreview);
    connect(d_ptr->controlWidget, &ControlWidget::leavePosition, this, &MainWindow::onPreviewFinish);
    connect(d_ptr->controlWidget, &ControlWidget::volumeChanged, d_ptr->mpvPlayer, [this](int value) {
        d_ptr->mpvPlayer->setVolume(value);
        d_ptr->setTitleWidgetText(tr("Volume: %1%").arg(value));
    });
    auto max = d_ptr->mpvPlayer->volumeMax();
    d_ptr->controlWidget->setVolumeMax(max);
    d_ptr->controlWidget->setVolume(max / 2);
    connect(d_ptr->controlWidget,
            &ControlWidget::speedChanged,
            d_ptr->mpvPlayer,
            [this](double value) {
                d_ptr->mpvPlayer->setSpeed(value);
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
}

void MainWindow::initMenu()
{
    d_ptr->menu->addAction(tr("Open Local Media"), this, &MainWindow::onOpenLocalMedia);
    d_ptr->menu->addAction(tr("Open Web Media"), this, &MainWindow::onOpenWebMedia);

    d_ptr->menu->addMenu(d_ptr->createHWMenu());
    d_ptr->menu->addMenu(d_ptr->createGpuApiMenu());
    d_ptr->menu->addMenu(d_ptr->videoMenu);
    d_ptr->menu->addMenu(d_ptr->audioMenu);
    d_ptr->menu->addMenu(d_ptr->subMenu);

    auto *equalizerAction = new QAction(tr("Equalizer"), this);
    connect(equalizerAction, &QAction::triggered, this, &MainWindow::onEqualizer);
    d_ptr->videoMenu->addAction(equalizerAction);
    d_ptr->videoMenu->addMenu(d_ptr->createToneMappingMenu());
    d_ptr->videoMenu->addMenu(d_ptr->createTargetPrimariesMenu());

    connect(d_ptr->videoTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        auto data = action->data().value<Mpv::TraskInfo>();
        d_ptr->mpvPlayer->setVid(data.id);
    });
    connect(d_ptr->audioTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        auto data = action->data().value<Mpv::TraskInfo>();
        d_ptr->mpvPlayer->setAid(data.id);
    });
    connect(d_ptr->subTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        auto data = action->data().value<Mpv::TraskInfo>();
        d_ptr->mpvPlayer->setSid(data.id);
    });
    connect(d_ptr->loadSubTitlesAction, &QAction::triggered, this, &MainWindow::onLoadSubtitleFiles);
    connect(d_ptr->subDelayAction,
            &QAction::triggered,
            this,
            &MainWindow::onShowSubtitleDelayDialog);
#ifdef Q_OS_MACOS
    d_ptr->menu->setTitle(tr("Menu"));
    menuBar()->addMenu(d_ptr->menu);
#endif
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
    for (const auto &url : std::as_const(urls)) {
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

void MainWindow::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        d_ptr->playlistModel->playlist()->setCurrentIndex(index.row());
    }
}
