#ifndef SUBTITLEENCODERMODEL_HPP
#define SUBTITLEENCODERMODEL_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QAbstractTableModel>

class SubtitleEncoderModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Property { ID, SourceInfo, Burn, Remove };
    Q_ENUM(Property);

    explicit SubtitleEncoderModel(QObject *parent = nullptr);
    ~SubtitleEncoderModel() override;

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
    void append(const Ffmpeg::EncodeContexts &encodeContexts);
    [[nodiscard]] auto datas() const -> Ffmpeg::EncodeContexts;

    void remove(const QModelIndex &index);

    void setExclusive(int row);

private:
    class SubtitleEncoderModelPrivate;
    QScopedPointer<SubtitleEncoderModelPrivate> d_ptr;
};

#endif // SUBTITLEENCODERMODEL_HPP
