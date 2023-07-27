#ifndef CONTROLWIDGET_HPP
#define CONTROLWIDGET_HPP

#include <QWidget>

class ControlWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ControlWidget(QWidget *parent = nullptr);
    ~ControlWidget();

    int position() const;
    int duration() const;
    QPoint sliderGlobalPos() const;

    void setSourceFPS(float fps);
    void setCurrentFPS(float fps);

    void clickPlayButton();
    void setPlayButtonChecked(bool checked);

    void setVolume(int value);
    int volume() const;

public slots:
    void onDurationChanged(double value);
    void onPositionChanged(double value);
    void onReadSpeedChanged(qint64 speed);

signals:
    void previous();
    void next();
    void seek(int value);
    void hoverPosition(int pos, int value);
    void leavePosition();
    void play(bool state);
    void volumeChanged(int value);
    void speedChanged(double value);
    void modelChanged(int model);
    void showList();

private slots:
    void onSpeedChanged();
    void onModelChanged();

private:
    void buildConnect();

    class ControlWidgetPrivate;
    QScopedPointer<ControlWidgetPrivate> d_ptr;
};

#endif // CONTROLWIDGET_HPP
