#include "mainwindow.hpp"
#include "videowidget.hpp"

#include <examples/common/commonstr.hpp>
#include <examples/common/controlwidget.hpp>
#include <examples/common/openwebmediadialog.hpp>
#include <examples/common/playlistmodel.h>
#include <examples/common/playlistview.hpp>
#include <examples/common/qmediaplaylist.h>
#include <examples/common/titlewidget.hpp>

#include <rhi/qrhi.h>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QMediaMetaData>
#include <QVideoSink>
#include <QtWidgets>

void printAudioOuputDevice()
{
    const auto audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &audioDevice : std::as_const(audioDevices)) {
        qDebug() << audioDevice.id() << audioDevice.description() << audioDevice.isDefault();
    }
}
QString trackName(const QMediaMetaData &metaData, int index)
{
    QString name;
    QString title = metaData.stringValue(QMediaMetaData::Title);
    QLocale::Language lang = metaData.value(QMediaMetaData::Language).value<QLocale::Language>();

    if (title.isEmpty()) {
        if (lang == QLocale::Language::AnyLanguage) {
            name = Common::Tr::tr("Track %1").arg(index + 1);
        } else {
            name = QLocale::languageToString(lang);
        }
    } else {
        if (lang == QLocale::Language::AnyLanguage) {
            name = title;
        } else {
            name = QStringLiteral("%1 - [%2]").arg(title).arg(QLocale::languageToString(lang));
        }
    }
    return name;
}

class MainWindow::MainWindowPrivate
{
public:
    explicit MainWindowPrivate(MainWindow *q)
        : q_ptr(q)
    {
        mediaDevices = new QMediaDevices(q_ptr);

        player = new QMediaPlayer(q_ptr);
        audioOutput = new QAudioOutput(q_ptr);
        player->setAudioOutput(audioOutput);
        videoWidget = new VideoWidget(q_ptr);
        player->setVideoOutput(videoWidget);

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
        videoTracksGroup = new QActionGroup(q_ptr);
        videoTracksGroup->setExclusive(true);
        audioTracksGroup = new QActionGroup(q_ptr);
        audioTracksGroup->setExclusive(true);
        subTracksGroup = new QActionGroup(q_ptr);
        subTracksGroup->setExclusive(true);

        initShortcut();
    }

    void initShortcut()
    {
        new QShortcut(QKeySequence::MoveToNextChar, q_ptr, q_ptr, [this] {
            auto position = player->position();
            position += 5000;
            if (position > player->duration()) {
                position = player->duration();
            }
            setPosition(position);
            setTitleWidgetText(Common::Tr::tr("Fast forward: 5 seconds"));
        });
        new QShortcut(QKeySequence::MoveToPreviousChar, q_ptr, q_ptr, [this] {
            auto position = player->position();
            position -= 5000;
            if (position < 0) {
                position = 0;
            }
            setPosition(position);
            setTitleWidgetText(Common::Tr::tr("Fast return: 5 seconds"));
        });
        new QShortcut(QKeySequence::MoveToPreviousLine, q_ptr, q_ptr, [this] {
            controlWidget->setVolume(controlWidget->volume() + 10);
        });
        new QShortcut(QKeySequence::MoveToNextLine, q_ptr, q_ptr, [this] {
            controlWidget->setVolume(controlWidget->volume() - 10);
        });
        new QShortcut(Qt::Key_Space, q_ptr, q_ptr, [this] { player->pause(); });
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
        splitter->addWidget(videoWidget);
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
        auto geometry = videoWidget->geometry();
        geometry = QRect{q_ptr->mapToGlobal(geometry.topLeft()), geometry.size()};
        floatingWidget->setGeometry(geometry);
    }

    void addToPlaylist(const QList<QUrl> &urls)
    {
        auto *playlist = playlistModel->playlist();
        const int previousMediaCount = playlist->mediaCount();
        for (const auto &url : urls) {
            if (isPlaylist(url)) {
                playlist->load(url);
            } else {
                playlist->addMedia(url);
            }
        }
        if (playlist->mediaCount() > previousMediaCount) {
            auto index = playlistModel->index(previousMediaCount, 0);
            playlistView->setCurrentIndex(index);
            q_ptr->jump(index);
        }
    }

    void handleCursor(QMediaPlayer::MediaStatus status)
    {
#ifndef QT_NO_CURSOR
        if (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::BufferingMedia
            || status == QMediaPlayer::StalledMedia) {
            q_ptr->setCursor(QCursor(Qt::BusyCursor));
        } else {
            q_ptr->unsetCursor();
        }
#endif
    }

    void setPosition(qint64 position)
    {
        if (player->isSeekable()) {
            player->setPosition(position);
        }
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
        subTracksMenuPtr->addSeparator();

        videoMenu->addMenu(videoTracksMenuPtr.data());
        audioMenu->addMenu(audioTracksMenuPtr.data());
        subMenu->addMenu(subTracksMenuPtr.data());
    }

    MainWindow *q_ptr;

    QMediaDevices *mediaDevices;
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    VideoWidget *videoWidget;

