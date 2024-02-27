#ifndef PREVIEWWIDGET_HPP
#define PREVIEWWIDGET_HPP

#include <QWidget>

#include <vector>

namespace Ffmpeg {
class Frame;
};

class PreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget() override;

    void setFrames(const std::vector<QSharedPointer<Ffmpeg::Frame>> &framePtrs);

private slots:
    void onPerFrame();
    void onNextFrame();

private:
    void setupUI();
    void buildConnect();

    class PreviewWidgetPrivate;
    QScopedPointer<PreviewWidgetPrivate> d_ptr;
};

#endif // PREVIEWWIDGET_HPP
