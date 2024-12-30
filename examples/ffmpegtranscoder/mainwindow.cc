#include "mainwindow.hpp"
#include "audioencoderwidget.hpp"
#include "outputwidget.hpp"
#include "previewwidget.hpp"
#include "sourcewidget.hpp"
#include "stautuswidget.hpp"
#include "subtitleencoderwidget.hpp"
#include "videoencoderwidget.hpp"

#include <examples/common/commonstr.hpp>
#include <ffmpeg/averror.h>
#include <ffmpeg/encodecontext.hpp>
#include <ffmpeg/event/errorevent.hpp>
#include <ffmpeg/event/trackevent.hpp>
#include <ffmpeg/event/valueevent.hpp>
#include <ffmpeg/ffmpegutils.hpp>
#include <ffmpeg/transcoder.hpp>
#include <ffmpeg/widgets/mediainfodialog.hpp>

#include <QtWidgets>

class MainWindow::MainWindowPrivate
{
public:
    explicit MainWindowPrivate(MainWindow *q)
        : q_ptr(q)
    {
        Ffmpeg::printFfmpegInfo();

        Ffmpeg::EncodeContext encodeContext;

        transcoder = new Ffmpeg::Transcoder(q_ptr);

        sourceWidget = new SourceWidget(q_ptr);
        tabWidget = new QTabWidget(q_ptr);
        previewWidget = new PreviewWidget(q_ptr);
        videoEncoderWidget = new VideoEncoderWidget(q_ptr);
        audioEncoderWidget = new AudioEncoderWidget(q_ptr);
        subtitleEncoderWidget = new SubtitleEncoderWidget(q_ptr);
        tabWidget->addTab(previewWidget, Common::Tr::tr("Preview"));
        tabWidget->addTab(videoEncoderWidget, Common::Tr::tr("Video"));
        tabWidget->addTab(audioEncoderWidget, Common::Tr::tr("Audio"));
        tabWidget->addTab(subtitleEncoderWidget, Common::Tr::tr("Subtitle"));

        outPutWidget = new OutPutWidget(q_ptr);
        statusWidget = new StautusWidget(q_ptr);

        fpsTimer = new QTimer(q_ptr);

        subtitleTextEdit = new QTextEdit(q_ptr);
    }

    void initInputFileAttribute(const QString &filePath) const
    {
        transcoder->setInFilePath(filePath);
        transcoder->startPreviewFrames(10);
        transcoder->parseInputFile();
    }

    void initUI() const
    {
        QSize size;
        double frameRate = 0;
        QString format;
        int videoCount = 0;
        int audioCount = 0;
        int subtitleCount = 0;

        auto mediaInfo = transcoder->mediaInfo();
        auto streamInfos = mediaInfo.streamInfos;
        for (const auto &streamInfo : std::as_const(streamInfos)) {
            switch (streamInfo.mediaType) {
            case AVMEDIA_TYPE_VIDEO:
                videoCount++;
                size = streamInfo.size;
                frameRate = streamInfo.frameRate;
                format = streamInfo.format;
                break;
            case AVMEDIA_TYPE_AUDIO: audioCount++; break;
            case AVMEDIA_TYPE_SUBTITLE: subtitleCount++; break;
            default: break;
            }
        }

        auto fileName = QFileInfo(mediaInfo.url).fileName();
        auto info = QCoreApplication::translate(
                        "MainWindowPrivate",
                        "%1\nDuration: %2, %3, %4x%5, %6FPS, %7 video, %8 audio, %9 subtitle")
                        .arg(fileName,
                             mediaInfo.durationText,
                             format,
                             QString::number(size.width()),
                             QString::number(size.height()),
                             QString::number(frameRate),
                             QString::number(videoCount),
                             QString::number(audioCount),
                             QString::number(subtitleCount));
        sourceWidget->setFileInfo(info);
        sourceWidget->setDuration(mediaInfo.duration / 1000);

        q_ptr->setWindowTitle(fileName);
    }

    MainWindow *q_ptr;

    Ffmpeg::Transcoder *transcoder;

    SourceWidget *sourceWidget;
    QTabWidget *tabWidget;
    PreviewWidget *previewWidget;
    VideoEncoderWidget *videoEncoderWidget;
    AudioEncoderWidget *audioEncoderWidget;
    SubtitleEncoderWidget *subtitleEncoderWidget;
    OutPutWidget *outPutWidget;
    StautusWidget *statusWidget;

