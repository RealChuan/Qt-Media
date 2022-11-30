#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <ffmpeg/videorender/openglrender.hpp>

class PlayerWidget : public Ffmpeg::OpenglRender
{
    Q_OBJECT
public:
    explicit PlayerWidget(QWidget *parent = nullptr);
    ~PlayerWidget();

signals:
    void openFile(const QString &filepath);

private slots:
    void onOpenLocalMedia();
    void onOpenWebMedia();

protected:
    void contextMenuEvent(QContextMenuEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();

    class PlayerWidgetPrivate;
    QScopedPointer<PlayerWidgetPrivate> d_ptr;
};

#endif // PLAYERWIDGET_H
