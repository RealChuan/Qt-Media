#ifndef PREVIEWWIDGET_HPP
#define PREVIEWWIDGET_HPP

#include "mpv_global.h"

#include <QWidget>

namespace Mpv {

class MPV_LIB_EXPORT PreviewWidget : public QWidget
{
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget() override;

    void startPreview(const QString &filepath, int timestamp);
    void clearAllTask();

private:
    void setupUI();

    class PreviewWidgetPrivate;
    QScopedPointer<PreviewWidgetPrivate> d_ptr;
};

} // namespace Mpv

#endif // PREVIEWWIDGET_HPP
