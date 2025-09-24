#pragma once

#include <ffmpeg/frame.hpp>

#include <QWidget>

class PreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget() override;

    void setFrames(const Ffmpeg::FramePtrList &framePtrs);

private slots:
    void onPerFrame();
    void onNextFrame();

private:
    void setupUI();
    void buildConnect();

    class PreviewWidgetPrivate;
    QScopedPointer<PreviewWidgetPrivate> d_ptr;
};
