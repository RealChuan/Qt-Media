#include "subtitleencodertableview.hpp"
#include "styleditemdelegate.hpp"
#include "subtitleencodermodel.hpp"

#include <QHeaderView>

class SubtitleEncoderTableView::SubtitleEncoderTableViewPrivate
{
public:
    explicit SubtitleEncoderTableViewPrivate(SubtitleEncoderTableView *q)
        : q_ptr(q)
    {
        model = new SubtitleEncoderModel(q_ptr);
    }

    SubtitleEncoderTableView *q_ptr;

    SubtitleEncoderModel *model;
};

SubtitleEncoderTableView::SubtitleEncoderTableView(QWidget *parent)
    : QTableView{parent}
    , d_ptr(new SubtitleEncoderTableViewPrivate(this))
{
    setupUI();
    buildConnect();
}

SubtitleEncoderTableView::~SubtitleEncoderTableView() = default;

void SubtitleEncoderTableView::setDatas(const Ffmpeg::EncodeContexts &encodeContexts)
{
    d_ptr->model->setDatas(encodeContexts);
}

void SubtitleEncoderTableView::append(const Ffmpeg::EncodeContexts &encodeContexts)
{
    d_ptr->model->append(encodeContexts);
}

auto SubtitleEncoderTableView::datas() const -> Ffmpeg::EncodeContexts
{
    return d_ptr->model->datas();
}

void SubtitleEncoderTableView::onRemoved(const QModelIndex &index)
{
    d_ptr->model->remove(index);
}

void SubtitleEncoderTableView::onDataChnged(const QModelIndex &topLeft,
                                            const QModelIndex &bottomRight,
                                            const QList<int> &roles)
{
    if (topLeft.column() == SubtitleEncoderModel::Property::Burn) {
        d_ptr->model->setExclusive(topLeft.row());
    }
    update();
}

void SubtitleEncoderTableView::onResize()
{
    setColumnWidth(SubtitleEncoderModel::Property::ID, 50);
    setColumnWidth(SubtitleEncoderModel::Property::Burn, 50);
    setColumnWidth(SubtitleEncoderModel::Property::Remove, 50);

    auto width = this->width() - columnWidth(SubtitleEncoderModel::Property::ID)
                 - columnWidth(SubtitleEncoderModel::Property::Burn) - 50 - 10;
    setColumnWidth(SubtitleEncoderModel::Property::SourceInfo, width);
}

void SubtitleEncoderTableView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    QMetaObject::invokeMethod(this, &SubtitleEncoderTableView::onResize, Qt::QueuedConnection);
}

void SubtitleEncoderTableView::setupUI()
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

    auto *removedDelegate = new RemovedDelegate(this);
    connect(removedDelegate, &RemovedDelegate::removed, this, &SubtitleEncoderTableView::onRemoved);
    setItemDelegateForColumn(SubtitleEncoderModel::Property::Remove, removedDelegate);
}

void SubtitleEncoderTableView::buildConnect()
{
    connect(d_ptr->model,
            &SubtitleEncoderModel::dataChanged,
            this,
            &SubtitleEncoderTableView::onDataChnged,
            Qt::QueuedConnection);
}