    QTimer *fpsTimer;

    QTextEdit *subtitleTextEdit;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
    buildConnect();

    resize(1000, 618);
}

MainWindow::~MainWindow() = default;

void MainWindow::onOpenInputFile()
{
    const auto path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                          .value(0, QDir::homePath());
    const auto filePath
        = QFileDialog::getOpenFileName(this,
                                       tr("Open File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->sourceWidget->setSource(filePath);
    d_ptr->initInputFileAttribute(filePath);
    d_ptr->outPutWidget->setOutputFileName(QFileInfo(filePath).fileName());
}

void MainWindow::onResetConfig()
{
    auto filePath = d_ptr->sourceWidget->source();
    if (filePath.isEmpty()) {
        qWarning() << "filePath.isEmpty()";
        return;
    }
    if (QFile::exists(filePath)) {
        d_ptr->initInputFileAttribute(filePath);
        return;
    }
    QUrl url(filePath);
    if (!url.isValid()) {
        qWarning() << "!url.isValid()";
        return;
    }
    d_ptr->initInputFileAttribute(url.toEncoded());
}

void MainWindow::onOpenSubtitle()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          tr("Open File"),
                                                          path,
                                                          tr("Audio Video (*.srt *.ass *.txt)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->subtitleTextEdit->setPlainText(filePath);
}

void MainWindow::onStart()
{
    if (d_ptr->statusWidget->status() == tr("Start")) {
        auto inPath = d_ptr->sourceWidget->source();
        auto subtitlePath = d_ptr->subtitleTextEdit->toPlainText();
        auto outPath = d_ptr->outPutWidget->outputFilePath();
        if (inPath.isEmpty() || outPath.isEmpty()) {
            return;
        }

        d_ptr->transcoder->setInFilePath(inPath);
        d_ptr->transcoder->setOutFilePath(outPath);
        d_ptr->transcoder->setGpuDecode(d_ptr->videoEncoderWidget->isGpuDecode());

        Ffmpeg::EncodeContexts encodeContexts(d_ptr->transcoder->decodeContexts().size());
        auto videoEncodeContexts = d_ptr->videoEncoderWidget->encodeContext();
        auto audioEncodeContexts = d_ptr->audioEncoderWidget->encodeContexts();
        auto subtitleEncodeContexts = d_ptr->subtitleEncoderWidget->encodeContexts();
        encodeContexts.replace(videoEncodeContexts.streamIndex, videoEncodeContexts);
        for (const auto &context : std::as_const(audioEncodeContexts)) {
            encodeContexts.replace(context.streamIndex, context);
        }
        for (const auto &context : std::as_const(subtitleEncodeContexts)) {
            encodeContexts.replace(context.streamIndex, context);
        }
        d_ptr->transcoder->setEncodeContexts(encodeContexts);

        if (QFile::exists(subtitlePath)) {
            d_ptr->transcoder->setSubtitleFilename(subtitlePath);
        }
        d_ptr->transcoder->startTranscode();
        d_ptr->statusWidget->setStatus(tr("Stop"));

        auto filename = QFile::exists(inPath) ? QFileInfo(inPath).fileName()
                                              : QFileInfo(QUrl(inPath).toString()).fileName();
        setWindowTitle(filename);

        d_ptr->fpsTimer->start(1000);
    } else if (d_ptr->statusWidget->status() == tr("Stop")) {
        d_ptr->transcoder->stopTranscode();
        d_ptr->statusWidget->setProgress(0);
        d_ptr->statusWidget->setStatus(tr("Start"));

        d_ptr->fpsTimer->stop();

        d_ptr->transcoder->parseInputFile();
    }
}

void MainWindow::onShowMediaInfo()
{
    Ffmpeg::MediaInfoDialog dialog(this);
    dialog.setMediaInfo(d_ptr->transcoder->mediaInfo());
    dialog.exec();
}

void MainWindow::onProcessEvents()
{
    while (d_ptr->transcoder->propertyChangeEventSize() > 0) {
        auto eventPtr = d_ptr->transcoder->takePropertyChangeEvent();
        switch (eventPtr->type()) {
        case Ffmpeg::PropertyChangeEvent::EventType::Position: {
            auto *positionEvent = dynamic_cast<Ffmpeg::PositionEvent *>(eventPtr.data());
            d_ptr->statusWidget->setProgress(positionEvent->position() * 100.0
                                             / d_ptr->transcoder->duration());
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::MediaTrack: {
            d_ptr->initUI();

            auto decodeContexts = d_ptr->transcoder->decodeContexts();
            Ffmpeg::EncodeContexts audioDecodeContexts;
            Ffmpeg::EncodeContexts videoDecodeContexts;
            Ffmpeg::EncodeContexts subtitleDecodeContexts;
            for (const auto &decodeContext : std::as_const(decodeContexts)) {
                switch (decodeContext.mediaType) {
                case AVMEDIA_TYPE_AUDIO: audioDecodeContexts.append(decodeContext); break;
                case AVMEDIA_TYPE_VIDEO: videoDecodeContexts.append(decodeContext); break;
                case AVMEDIA_TYPE_SUBTITLE: subtitleDecodeContexts.append(decodeContext); break;
                default: break;
                }
            }
            d_ptr->audioEncoderWidget->setDecodeContext(audioDecodeContexts);
            d_ptr->subtitleEncoderWidget->setDecodeContext(subtitleDecodeContexts);
            if (!videoDecodeContexts.isEmpty()) {
                d_ptr->videoEncoderWidget->setDecodeContext(videoDecodeContexts.first());
            }
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::PreviewFramesChanged:
            d_ptr->previewWidget->setFrames(d_ptr->transcoder->previewFrames());
            break;
        case Ffmpeg::PropertyChangeEvent::EventType::AVError: {
            auto *errorEvent = dynamic_cast<Ffmpeg::AVErrorEvent *>(eventPtr.data());
            const auto text = tr("Error[%1]:%2.")
                                  .arg(QString::number(errorEvent->error().errorCode()),
                                       errorEvent->error().errorString());
            qWarning() << text;
            statusBar()->showMessage(text);
        } break;
        case Ffmpeg::PropertyChangeEvent::EventType::Error: {
            auto *errorEvent = dynamic_cast<Ffmpeg::ErrorEvent *>(eventPtr.data());
            const auto text = tr("Error:%1.").arg(errorEvent->error());
            qWarning() << text;
            statusBar()->showMessage(text);
        } break;
        default: break;
        }
    }
}

void MainWindow::setupUI()
{
    auto *fileMenu = new QMenu(tr("File"), this);
    fileMenu->addAction(tr("Open In"), this, &MainWindow::onOpenInputFile);
    fileMenu->addAction(tr("Reset Config"), this, &MainWindow::onResetConfig);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit, Qt::QueuedConnection);
    menuBar()->addMenu(fileMenu);

    auto *subtitleBtn = new QToolButton(this);
    subtitleBtn->setText(tr("Add Subtitle"));
    connect(subtitleBtn, &QToolButton::clicked, this, &MainWindow::onOpenSubtitle);

    auto *editLayout = new QGridLayout;
    editLayout->addWidget(d_ptr->subtitleTextEdit, 1, 0, 1, 1);
    editLayout->addWidget(subtitleBtn, 1, 1, 1, 1);

    auto *widget = new QWidget(this);
    auto *layout = new QVBoxLayout(widget);
    layout->addWidget(d_ptr->sourceWidget);
    layout->addWidget(d_ptr->tabWidget);
    layout->addWidget(d_ptr->outPutWidget);
    setCentralWidget(widget);

    statusBar()->addPermanentWidget(d_ptr->statusWidget);

    auto *tempWidget = new QWidget(this);
    auto *tempLayout = new QVBoxLayout(tempWidget);
    tempLayout->addLayout(editLayout);
    d_ptr->tabWidget->addTab(tempWidget, tr("Temp Widget"));
}

void MainWindow::buildConnect()
{
    connect(d_ptr->transcoder,
            &Ffmpeg::Transcoder::eventIncrease,
            this,
            &MainWindow::onProcessEvents);
    connect(d_ptr->transcoder, &Ffmpeg::Transcoder::finished, this, [this] {
        if (d_ptr->statusWidget->status() == tr("Stop")) {
            d_ptr->statusWidget->click();
        }
    });

    connect(d_ptr->sourceWidget, &SourceWidget::showMediaInfo, this, &MainWindow::onShowMediaInfo);
    connect(d_ptr->statusWidget, &StautusWidget::start, this, &MainWindow::onStart);

    connect(d_ptr->fpsTimer, &QTimer::timeout, this, [this] {
        d_ptr->statusWidget->setFPS(d_ptr->transcoder->fps());
    });
}