    QWidget *floatingWidget;
    ControlWidget *controlWidget;
    TitleWidget *titleWidget;

    PlayListView *playlistView;
    PlaylistModel *playlistModel;
    QMenu *playListMenu;

    QMenu *menu;

    QMenu *videoMenu;
    QPointer<QMenu> videoTracksMenuPtr;
    QActionGroup *videoTracksGroup;

    QMenu *audioMenu;
    QPointer<QMenu> audioTracksMenuPtr;
    QActionGroup *audioTracksGroup;

    QMenu *subMenu;
    QPointer<QMenu> subTracksMenuPtr;
    QActionGroup *subTracksGroup;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    d_ptr->setupUI();
    initMenu();
    initPlayListMenu();
    buildConnect();

    setAttribute(Qt::WA_Hover);
    d_ptr->videoWidget->installEventFilter(this);
    d_ptr->playlistView->installEventFilter(this);
    installEventFilter(this);

    resize(1000, 618);
    printAudioOuputDevice();
}

MainWindow::~MainWindow() {}

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
    d_ptr->addToPlaylist(urls);
}

void MainWindow::onOpenWebMedia()
{
    OpenWebMediaDialog dialog(this);
    dialog.setMinimumSize(size() / 2.0);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    d_ptr->addToPlaylist({QUrl(dialog.url())});
}

void MainWindow::onTrackChanged()
{
    d_ptr->resetTrackMenu();

    auto newNoAction = [](QWidget *parent = nullptr) {
        auto *action = new QAction("no", parent);
        action->setData(-1);
        action->setCheckable(true);
        return action;
    };

    const auto audioTracks = d_ptr->player->audioTracks();
    auto *action = newNoAction(this);
    d_ptr->audioTracksMenuPtr->addAction(action);
    d_ptr->audioTracksGroup->addAction(action);
    if (d_ptr->player->activeAudioTrack() == -1) {
        action->setChecked(true);
    }
    for (int i = 0; i < audioTracks.size(); ++i) {
        action = new QAction(trackName(audioTracks.at(i), i), this);
        action->setData(i);
        action->setCheckable(true);
        d_ptr->audioTracksMenuPtr->addAction(action);
        d_ptr->audioTracksGroup->addAction(action);
        if (d_ptr->player->activeAudioTrack() == i) {
            action->setChecked(true);
        }
    }

    const auto videoTracks = d_ptr->player->videoTracks();
    action = newNoAction(this);
    d_ptr->videoTracksMenuPtr->addAction(action);
    d_ptr->videoTracksGroup->addAction(action);
    if (d_ptr->player->activeVideoTrack() == -1) {
        action->setChecked(true);
    }
    for (int i = 0; i < videoTracks.size(); ++i) {
        action = new QAction(trackName(videoTracks.at(i), i), this);
        action->setData(i);
        action->setCheckable(true);
        d_ptr->videoTracksMenuPtr->addAction(action);
        d_ptr->videoTracksGroup->addAction(action);
        if (d_ptr->player->activeVideoTrack() == i) {
            action->setChecked(true);
        }
    }

    const auto subTracks = d_ptr->player->subtitleTracks();
    action = newNoAction(this);
    d_ptr->subTracksMenuPtr->addAction(action);
    d_ptr->subTracksGroup->addAction(action);
    if (d_ptr->player->activeSubtitleTrack() == -1) {
        action->setChecked(true);
    }
    for (int i = 0; i < subTracks.size(); ++i) {
        action = new QAction(trackName(subTracks.at(i), i), this);
        action->setData(i);
        action->setCheckable(true);
        d_ptr->subTracksMenuPtr->addAction(action);
        d_ptr->subTracksGroup->addAction(action);
        if (d_ptr->player->activeSubtitleTrack() == i) {
            action->setChecked(true);
        }
    }
}

void MainWindow::onBufferingProgress(float progress)
{
    if (d_ptr->player->mediaStatus() == QMediaPlayer::StalledMedia) {
        d_ptr->setTitleWidgetText(tr("Stalled %1%").arg(qRound(progress * 100.)));
    } else {
        d_ptr->setTitleWidgetText(tr("Buffering %1%").arg(qRound(progress * 100.)));
    }
}

void MainWindow::onDisplayError()
{
    if (d_ptr->player->error() != QMediaPlayer::NoError) {
        d_ptr->titleWidget->setText(d_ptr->player->errorString());
    }
}

void MainWindow::onStatusChanged(QMediaPlayer::MediaStatus status)
{
    d_ptr->handleCursor(status);

    switch (status) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia: {
        auto *videoSink = d_ptr->player->videoSink();
        auto text = tr("%1 [%2x%3] - Backend:%4")
                        .arg(d_ptr->player->source().fileName(),
                             QString::number(videoSink->videoSize().width()),
                             QString::number(videoSink->videoSize().height()),
                             videoSink->rhi()->backendName());
        setWindowTitle(text);
    } break;
    case QMediaPlayer::LoadingMedia: d_ptr->setTitleWidgetText(tr("Loading...")); break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        d_ptr->setTitleWidgetText(
            tr("Buffering %1%").arg(qRound(d_ptr->player->bufferProgress() * 100.)));
        break;
    case QMediaPlayer::StalledMedia:
        d_ptr->setTitleWidgetText(
            tr("Stalled %1%").arg(qRound(d_ptr->player->bufferProgress() * 100.)));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        d_ptr->playlistModel->playlist()->next();
        break;
    case QMediaPlayer::InvalidMedia: onDisplayError(); break;
    }
}

