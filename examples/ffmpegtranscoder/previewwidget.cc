#include "previewwidget.hpp"

#include <ffmpeg/frame.hpp>
#include <ffmpeg/videorender/videorendercreate.hpp>

#include <QtWidgets>

class PreviewWidget::PreviewWidgetPrivate
{
public:
    explicit PreviewWidgetPrivate(PreviewWidget *q)
        : q_ptr(q)
    {
        const QSize buttonSize(50, 50);

        renderPtr.reset(Ffmpeg::VideoRenderCreate::create(Ffmpeg::VideoRenderCreate::Opengl));
        renderPtr->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        renderPtr->setBackgroundColor(Qt::white);
        infoLabel = new QLabel(q_ptr);
        infoLabel->setAlignment(Qt::AlignCenter);
        leftButton = new QToolButton(q_ptr);
        leftButton->setMinimumSize(buttonSize);
        leftButton->setText("<");
        rightButton = new QToolButton(q_ptr);
        rightButton->setMinimumSize(buttonSize);
        rightButton->setText(">");
    }

    void setCurrentFrame(int index)
    {
        Q_ASSERT(index >= 0 && index < framePtrs.size());
        frameIndex = index;
        renderPtr->setFrame(framePtrs[frameIndex]);
        auto currentTime = QTime::fromMSecsSinceStartOfDay(framePtrs[frameIndex]->pts() / 1000)
                               .toString("hh:mm:ss.zzz");
        infoLabel->setText(QCoreApplication::translate("PreviewWidgetPrivate",
                                                       "Source Preview: %1 / %2, Position: %3")
                               .arg(QString::number(frameIndex + 1),
                                    QString::number(framePtrs.size()),
                                    currentTime));

        leftButton->setEnabled(frameIndex != 0);
        rightButton->setEnabled(frameIndex != (framePtrs.size() - 1));
    }

    PreviewWidget *q_ptr;

    QScopedPointer<Ffmpeg::VideoRender> renderPtr;
    QLabel *infoLabel;
    QToolButton *leftButton;
    QToolButton *rightButton;

    std::vector<Ffmpeg::FramePtr> framePtrs;
    int frameIndex = 0;
};

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new PreviewWidgetPrivate(this))
{
    setupUI();
    buildConnect();
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::setFrames(const std::vector<QSharedPointer<Ffmpeg::Frame>> &framePtrs)
{
    d_ptr->framePtrs = framePtrs;
    d_ptr->frameIndex = 2;
    onPerFrame();
}

void PreviewWidget::onPerFrame()
{
    d_ptr->setCurrentFrame(d_ptr->frameIndex - 1);
}

void PreviewWidget::onNextFrame()
{
    d_ptr->setCurrentFrame(d_ptr->frameIndex + 1);
}

void PreviewWidget::setupUI()
{
    auto *renderLayout = new QHBoxLayout;
    renderLayout->addWidget(d_ptr->leftButton);
    renderLayout->addWidget(d_ptr->renderPtr->widget());
    renderLayout->addWidget(d_ptr->rightButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(d_ptr->infoLabel);
    layout->addLayout(renderLayout);
}

void PreviewWidget::buildConnect()
{
    connect(d_ptr->leftButton, &QToolButton::clicked, this, &PreviewWidget::onPerFrame);
    connect(d_ptr->rightButton, &QToolButton::clicked, this, &PreviewWidget::onNextFrame);
}
