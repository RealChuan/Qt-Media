#ifndef MEDIAINFODIALOG_HPP
#define MEDIAINFODIALOG_HPP

#include <QDialog>

namespace Ffmpeg {
struct MediaInfo;
}

class MediaInfoDialog : public QDialog
{
public:
    explicit MediaInfoDialog(QWidget *parent = nullptr);
    ~MediaInfoDialog() override;

    void setMediaInfo(const Ffmpeg::MediaInfo &info);

private:
    class MediaInfoDialogPrivate;
    QScopedPointer<MediaInfoDialogPrivate> d_ptr;
};

#endif // MEDIAINFODIALOG_HPP
