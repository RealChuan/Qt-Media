#include "audioencodertableview.hpp"
#include "audioencodermodel.hpp"
#include "styleditemdelegate.hpp"

#include <QHeaderView>

class AudioEncoderTableView::AudioEncoderTableViewPrivate
{
public:
    explicit AudioEncoderTableViewPrivate(AudioEncoderTableView *q)
        : q_ptr(q)
    {
        model = new AudioEncoderModel(q_ptr);
    }

    AudioEncoderTableView *q_ptr;

    AudioEncoderModel *model;
};

AudioEncoderTableView::AudioEncoderTableView(QWidget *parent)
    : QTableView{parent}
    , d_ptr(new AudioEncoderTableViewPrivate(this))
{
    setupUI();
    buildConnect();
}

AudioEncoderTableView::~AudioEncoderTableView() = default;

void AudioEncoderTableView::setDatas(const Ffmpeg::EncodeContexts &encodeContexts)
{
    d_ptr->model->setDatas(encodeContexts);
}

auto AudioEncoderTableView::datas() const -> Ffmpeg::EncodeContexts
{
    return d_ptr->model->datas();
}

void AudioEncoderTableView::onRemoved(const QModelIndex &index)
{
    d_ptr->model->remove(index);
}

void AudioEncoderTableView::onResize()
{
    setColumnWidth(AudioEncoderModel::Property::ID, 50);
    setColumnWidth(AudioEncoderModel::Property::SourceInfo, this->width() / 5 - 10);
    setColumnWidth(AudioEncoderModel::Property::ChannelLayout, 100);
    setColumnWidth(AudioEncoderModel::Property::Bitrate, 100);
    setColumnWidth(AudioEncoderModel::Property::SampleRate, 100);
    setColumnWidth(AudioEncoderModel::Property::Crf, 50);
    setColumnWidth(AudioEncoderModel::Property::Profile, 100);
    setColumnWidth(AudioEncoderModel::Property::Remove, 50);

    auto width = this->width() - columnWidth(AudioEncoderModel::Property::ID)
                 - columnWidth(AudioEncoderModel::Property::SourceInfo)
                 - columnWidth(AudioEncoderModel::Property::ChannelLayout)
                 - columnWidth(AudioEncoderModel::Property::Bitrate)
                 - columnWidth(AudioEncoderModel::Property::SampleRate)
                 - columnWidth(AudioEncoderModel::Property::Crf)
                 - columnWidth(AudioEncoderModel::Property::Profile) - 50 - 10;
    setColumnWidth(AudioEncoderModel::Property::Encoder, width);
}

void AudioEncoderTableView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    QMetaObject::invokeMethod(this, &AudioEncoderTableView::onResize, Qt::QueuedConnection);
}

void AudioEncoderTableView::setupUI()
{
    setModel(d_ptr->model);

    setShowGrid(true);
    setWordWrap(false);
    setAlternatingRowColors(true);
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(35);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultSectionSize(120);
    horizontalHeader()->setMinimumSectionSize(60);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    setIconSize(QSize(20, 20));

    setItemDelegateForColumn(AudioEncoderModel::Property::Encoder, new AudioEncoderDelegate(this));
    setItemDelegateForColumn(AudioEncoderModel::Property::ChannelLayout,
                             new ChannelLayoutDelegate(this));
    setItemDelegateForColumn(AudioEncoderModel::Property::SampleRate, new SampleRateDelegate(this));
    setItemDelegateForColumn(AudioEncoderModel::Property::Profile, new ProfileDelegate(this));
    auto *removedDelegate = new RemovedDelegate(this);
    connect(removedDelegate, &RemovedDelegate::removed, this, &AudioEncoderTableView::onRemoved);
    setItemDelegateForColumn(AudioEncoderModel::Property::Remove, removedDelegate);
}

void AudioEncoderTableView::buildConnect()
{
    connect(
        d_ptr->model,
        &AudioEncoderModel::dataChanged,
        this,
        [this] { update(); },
        Qt::QueuedConnection);
}
