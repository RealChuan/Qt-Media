#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QOpenGLWidget>

class PlayerWidgetPrivate;
class PlayerWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit PlayerWidget(QWidget *parent = nullptr);
    ~PlayerWidget();

public slots:
    void onReadyRead(const QPixmap& pixmap);

signals:
    void openFile(const QString &filepath);

private slots:
    void onOpenVideo();

protected:
    void contextMenuEvent(QContextMenuEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();

    QScopedPointer<PlayerWidgetPrivate> d_ptr;
};

#endif // PLAYERWIDGET_H
