#ifndef AUDIOENCODERMODEL_HPP
#define AUDIOENCODERMODEL_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QAbstractTableModel>

class AudioEncoderModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Property {
        ID,
        SourceInfo,
        Encoder,
        ChannelLayout,
        Bitrate,
        SampleRate,
        Crf,
        Profile,
        Remove
    };
    Q_ENUM(Property);

    explicit AudioEncoderModel(QObject *parent = nullptr);
    ~AudioEncoderModel() override;

    [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int override;
    [[nodiscard]] auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int override;

    [[nodiscard]] auto data(const QModelIndex &index, int role = Qt::DisplayRole) const
        -> QVariant override;
    auto setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
        -> bool override;

    [[nodiscard]] auto flags(const QModelIndex &index) const -> Qt::ItemFlags override;

    [[nodiscard]] auto headerData(int section,
                                  Qt::Orientation orientation,
                                  int role = Qt::DisplayRole) const -> QVariant override;

    void setDatas(const Ffmpeg::EncodeContexts &encodeContexts);
    [[nodiscard]] auto datas() const -> Ffmpeg::EncodeContexts;

    void remove(const QModelIndex &index);

private:
    class AudioEncoderModelPrivate;
    QScopedPointer<AudioEncoderModelPrivate> d_ptr;
};

#endif // AUDIOENCODERMODEL_HPP
