#include "mainwindow.hpp"

#include <ffmpeg/avversion.hpp>
#include <ffmpeg/transcode.hpp>

#include <QtWidgets>

#define BUTTON_SIZE QSize(100, 35)

class MainWindow::MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : owner(parent)
    {
        transcode = new Ffmpeg::Transcode(owner);

        inTextEdit = new QTextEdit(owner);
        outTextEdit = new QTextEdit(owner);

        audioCodecCbx = new QComboBox(owner);
        audioCodecCbx->setView(new QListView(audioCodecCbx));
        audioCodecCbx->setMaxVisibleItems(10);
        audioCodecCbx->setStyleSheet("QComboBox {combobox-popup:0;}");
        for (int i = AV_CODEC_ID_MP2; i <= AV_CODEC_ID_CODEC2; i++) {
            audioCodecCbx->addItem(avcodec_get_name(static_cast<AVCodecID>(i)), i);
        }
        audioCodecCbx->setCurrentIndex(audioCodecCbx->findData(AV_CODEC_ID_AAC));
        videoCodecCbx = new QComboBox(owner);
        videoCodecCbx->setView(new QListView(videoCodecCbx));
        videoCodecCbx->setMaxVisibleItems(10);
        videoCodecCbx->setStyleSheet("QComboBox {combobox-popup:0;}");
        for (int i = AV_CODEC_ID_MPEG1VIDEO; i <= AV_CODEC_ID_VVC; i++) {
            videoCodecCbx->addItem(avcodec_get_name(static_cast<AVCodecID>(i)), i);
        }
        videoCodecCbx->setCurrentIndex(videoCodecCbx->findData(AV_CODEC_ID_H264));

        quailtySbx = new QSpinBox(owner);
        quailtySbx->setRange(2, 31);
        quailtySbx->setToolTip(tr("smaller -> better"));

        widthLineEdit = new QLineEdit(owner);
        widthLineEdit->setValidator(new QIntValidator(0, INT_MAX, widthLineEdit));
        heightLineEdit = new QLineEdit(owner);
        heightLineEdit->setValidator(new QIntValidator(0, INT_MAX, heightLineEdit));
        keepAspectRatioCkb = new QCheckBox(tr("keepAspectRatio"), owner);
        keepAspectRatioCkb->setChecked(true);

        startButton = new QToolButton(owner);
        startButton->setText(QObject::tr("Start"));
        startButton->setMinimumSize(BUTTON_SIZE);
        progressBar = new QProgressBar(owner);
        progressBar->setRange(0, 100);
    }

    void initInputFileAttribute(const QString &filePath)
    {
        bool audioSet = false;
        bool videoSet = false;
        auto codecs = Ffmpeg::getFileCodecInfo(filePath);
        for (const auto &codec : qAsConst(codecs)) {
            if (audioSet && videoSet) {
                break;
            }

            switch (codec.mediaType) {
            case AVMEDIA_TYPE_AUDIO:
                if (!audioSet) {
                    auto index = audioCodecCbx->findData(codec.codecID);
                    if (index > 0) {
                        audioCodecCbx->setCurrentIndex(index);
                        audioSet = true;
                    }
                }
                break;
            case AVMEDIA_TYPE_VIDEO:
                if (!videoSet) {
                    auto index = videoCodecCbx->findData(codec.codecID);
                    if (index > 0) {
                        videoCodecCbx->setCurrentIndex(index);
                        videoSet = true;
                    }
                    widthLineEdit->blockSignals(true);
                    heightLineEdit->blockSignals(true);
                    widthLineEdit->setText(QString::number(codec.size.width()));
                    heightLineEdit->setText(QString::number(codec.size.height()));
                    originalSize = codec.size;
                    widthLineEdit->blockSignals(false);
                    heightLineEdit->blockSignals(false);
                    if (codec.quantizer.first != -1 && codec.quantizer.second != -1) {
                        quailtySbx->setRange(codec.quantizer.first, codec.quantizer.second);
                        quailtySbx->setValue((codec.quantizer.first + codec.quantizer.second) / 2);
                    }
                }
                break;
            default: break;
            }
        }
    }

    QWidget *owner;

    Ffmpeg::Transcode *transcode;

    QTextEdit *inTextEdit;
    QTextEdit *outTextEdit;

    QComboBox *audioCodecCbx;
    QComboBox *videoCodecCbx;
    QSpinBox *quailtySbx;
    QLineEdit *widthLineEdit;
    QLineEdit *heightLineEdit;
    QSize originalSize = QSize(-1, -1);
    QCheckBox *keepAspectRatioCkb;

    QToolButton *startButton;
    QProgressBar *progressBar;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    Ffmpeg::printFfmpegInfo();

    setupUI();
    buildConnect();
    resize(700, 430);
}

MainWindow::~MainWindow() {}

void MainWindow::onOpenInputFile()
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

    d_ptr->inTextEdit->setPlainText(filePath);

    d_ptr->initInputFileAttribute(filePath);
}

