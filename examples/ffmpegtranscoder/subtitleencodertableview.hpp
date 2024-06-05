#ifndef SUBTITLEENCODERTABLEVIEW_HPP
#define SUBTITLEENCODERTABLEVIEW_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QTableView>

class SubtitleEncoderTableView : public QTableView
{
    Q_OBJECT
public:
    explicit SubtitleEncoderTableView(QWidget *parent = nullptr);
    ~SubtitleEncoderTableView() override;

    void setDatas(const Ffmpeg::EncodeContexts &encodeContexts);
    void append(const Ffmpeg::EncodeContexts &encodeContexts);
    [[nodiscard]] auto datas() const -> Ffmpeg::EncodeContexts;

private slots:
    void onDataChnged(const QModelIndex &topLeft,
                      const QModelIndex &bottomRight,
                      const QList<int> &roles);
    void onRemoved(const QModelIndex &index);
    void onResize();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void buildConnect();

    class SubtitleEncoderTableViewPrivate;
    QScopedPointer<SubtitleEncoderTableViewPrivate> d_ptr;
};

#endif // SUBTITLEENCODERTABLEVIEW_HPP
