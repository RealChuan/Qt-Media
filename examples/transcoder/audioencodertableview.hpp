#ifndef AUDIOENCODERTABLEVIEW_HPP
#define AUDIOENCODERTABLEVIEW_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QTableView>

class AudioEncoderTableView : public QTableView
{
    Q_OBJECT
public:
    explicit AudioEncoderTableView(QWidget *parent = nullptr);
    ~AudioEncoderTableView() override;

    void setDatas(const Ffmpeg::EncodeContexts &encodeContexts);
    [[nodiscard]] auto datas() const -> Ffmpeg::EncodeContexts;

private slots:
    void onRemoved(const QModelIndex &index);
    void onResize();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void buildConnect();

    class AudioEncoderTableViewPrivate;
    QScopedPointer<AudioEncoderTableViewPrivate> d_ptr;
};

#endif // AUDIOENCODERTABLEVIEW_HPP