void MainWindow::onOpenOutputFile()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath
        = QFileDialog::getSaveFileName(this,
                                       tr("Save File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->outTextEdit->setPlainText(filePath);
}

void MainWindow::onStart()
{
    if (d_ptr->startButton->text() == tr("Start")) {
        auto inPath = d_ptr->inTextEdit->toPlainText();
        if (!QFile::exists(inPath)) {
            QMessageBox::warning(this, tr("Not exist"), tr("Input file path does not exist!"));
            return;
        }
        auto outPath = d_ptr->outTextEdit->toPlainText();

        d_ptr->transcode->setInFilePath(inPath);
        d_ptr->transcode->setOutFilePath(outPath);
        d_ptr->transcode->setAudioEncodecID(
            static_cast<AVCodecID>(d_ptr->audioCodecCbx->currentData().toInt()));
        d_ptr->transcode->setVideoEnCodecID(
            static_cast<AVCodecID>(d_ptr->videoCodecCbx->currentData().toInt()));
        d_ptr->transcode->setSize(
            {d_ptr->widthLineEdit->text().toInt(), d_ptr->heightLineEdit->text().toInt()});
        d_ptr->transcode->setQuailty(d_ptr->quailtySbx->value());
        d_ptr->transcode->startTranscode();
        d_ptr->startButton->setText(tr("Stop"));
    } else if (d_ptr->startButton->text() == tr("Stop")) {
        d_ptr->transcode->stopTranscode();
        d_ptr->startButton->setText(tr("Start"));
    }
}

void MainWindow::setupUI()
{
    auto inBtn = new QToolButton(this);
    inBtn->setText(tr("Open In"));
    inBtn->setMinimumSize(BUTTON_SIZE);
    connect(inBtn, &QToolButton::clicked, this, &MainWindow::onOpenInputFile);
    auto outBtn = new QToolButton(this);
    outBtn->setText(tr("Open Out"));
    outBtn->setMinimumSize(BUTTON_SIZE);
    connect(outBtn, &QToolButton::clicked, this, &MainWindow::onOpenOutputFile);

    auto editLayout = new QGridLayout;
    editLayout->addWidget(d_ptr->inTextEdit, 0, 0, 1, 1);
    editLayout->addWidget(inBtn, 0, 1, 1, 1);
    editLayout->addWidget(d_ptr->outTextEdit, 1, 0, 1, 1);
    editLayout->addWidget(outBtn, 1, 1, 1, 1);

    auto groupLayout1 = new QHBoxLayout;
    groupLayout1->addWidget(new QLabel(tr("Audio Codec ID:"), this));
    groupLayout1->addWidget(d_ptr->audioCodecCbx);
    groupLayout1->addWidget(new QLabel(tr("Video Codec ID:"), this));
    groupLayout1->addWidget(d_ptr->videoCodecCbx);
    groupLayout1->addWidget(new QLabel(tr("Quality:"), this));
    groupLayout1->addWidget(d_ptr->quailtySbx);
    auto groupLayout2 = new QHBoxLayout;
    groupLayout2->addWidget(new QLabel(tr("Width:"), this));
    groupLayout2->addWidget(d_ptr->widthLineEdit);
    groupLayout2->addWidget(new QLabel(tr("Height:"), this));
    groupLayout2->addWidget(d_ptr->heightLineEdit);
    groupLayout2->addWidget(d_ptr->keepAspectRatioCkb);
    auto groupBox = new QGroupBox(tr("Encoder Settings"), this);
    auto groupLayout = new QVBoxLayout(groupBox);
    groupLayout->addLayout(groupLayout1);
    groupLayout->addLayout(groupLayout2);

    auto displayLayout = new QHBoxLayout;
    displayLayout->addWidget(d_ptr->startButton);
    displayLayout->addWidget(d_ptr->progressBar);

    auto widget = new QWidget(this);
    auto layout = new QVBoxLayout(widget);
    layout->addLayout(editLayout);
    layout->addWidget(groupBox);
    layout->addLayout(displayLayout);
    setCentralWidget(widget);
}

void MainWindow::buildConnect()
{
    connect(d_ptr->widthLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!d_ptr->keepAspectRatioCkb->isChecked() || !d_ptr->originalSize.isValid()) {
            return;
        }
        auto multiple = d_ptr->originalSize.width() * 1.0 / text.toInt();
        int height = d_ptr->originalSize.height() / multiple;
        d_ptr->heightLineEdit->blockSignals(true);
        d_ptr->heightLineEdit->setText(QString::number(height));
        d_ptr->heightLineEdit->blockSignals(false);
    });
    connect(d_ptr->heightLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!d_ptr->keepAspectRatioCkb->isChecked() || !d_ptr->originalSize.isValid()) {
            return;
        }
        auto multiple = d_ptr->originalSize.height() * 1.0 / text.toInt();
        int width = d_ptr->originalSize.width() / multiple;
        d_ptr->widthLineEdit->blockSignals(true);
        d_ptr->widthLineEdit->setText(QString::number(width));
        d_ptr->widthLineEdit->blockSignals(false);
    });
    connect(d_ptr->keepAspectRatioCkb, &QCheckBox::stateChanged, this, [this] {
        if (!d_ptr->keepAspectRatioCkb->isChecked() || !d_ptr->originalSize.isValid()) {
            return;
        }
        auto multiple = d_ptr->originalSize.width() * 1.0 / d_ptr->widthLineEdit->text().toInt();
        int height = d_ptr->originalSize.height() / multiple;
        d_ptr->heightLineEdit->blockSignals(true);
        d_ptr->heightLineEdit->setText(QString::number(height));
        d_ptr->heightLineEdit->blockSignals(false);
    });

    connect(d_ptr->startButton, &QToolButton::clicked, this, &MainWindow::onStart);
    connect(d_ptr->transcode, &Ffmpeg::Transcode::progressChanged, this, [this](qreal value) {
        d_ptr->progressBar->setValue(value * 100);
    });
    connect(d_ptr->transcode, &Ffmpeg::Transcode::finished, this, [this] {
        d_ptr->startButton->setEnabled(true);
        d_ptr->progressBar->setValue(0);
    });
}
