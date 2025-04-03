#include "videoencoderwidget.hpp"
#include "commonwidgets.hpp"

#include <ffmpeg/avcontextinfo.h>
#include <ffmpeg/codeccontext.h>
#include <ffmpeg/ffmpegutils.hpp>

#include <QtWidgets>

class VideoEncoderWidget::VideoEncoderWidgetPrivate
{
public:
    explicit VideoEncoderWidgetPrivate(VideoEncoderWidget *q)
        : q_ptr(q)
    {
        videoEncoderCbx = CommonWidgets::createComboBox(q_ptr);
        auto videoCodecs = Ffmpeg::getCodecsInfo(AVMEDIA_TYPE_VIDEO, true);
        for (const auto &codec : std::as_const(videoCodecs)) {
            videoEncoderCbx->addItem(codec.displayName, QVariant::fromValue(codec));
            if (codec.codecId == AV_CODEC_ID_H264) {
                videoEncoderCbx->setCurrentText(codec.displayName);
            }
        }
        videoEncoderCbx->model()->sort(0);

        gpuDecodeCbx = new QCheckBox(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                 "Use GPU Decode"),
                                     q_ptr);
        gpuDecodeCbx->setChecked(true);

        initInvailedGroupBox();
        initCrf();
        initBitrateGroupBox();
        initSizeGroupBox();

        init();
    }

    void initInvailedGroupBox()
    {
        presetCbx = CommonWidgets::createComboBox(q_ptr);
        tuneCbx = CommonWidgets::createComboBox(q_ptr);
        profileCbx = CommonWidgets::createComboBox(q_ptr);
    }

    void initCrf()
    {
        crfRadioButton = new QRadioButton(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                      "Crf:"),
                                          q_ptr);
        crfSbx = new QSpinBox(q_ptr);
        crfSbx->setToolTip(
            QCoreApplication::translate("VideoEncoderWidgetPrivate", "bigger -> better"));
    }

    void initBitrateGroupBox()
    {
        bitrateRadioButton
            = new QRadioButton(QCoreApplication::translate("VideoEncoderWidgetPrivate", "Bitrate:"),
                               q_ptr);
        bitrateWidget = new QWidget(q_ptr);
        minBitrateSbx = new QSpinBox(q_ptr);
        minBitrateSbx->setRange(0, INT_MAX);
        bitrateSbx = new QSpinBox(q_ptr);
        bitrateSbx->setRange(0, INT_MAX);
        maxBitrateSbx = new QSpinBox(q_ptr);
        maxBitrateSbx->setRange(0, INT_MAX);

        auto *bitrateLayout = new QFormLayout(bitrateWidget);
        bitrateLayout->setContentsMargins(QMargins());
        bitrateLayout->addRow(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                          "Min Bitrate:"),
                              minBitrateSbx);
        bitrateLayout->addRow(QCoreApplication::translate("VideoEncoderWidgetPrivate", "Bitrate:"),
                              bitrateSbx);
        bitrateLayout->addRow(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                          "Max Bitrate:"),
                              maxBitrateSbx);
    }

    void initSizeGroupBox()
    {
        widthSbx = new QSpinBox(q_ptr);
        widthSbx->setRange(0, INT_MAX);
        heightSbx = new QSpinBox(q_ptr);
        heightSbx->setRange(0, INT_MAX);
        aspectCheckBox = new QCheckBox(QCoreApplication::translate("VideoEncoderWidgetPrivate",
                                                                   "Keep aspect ratio:"),
                                       q_ptr);
        aspectCheckBox->setChecked(true);
    }

    void calBitrate() const
    {
        auto w = widthSbx->value();
        auto h = heightSbx->value();
        minBitrateSbx->setValue(w * h);
        maxBitrateSbx->setValue(w * h * 4);
        bitrateSbx->setValue(w * h * 4);
    }

    void init() const
    {
        Ffmpeg::EncodeContext encodeParam;

        presetCbx->clear();
        presetCbx->addItems(Ffmpeg::EncodeLimit::presets);
        presetCbx->setCurrentText(encodeParam.preset);

        tuneCbx->clear();
        tuneCbx->addItems(Ffmpeg::EncodeLimit::tunes);
        tuneCbx->setCurrentText(encodeParam.tune);

        profileCbx->clear();

        crfSbx->setRange(Ffmpeg::EncodeLimit::crf_min, Ffmpeg::EncodeLimit::crf_max);
        crfSbx->setValue(encodeParam.crf);
    }

    [[nodiscard]] auto currentCodecName() const -> QString
    {
        return videoEncoderCbx->currentData().value<Ffmpeg::CodecInfo>().name;
    }

    VideoEncoderWidget *q_ptr;

    QComboBox *videoEncoderCbx;
    QCheckBox *gpuDecodeCbx;

    QRadioButton *crfRadioButton;
    QSpinBox *crfSbx;

    QRadioButton *bitrateRadioButton;
    QWidget *bitrateWidget;
    QSpinBox *minBitrateSbx;
    QSpinBox *bitrateSbx;
    QSpinBox *maxBitrateSbx;

    QSpinBox *widthSbx;
    QSpinBox *heightSbx;
    QCheckBox *aspectCheckBox;

    QComboBox *presetCbx;
    QComboBox *tuneCbx;
    QComboBox *profileCbx;

    Ffmpeg::EncodeContext decodeContext;
};

