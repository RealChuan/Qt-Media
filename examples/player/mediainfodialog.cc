#include "mediainfodialog.hpp"

#include <ffmpeg/mediainfo.hpp>

#include <QtWidgets>

class MediaInfoDialog::MediaInfoDialogPrivate
{
public:
    explicit MediaInfoDialogPrivate(MediaInfoDialog *q)
        : q_ptr(q)
    {
        textBrowser = new QTextBrowser(q_ptr);
        auto *layout = new QVBoxLayout(q_ptr);
        layout->addWidget(textBrowser);
    }

    MediaInfoDialog *q_ptr;
    QTextBrowser *textBrowser;
};

MediaInfoDialog::MediaInfoDialog(QWidget *parent)
    : QDialog(parent)
    , d_ptr(new MediaInfoDialogPrivate(this))
{
    resize(550, 600);
}

MediaInfoDialog::~MediaInfoDialog() = default;

void MediaInfoDialog::setMediaInfo(const Ffmpeg::MediaInfo &info)
{
    auto text = QString::fromUtf8(QJsonDocument(info.toJson()).toJson());
    d_ptr->textBrowser->setText(text);
}
