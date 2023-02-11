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

    void setUseGpu(bool useGpu);

    void setVolume(int value);
    int volume() const;

    void setAudioTracks(const QStringList &list);
    void setCurrentAudioTrack(const QString &track);
    void setSubTracks(const QStringList &list);
    void setCurrentSubTrack(const QString &track);

public slots:
    void onDurationChanged(double value);
    void onPositionChanged(double value);

signals:
    void seek(int value);
    void hoverPosition(int pos, int value);
    void leavePosition();
    void play(bool);
    void useGpu(bool);
    void volumeChanged(int value);
    void speedChanged(double);
    void audioTrackChanged(const QString &text);
    void subTrackChanged(const QString &text);
    void showList();

private slots:
    void onSpeedChanged();

private:
    void setupUI();
    void buildConnect();

    class ControlWidgetPrivate;
    QScopedPointer<ControlWidgetPrivate> d_ptr;
};

#endif // CONTROLWIDGET_HPP
