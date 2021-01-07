#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QWidget>

class PlayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerWidget(QWidget *parent = nullptr);

public slots:
    void onReadyRead(const QPixmap& pixmap);

signals:
    void openFile(const QString &filepath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_pixmap;
};

#endif // PLAYERWIDGET_H