VideoEncoderWidget::VideoEncoderWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new VideoEncoderWidgetPrivate(this))
{
    setupUI();
    buildConnect();
    onEncoderChanged();
    d_ptr->crfRadioButton->click();
}

VideoEncoderWidget::~VideoEncoderWidget() = default;

auto VideoEncoderWidget::encodeContext() const -> Ffmpeg::EncodeContext
{
    Ffmpeg::EncodeContext encodeContext;
    encodeContext.streamIndex = d_ptr->decodeContext.streamIndex;
    encodeContext.mediaType = AVMEDIA_TYPE_VIDEO;
    encodeContext.setEncoderName(d_ptr->currentCodecName());
    encodeContext.size = {d_ptr->widthSbx->value(), d_ptr->heightSbx->value()};
    encodeContext.minBitrate = d_ptr->minBitrateSbx->value();
    encodeContext.maxBitrate = d_ptr->maxBitrateSbx->value();
    encodeContext.bitrate = d_ptr->bitrateSbx->value();
    encodeContext.crf = d_ptr->crfRadioButton->isChecked() ? d_ptr->crfSbx->value()
                                                           : Ffmpeg::EncodeLimit::invalid_crf;
    encodeContext.preset = d_ptr->presetCbx->currentText();
    encodeContext.tune = d_ptr->tuneCbx->currentText();
    // encodeContext.profile = d_ptr->profileCbx->currentText();

    return encodeContext;
}

bool VideoEncoderWidget::isGpuDecode() const
{
    return d_ptr->gpuDecodeCbx->isChecked();
}

void VideoEncoderWidget::setDecodeContext(const Ffmpeg::EncodeContext &decodeContext)
{
    Ffmpeg::CodecInfo codec{decodeContext.codecInfo().name};
    auto index = d_ptr->videoEncoderCbx->findData(QVariant::fromValue(codec));
    if (index >= 0) {
        d_ptr->videoEncoderCbx->setCurrentIndex(index);
    }

    d_ptr->widthSbx->blockSignals(true);
    d_ptr->heightSbx->blockSignals(true);
    d_ptr->widthSbx->setValue(decodeContext.size.width());
    d_ptr->heightSbx->setValue(decodeContext.size.height());
    d_ptr->widthSbx->blockSignals(false);
    d_ptr->heightSbx->blockSignals(false);

    if (decodeContext.bitrate <= 0) {
        d_ptr->calBitrate();
    } else {
        d_ptr->bitrateSbx->setValue(decodeContext.bitrate);
        if (decodeContext.maxBitrate <= 0) {
            d_ptr->maxBitrateSbx->setValue(decodeContext.bitrate);
        }
        if (decodeContext.minBitrate <= 0) {
            d_ptr->minBitrateSbx->setValue(decodeContext.bitrate);
        }
    }

    d_ptr->decodeContext = decodeContext;
}

void VideoEncoderWidget::onEncoderChanged()
{
    d_ptr->profileCbx->clear();

    QScopedPointer<Ffmpeg::AVContextInfo> contextInfoPtr(new Ffmpeg::AVContextInfo);
    if (!contextInfoPtr->initEncoder(d_ptr->currentCodecName())) {
        return;
    }
    auto profiles = contextInfoPtr->codecCtx()->supportedProfiles();
    for (const auto &profile : std::as_const(profiles)) {
        d_ptr->profileCbx->addItem(profile.name, profile.profile);
    }
}

