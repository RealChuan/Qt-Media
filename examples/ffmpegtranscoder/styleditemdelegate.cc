#include "styleditemdelegate.hpp"
#include "commonwidgets.hpp"

#include <ffmpeg/encodecontext.hpp>
#include <ffmpeg/ffmpegutils.hpp>

#include <QtWidgets>

AudioEncoderDelegate::AudioEncoderDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

auto AudioEncoderDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &,
                                        const QModelIndex &) const -> QWidget *
{
    static auto audioEncodercs = Ffmpeg::getCodecsInfo(AVMEDIA_TYPE_AUDIO, true);

    auto *comboBox = CommonWidgets::createComboBox(parent);
    for (const auto &codec : std::as_const(audioEncodercs)) {
        comboBox->addItem(codec.displayName, QVariant::fromValue(codec));
    }
    comboBox->model()->sort(0);
    return comboBox;
}

void AudioEncoderDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    comboBox->setCurrentText(index.data(Qt::EditRole).toString());
}

void AudioEncoderDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    model->setData(index, comboBox->currentData(), Qt::EditRole);
}
ChannelLayoutDelegate::ChannelLayoutDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

auto ChannelLayoutDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &,
                                         const QModelIndex &index) const -> QWidget *
{
    auto *comboBox = CommonWidgets::createComboBox(parent);
    auto data = index.data(Qt::UserRole).value<Ffmpeg::EncodeContext>();
    for (const auto &chLayout : std::as_const(data.chLayouts)) {
        comboBox->addItem(chLayout.channelName, chLayout.channel);
    }
    return comboBox;
}

void ChannelLayoutDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    comboBox->setCurrentText(index.data(Qt::EditRole).toString());
}

void ChannelLayoutDelegate::setModelData(QWidget *editor,
                                         QAbstractItemModel *model,
                                         const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    model->setData(index, comboBox->currentData(), Qt::EditRole);
}

ProfileDelegate::ProfileDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

auto ProfileDelegate::createEditor(QWidget *parent,
                                   const QStyleOptionViewItem &,
                                   const QModelIndex &index) const -> QWidget *
{
    auto *comboBox = CommonWidgets::createComboBox(parent);
    auto data = index.data(Qt::UserRole).value<Ffmpeg::EncodeContext>();
    for (const auto &profile : std::as_const(data.profiles)) {
        comboBox->addItem(profile.name, profile.profile);
    }
    return comboBox;
}

void ProfileDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    comboBox->setCurrentText(index.data(Qt::EditRole).toString());
}

void ProfileDelegate::setModelData(QWidget *editor,
                                   QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    model->setData(index, comboBox->currentData(), Qt::EditRole);
}

SampleRateDelegate::SampleRateDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

auto SampleRateDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &,
                                      const QModelIndex &index) const -> QWidget *
{
    auto *comboBox = CommonWidgets::createComboBox(parent);
    auto data = index.data(Qt::UserRole).value<Ffmpeg::EncodeContext>();
    for (const auto &sampleRate : std::as_const(data.sampleRates)) {
        comboBox->addItem(QString::number(sampleRate), sampleRate);
    }
    return comboBox;
}

void SampleRateDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    comboBox->setCurrentText(index.data(Qt::EditRole).toString());
}

void SampleRateDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const
{
    auto *comboBox = qobject_cast<QComboBox *>(editor);
    model->setData(index, comboBox->currentData(), Qt::EditRole);
}

RemovedDelegate::RemovedDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

auto RemovedDelegate::editorEvent(QEvent *event,
                                  QAbstractItemModel *model,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) -> bool
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *e = static_cast<QMouseEvent *>(event);
        if (e != nullptr) {
            emit removed(index);
            qDebug() << "remove";
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
