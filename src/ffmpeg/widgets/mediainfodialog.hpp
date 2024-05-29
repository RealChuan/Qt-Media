#ifndef MEDIAINFODIALOG_HPP
#define MEDIAINFODIALOG_HPP

#include <ffmpeg/ffmepg_global.h>

#include <QDialog>

namespace Ffmpeg {

struct MediaInfo;
class FFMPEG_EXPORT MediaInfoDialog : public QDialog
{
public:
    explicit MediaInfoDialog(QWidget *parent = nullptr);
    ~MediaInfoDialog() override;

    void setMediaInfo(const Ffmpeg::MediaInfo &info);

private:
    class MediaInfoDialogPrivate;
    QScopedPointer<MediaInfoDialogPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // MEDIAINFODIALOG_HPP
