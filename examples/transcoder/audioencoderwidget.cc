#include "audioencoderwidget.hpp"
#include "audioencodertableview.hpp"

#include <QtWidgets>

class AudioEncoderWidget::AudioEncoderWidgetPrivate
{
public:
    explicit AudioEncoderWidgetPrivate(AudioEncoderWidget *q)
        : q_ptr(q)
    {
        audioEncoderTable = new AudioEncoderTableView(q_ptr);
    }

    AudioEncoderWidget *q_ptr;

    AudioEncoderTableView *audioEncoderTable;
    Ffmpeg::EncodeContexts encodeContexts;
};

AudioEncoderWidget::AudioEncoderWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new AudioEncoderWidgetPrivate(this))
{
    setupUI();
}

AudioEncoderWidget::~AudioEncoderWidget() = default;

void AudioEncoderWidget::setDecodeContext(const Ffmpeg::EncodeContexts &decodeContexts)
{
    d_ptr->encodeContexts = decodeContexts;
    d_ptr->audioEncoderTable->setDatas(decodeContexts);
}

auto AudioEncoderWidget::encodeContexts() const -> Ffmpeg::EncodeContexts
{
    return d_ptr->audioEncoderTable->datas();
}

void AudioEncoderWidget::onReset()
{
    d_ptr->audioEncoderTable->setDatas(d_ptr->encodeContexts);
}

void AudioEncoderWidget::setupUI()
{
    auto *button = new QToolButton(this);
    button->setText(tr("Reset"));
    connect(button, &QToolButton::clicked, this, &AudioEncoderWidget::onReset);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(button);
    layout->addWidget(d_ptr->audioEncoderTable);
}
