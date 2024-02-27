#include "subtitleencodermodel.hpp"

#include <QApplication>
#include <QIcon>
#include <QStyle>

class SubtitleEncoderModel::SubtitleEncoderModelPrivate
{
public:
    explicit SubtitleEncoderModelPrivate(SubtitleEncoderModel *q)
        : q_ptr(q)
    {
        auto metaEnums = QMetaEnum::fromType<Property>();
        for (int i = 0; i < metaEnums.keyCount(); i++) {
            headers.append(metaEnums.key(i));
        }
    }

    SubtitleEncoderModel *q_ptr;

    Ffmpeg::EncodeContexts encodeContexts;
    QStringList headers;

    QIcon removeIcon = qApp->style()->standardIcon(QStyle::SP_TitleBarCloseButton);
};

SubtitleEncoderModel::SubtitleEncoderModel(QObject *parent)
    : QAbstractTableModel{parent}
    , d_ptr(new SubtitleEncoderModelPrivate(this))
{}

SubtitleEncoderModel::~SubtitleEncoderModel() = default;

auto SubtitleEncoderModel::rowCount(const QModelIndex &parent) const -> int
{
    Q_UNUSED(parent);
    return d_ptr->encodeContexts.size();
}

auto SubtitleEncoderModel::columnCount(const QModelIndex &parent) const -> int
{
    Q_UNUSED(parent);
    return d_ptr->headers.size();
}

auto SubtitleEncoderModel::data(const QModelIndex &index, int role) const -> QVariant
{
    if (!index.isValid()) {
        return {};
    }

    auto row = index.row();
    auto col = index.column();

    const auto &data = d_ptr->encodeContexts.at(row);
    switch (role) {
    case Qt::TextAlignmentRole: return Qt::AlignCenter;
    case Qt::WhatsThisRole:
    case Qt::ToolTipRole:
    case Qt::DisplayRole:
    case Qt::EditRole: { //双击为空需添加
        switch (col) {
        case Property::ID: return data.streamIndex;
        case Property::SourceInfo: return data.sourceInfo;
        default: break;
        }
        break;
    case Qt::CheckStateRole:
        switch (col) {
        case Property::Burn: return data.burn ? Qt::Checked : Qt::Unchecked;
        default: break;
        }
        break;
    case Qt::DecorationRole:
        switch (col) {
        case Property::Remove: return d_ptr->removeIcon;
        default: break;
        }
        break;
    case Qt::UserRole: return QVariant::fromValue(data);
    }
    default: break;
    }
    return {};
}

auto SubtitleEncoderModel::setData(const QModelIndex &index, const QVariant &value, int role)
    -> bool
{
    if (!index.isValid()) {
        return false;
    }

    auto row = index.row();
    auto col = index.column();

    auto &data = d_ptr->encodeContexts[row];
    switch (role) {
    case Qt::CheckStateRole: {
        switch (col) {
        case Property::Burn:
            data.burn = value.toBool();
            emit dataChanged(index, index);
            break;
        default: break;
        }
    } break;
    default: break;
    }

    return false;
}

auto SubtitleEncoderModel::flags(const QModelIndex &index) const -> Qt::ItemFlags
{
    if (!index.isValid()) {
        return {};
    }
    auto flags = QAbstractTableModel::flags(index);
    switch (index.column()) {
    case Property::Burn: flags |= Qt::ItemIsUserCheckable; break;
    default: break;
    }
    return flags;
}

auto SubtitleEncoderModel::headerData(int section, Qt::Orientation orientation, int role) const
    -> QVariant
{
    if (section < 0 || section >= d_ptr->headers.size() || orientation != Qt::Horizontal) {
        return {};
    }
    switch (role) {
    case Qt::TextAlignmentRole: return Qt::AlignCenter;
    case Qt::WhatsThisRole:
    case Qt::ToolTipRole:
    case Qt::DisplayRole: return d_ptr->headers.at(section);
    default: break;
    }
    return {};
}

void SubtitleEncoderModel::setDatas(const Ffmpeg::EncodeContexts &encodeContexts)
{
    beginResetModel();
    d_ptr->encodeContexts = encodeContexts;
    endResetModel();
}

void SubtitleEncoderModel::append(const Ffmpeg::EncodeContexts &encodeContexts)
{
    beginInsertRows({},
                    d_ptr->encodeContexts.size(),
                    d_ptr->encodeContexts.size() + encodeContexts.size() - 1);
    d_ptr->encodeContexts.append(encodeContexts);
    endInsertRows();
}

auto SubtitleEncoderModel::datas() const -> Ffmpeg::EncodeContexts
{
    return d_ptr->encodeContexts;
}

void SubtitleEncoderModel::remove(const QModelIndex &index)
{
    beginRemoveRows(index.parent(), index.row(), index.row());
    d_ptr->encodeContexts.removeAt(index.row());
    endRemoveRows();
}

void SubtitleEncoderModel::setExclusive(int row)
{
    beginResetModel();
    for (int i = 0; i < d_ptr->encodeContexts.size(); i++) {
        if (i != row) {
            d_ptr->encodeContexts[i].burn = false;
        }
    }
    endResetModel();
}
