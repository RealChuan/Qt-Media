#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <ffmpeg/videooutputwidget.h>

using namespace Ffmpeg;

class PlayerWidgetPrivate;
class PlayerWidget : public VideoOutputWidget
{
    Q_OBJECT
public:
    explicit PlayerWidget(QWidget *parent = nullptr);
    ~PlayerWidget();

signals:
    void openFile(const QString &filepath);

private slots:
    void onOpenVideo();

protected:
    void contextMenuEvent(QContextMenuEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();

    QScopedPointer<PlayerWidgetPrivate> d_ptr;
};

#endif // PLAYERWIDGET_H