void VideoEncoderWidget::onVideoWidthChanged()
{
    if (!d_ptr->aspectCheckBox->isChecked() || !d_ptr->decodeContext.size.isValid()) {
        return;
    }
    auto multiple = d_ptr->decodeContext.size.width() * 1.0 / d_ptr->widthSbx->value();
    int height = d_ptr->decodeContext.size.height() / multiple;
    d_ptr->heightSbx->blockSignals(true);
    d_ptr->heightSbx->setValue(height);
    d_ptr->heightSbx->blockSignals(false);
    d_ptr->calBitrate();
}

void VideoEncoderWidget::onVideoHeightChanged()
{
    if (!d_ptr->aspectCheckBox->isChecked() || !d_ptr->decodeContext.size.isValid()) {
        return;
    }
    auto multiple = d_ptr->decodeContext.size.height() * 1.0 / d_ptr->heightSbx->value();
    int width = d_ptr->decodeContext.size.width() / multiple;
    d_ptr->widthSbx->blockSignals(true);
    d_ptr->widthSbx->setValue(width);
    d_ptr->widthSbx->blockSignals(false);
    d_ptr->calBitrate();
}

void VideoEncoderWidget::onQualityTypeChanged()
{
    auto enabled = d_ptr->crfRadioButton->isChecked();
    d_ptr->crfSbx->setEnabled(enabled);
    d_ptr->bitrateWidget->setEnabled(!enabled);
}

void VideoEncoderWidget::setupUI()
{
    auto *codecLayout = new QHBoxLayout;
    codecLayout->setSpacing(20);
    codecLayout->addWidget(new QLabel(tr("Encoder:"), this));
    codecLayout->addWidget(d_ptr->videoEncoderCbx);
    codecLayout->addStretch();
    codecLayout->addWidget(d_ptr->gpuDecodeCbx);

    auto *sizeGroupBox = new QGroupBox(tr("Size"), this);
    auto *sizeLayout = new QFormLayout(sizeGroupBox);
    sizeLayout->addRow(tr("Width:"), d_ptr->widthSbx);
    sizeLayout->addRow(tr("Height:"), d_ptr->heightSbx);
    sizeLayout->addRow(d_ptr->aspectCheckBox);

    auto *invailedGroupBox = new QGroupBox(tr("Invalid setting"), this);
    auto *invailedLayout = new QFormLayout(invailedGroupBox);
    invailedLayout->addRow(tr("Preset:"), d_ptr->presetCbx);
    invailedLayout->addRow(tr("Tune:"), d_ptr->tuneCbx);
    invailedLayout->addRow(tr("Profile:"), d_ptr->profileCbx);

    auto *qualityGroupBox = new QGroupBox(tr("Quality"), this);
    auto *qualityLayout = new QFormLayout(qualityGroupBox);
    qualityLayout->addRow(d_ptr->crfRadioButton, d_ptr->crfSbx);
    qualityLayout->addRow(d_ptr->bitrateRadioButton, d_ptr->bitrateWidget);

    auto *layout = new QGridLayout(this);
    layout->addLayout(codecLayout, 0, 0, 1, 2);
    layout->addWidget(sizeGroupBox, 1, 0, 1, 1);
    layout->addWidget(qualityGroupBox, 2, 0, 1, 1);
    layout->addWidget(invailedGroupBox, 1, 1, 2, 1);
}

void VideoEncoderWidget::buildConnect()
{
    connect(d_ptr->videoEncoderCbx,
            &QComboBox::currentIndexChanged,
            this,
            &VideoEncoderWidget::onEncoderChanged);
    connect(d_ptr->widthSbx,
            &QSpinBox::valueChanged,
            this,
            &VideoEncoderWidget::onVideoWidthChanged);
    connect(d_ptr->heightSbx,
            &QSpinBox::valueChanged,
            this,
            &VideoEncoderWidget::onVideoHeightChanged);
    connect(d_ptr->aspectCheckBox,
            &QCheckBox::checkStateChanged,
            this,
            &VideoEncoderWidget::onVideoWidthChanged);

    auto *buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(d_ptr->crfRadioButton);
    buttonGroup->addButton(d_ptr->bitrateRadioButton);
    connect(buttonGroup, &QButtonGroup::idClicked, this, &VideoEncoderWidget::onQualityTypeChanged);
}
