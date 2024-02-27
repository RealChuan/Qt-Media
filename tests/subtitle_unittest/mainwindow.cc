#include "mainwindow.hpp"
#include "subtitlethread.hpp"

#include <QtWidgets>

#define BUTTON_SIZE QSize(100, 35)

class MainWindow::MainWindowPrivate
{
public:
    MainWindowPrivate(QWidget *parent)
        : q_ptr(parent)
    {
        inTextEdit = new QTextEdit(q_ptr);

        startButton = new QToolButton(q_ptr);
        startButton->setText(QObject::tr("Start"));
        startButton->setMinimumSize(BUTTON_SIZE);

        subtitleThread = new SubtitleThread(q_ptr);
    }

    QWidget *q_ptr;

    QTextEdit *inTextEdit;
    QToolButton *startButton;

    SubtitleThread *subtitleThread;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d_ptr(new MainWindowPrivate(this))
{
    setupUI();
    buildConnect();
    resize(500, 300);
}

MainWindow::~MainWindow() {}

void MainWindow::onOpenInputFile()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          tr("Open File"),
                                                          path,
                                                          tr("Audio Video (*.mkv *.ass *.srt)"));
    if (filePath.isEmpty()) {
        return;
    }

    d_ptr->inTextEdit->setPlainText(filePath);
}

void MainWindow::onStart()
{
    if (d_ptr->startButton->text() == tr("Start")) {
        auto inPath = d_ptr->inTextEdit->toPlainText();
        if (!QFile::exists(inPath)) {
            QMessageBox::warning(this, tr("Not exist"), tr("Input file path does not exist!"));
            return;
        }
        d_ptr->subtitleThread->setFilePath(inPath);
        d_ptr->subtitleThread->startRead();

        d_ptr->startButton->setText(tr("Stop"));

        auto filename = QFile::exists(inPath) ? QFileInfo(inPath).fileName()
                                              : QFileInfo(QUrl(inPath).toString()).fileName();
        setWindowTitle(filename);
    } else if (d_ptr->startButton->text() == tr("Stop")) {
        d_ptr->subtitleThread->stopRead();
        d_ptr->startButton->setText(tr("Start"));
    }
}

void MainWindow::setupUI()
{
    auto inBtn = new QToolButton(this);
    inBtn->setText(tr("Open In"));
    inBtn->setMinimumSize(BUTTON_SIZE);
    connect(inBtn, &QToolButton::clicked, this, &MainWindow::onOpenInputFile);

    auto rightLayout = new QVBoxLayout;
    rightLayout->addWidget(inBtn);
    rightLayout->addWidget(d_ptr->startButton);

    auto widget = new QWidget(this);
    auto layout = new QHBoxLayout(widget);
    layout->addWidget(d_ptr->inTextEdit);
    layout->addLayout(rightLayout);
    setCentralWidget(widget);
}

void MainWindow::buildConnect()
{
    connect(d_ptr->startButton, &QToolButton::clicked, this, &MainWindow::onStart);
    connect(d_ptr->subtitleThread, &SubtitleThread::finished, this, [this] {
        if (d_ptr->startButton->text() == tr("Stop")) {
            d_ptr->startButton->click();
        }
    });
}
