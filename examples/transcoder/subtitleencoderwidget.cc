#include "subtitleencoderwidget.hpp"
#include "subtitleencodertableview.hpp"

#include <QtWidgets>

class SubtitleEncoderWidget::SubtitleEncoderWidgetPrivate
{
public:
    explicit SubtitleEncoderWidgetPrivate(SubtitleEncoderWidget *q)
        : q_ptr(q)
    {
        subtitleEncoderTable = new SubtitleEncoderTableView(q_ptr);
    }

    SubtitleEncoderWidget *q_ptr;

    SubtitleEncoderTableView *subtitleEncoderTable;
    Ffmpeg::EncodeContexts encodeContexts;
};

SubtitleEncoderWidget::SubtitleEncoderWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new SubtitleEncoderWidgetPrivate(this))
{
    setupUI();
}

SubtitleEncoderWidget::~SubtitleEncoderWidget() = default;

void SubtitleEncoderWidget::setDecodeContext(const Ffmpeg::EncodeContexts &decodeContexts)
{
    d_ptr->encodeContexts = decodeContexts;
    d_ptr->subtitleEncoderTable->setDatas(decodeContexts);
}

auto SubtitleEncoderWidget::encodeContexts() const -> Ffmpeg::EncodeContexts
{
    return d_ptr->subtitleEncoderTable->datas();
}

void SubtitleEncoderWidget::onReset()
{
    d_ptr->subtitleEncoderTable->setDatas(d_ptr->encodeContexts);
}

void SubtitleEncoderWidget::onLoad()
{
    const auto path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                          .value(0, QDir::homePath());
    const auto filePath = QFileDialog::getOpenFileName(this,
                                                       tr("Open File"),
                                                       path,
                                                       tr("Subtitle (*.srt *.ass *.txt)"));
    if (filePath.isEmpty()) {
        return;
    }

    Ffmpeg::EncodeContext encodeContext;
    encodeContext.sourceInfo = filePath;
    encodeContext.external = true;
    d_ptr->subtitleEncoderTable->append({encodeContext});
}

void SubtitleEncoderWidget::setupUI()
{
    auto *resetButton = new QToolButton(this);
    resetButton->setText(tr("Reset"));
    connect(resetButton, &QToolButton::clicked, this, &SubtitleEncoderWidget::onReset);

    auto *loadButton = new QToolButton(this);
    loadButton->setText(tr("Load Subtitle"));
    connect(loadButton, &QToolButton::clicked, this, &SubtitleEncoderWidget::onLoad);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(buttonLayout);
    layout->addWidget(d_ptr->subtitleEncoderTable);
}