void MainWindow::onAudioOutputsChanged()
{
    printAudioOuputDevice();
    auto audioDevice = d_ptr->mediaDevices->defaultAudioOutput();
    d_ptr->player->audioOutput()->setDevice(audioDevice);
}

void MainWindow::playlistPositionChanged(int currentItem)
{
    if (currentItem < 0) {
        return;
    }
    auto url = d_ptr->playlistModel->playlist()->currentMedia();
    d_ptr->playlistView->setCurrentIndex(d_ptr->playlistModel->index(currentItem, 0));

    d_ptr->player->setSource(url);
    d_ptr->player->play();
}

void MainWindow::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        d_ptr->playlistModel->playlist()->setCurrentIndex(index.row());
    }
}

auto MainWindow::eventFilter(QObject *watched, QEvent *event) -> bool
{
    if (watched == d_ptr->videoWidget) {
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
                d_ptr->addToPlaylist(urls);
            }
        } break;
        case QEvent::ContextMenu: {
            auto *e = static_cast<QContextMenuEvent *>(event);
            d_ptr->menu->exec(e->globalPos());
        } break;
        case QEvent::Resize:
            QMetaObject::invokeMethod(
                this,
                [this] {
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
                [this] {
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

void MainWindow::buildConnect()
{
    connect(d_ptr->mediaDevices,
            &QMediaDevices::audioOutputsChanged,
            this,
            &MainWindow::onAudioOutputsChanged);

    connect(d_ptr->player,
            &QMediaPlayer::durationChanged,
            d_ptr->controlWidget,
            [this](qint64 duration) { d_ptr->controlWidget->setDuration(duration / 1000); });
    connect(d_ptr->player,
            &QMediaPlayer::positionChanged,
            d_ptr->controlWidget,
            [this](qint64 position) { d_ptr->controlWidget->setPosition(position / 1000); });
    connect(d_ptr->player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onStatusChanged);
    connect(d_ptr->player,
            &QMediaPlayer::bufferProgressChanged,
            this,
            &MainWindow::onBufferingProgress);
    connect(d_ptr->player, &QMediaPlayer::tracksChanged, this, &MainWindow::onTrackChanged);
    connect(d_ptr->player, &QMediaPlayer::errorChanged, this, &MainWindow::onDisplayError);

    connect(d_ptr->controlWidget,
            &ControlWidget::previous,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::previous);
    connect(d_ptr->controlWidget,
            &ControlWidget::next,
            d_ptr->playlistModel->playlist(),
            &QMediaPlaylist::next);
    // connect(d_ptr->controlWidget, &ControlWidget::hoverPosition, this, &MainWindow::onHoverSlider);
    // connect(d_ptr->controlWidget, &ControlWidget::leavePosition, this, &MainWindow::onLeaveSlider);
    connect(d_ptr->controlWidget, &ControlWidget::seek, d_ptr->player, [this](int value) {
        qint64 position = value;
        d_ptr->setPosition(position * 1000);
    });
    connect(d_ptr->controlWidget, &ControlWidget::play, d_ptr->player, [this](bool checked) {
        if (checked && !d_ptr->player->isPlaying()) {
            d_ptr->player->play();
        } else if (checked) {
            d_ptr->player->pause();
        } else if (!checked) {
            d_ptr->player->play();
        }
    });
    connect(d_ptr->controlWidget,
            &ControlWidget::volumeChanged,
            d_ptr->audioOutput,
            [this](int value) {
                d_ptr->audioOutput->setVolume(value / 100.0);
                d_ptr->setTitleWidgetText(tr("Volume: %1").arg(value));
            });
    d_ptr->controlWidget->setVolume(50);
    connect(d_ptr->controlWidget, &ControlWidget::speedChanged, d_ptr->player, [this](double value) {
        d_ptr->player->setPlaybackRate(value);
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
    d_ptr->menu->addMenu(d_ptr->videoMenu);
    d_ptr->menu->addMenu(d_ptr->audioMenu);
    d_ptr->menu->addMenu(d_ptr->subMenu);

    connect(d_ptr->audioTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->player->setActiveAudioTrack(action->data().toInt());
    });
    connect(d_ptr->videoTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->player->setActiveVideoTrack(action->data().toInt());
    });
    connect(d_ptr->subTracksGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        d_ptr->player->setActiveSubtitleTrack(action->data().toInt());
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
            for (const auto &index : std::as_const(indexs)) {
                d_ptr->playlistModel->playlist()->removeMedia(index.row());
            }
        });
    d_ptr->playListMenu->addAction(tr("Clear"), d_ptr->playlistView, [this] {
        d_ptr->playlistModel->playlist()->clear();
    });
}
