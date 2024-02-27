#include "audioencodermodel.hpp"

#include <QApplication>
#include <QIcon>
#include <QStyle>

class AudioEncoderModel::AudioEncoderModelPrivate
{
public:
    explicit AudioEncoderModelPrivate(AudioEncoderModel *q)
        : q_ptr(q)
    {
        auto metaEnums = QMetaEnum::fromType<Property>();
        for (int i = 0; i < metaEnums.keyCount(); i++) {
            headers.append(metaEnums.key(i));
        }
    }

    AudioEncoderModel *q_ptr;

    Ffmpeg::EncodeContexts encodeContexts;
    QStringList headers;

    QIcon removeIcon = qApp->style()->standardIcon(QStyle::SP_TitleBarCloseButton);

    const int bitrateOffset = 1000;
};

AudioEncoderModel::AudioEncoderModel(QObject *parent)
    : QAbstractTableModel{parent}
    , d_ptr(new AudioEncoderModelPrivate(this))
{}

AudioEncoderModel::~AudioEncoderModel() = default;

auto AudioEncoderModel::rowCount(const QModelIndex &parent) const -> int
{
    Q_UNUSED(parent);
    return d_ptr->encodeContexts.size();
}

auto AudioEncoderModel::columnCount(const QModelIndex &parent) const -> int
{
    Q_UNUSED(parent);
    return d_ptr->headers.size();
}

auto AudioEncoderModel::data(const QModelIndex &index, int role) const -> QVariant
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
        case Property::Encoder: return data.codecInfo().displayName;
        case Property::ChannelLayout: return data.chLayout().channelName;
        case Property::Bitrate: return data.bitrate;
        case Property::SampleRate: return data.sampleRate;
        case Property::Crf: return data.crf;
        case Property::Profile: {
            auto profile = data.profile();
            if (profile.profile >= 0) {
                return profile.name;
            }
            return {};
        }
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

auto AudioEncoderModel::setData(const QModelIndex &index, const QVariant &value, int role) -> bool
{
    if (!index.isValid()) {
        return false;
    }

    auto row = index.row();
    auto col = index.column();

    auto &data = d_ptr->encodeContexts[row];
    switch (role) {
    case Qt::EditRole: {
        switch (col) {
        case Property::Encoder: {
            auto codec = value.value<Ffmpeg::CodecInfo>();
            if (data.codecInfo() != codec) {
                data.setEncoderName(codec.name);
                emit dataChanged(index, index);
            }
        } break;
        case Property::ChannelLayout: {
            auto channel = static_cast<AVChannel>(value.toLongLong());
            if (data.chLayout().channel != channel) {
                data.setChannel(channel);
                emit dataChanged(index, index);
            }
        } break;
        case Property::Bitrate:
            data.bitrate = value.toInt();
            data.minBitrate = qMax(0, data.bitrate - d_ptr->bitrateOffset);
            data.maxBitrate = data.bitrate + d_ptr->bitrateOffset;
            emit dataChanged(index, index);
            break;
        case Property::SampleRate:
            data.sampleRate = value.toInt();
            emit dataChanged(index, index);
            break;
        case Property::Crf:
            data.crf = value.toInt();
            emit dataChanged(index, index);
            break;
        case Property::Profile: {
            auto profile = value.toInt();
            if (data.profile().profile != profile) {
                data.setProfile(profile);
                emit dataChanged(index, index);
            }
        } break;
        default: break;
        }
    } break;
    default: break;
    }

    return false;
}

auto AudioEncoderModel::flags(const QModelIndex &index) const -> Qt::ItemFlags
{
    if (!index.isValid()) {
        return {};
    }
    auto flags = QAbstractTableModel::flags(index);
    switch (index.column()) {
    case Property::ID:
    case Property::SourceInfo:
    case Property::Remove: break;
    default: flags |= Qt::ItemIsEditable; break;
    }
    return flags;
}

auto AudioEncoderModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void AudioEncoderModel::setDatas(const Ffmpeg::EncodeContexts &encodeContexts)
{
    beginResetModel();
    d_ptr->encodeContexts = encodeContexts;
    endResetModel();
}

auto AudioEncoderModel::datas() const -> Ffmpeg::EncodeContexts
{
    return d_ptr->encodeContexts;
}

void AudioEncoderModel::remove(const QModelIndex &index)
{
    beginRemoveRows(index.parent(), index.row(), index.row());
    d_ptr->encodeContexts.removeAt(index.row());
    endRemoveRows();
}
