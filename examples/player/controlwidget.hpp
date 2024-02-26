#ifndef CONTROLWIDGET_HPP
#define CONTROLWIDGET_HPP

#include <ffmpeg/mediainfo.hpp>

#include <QWidget>

class ControlWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ControlWidget(QWidget *parent = nullptr);
    ~ControlWidget() override;

    void setPosition(int value);
    [[nodiscard]] auto position() const -> int;

    void setDuration(int value);
    [[nodiscard]] auto duration() const -> int;
    void setChapters(const Ffmpeg::Chapters &chapters);

    [[nodiscard]] auto sliderGlobalPos() const -> QPoint;

    void setSourceFPS(float fps);
    void setCurrentFPS(float fps);

    void clickPlayButton();
    void setPlayButtonChecked(bool checked);

    void setVolume(int value);
    [[nodiscard]] auto volume() const -> int;

    void setCacheSpeed(qint64 speed);

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
