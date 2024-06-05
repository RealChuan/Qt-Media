#ifndef MPVLOGWINDOW_HPP
#define MPVLOGWINDOW_HPP

#include <QMainWindow>

namespace Mpv {

class MpvLogWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MpvLogWindow(QWidget *parent = nullptr);
    ~MpvLogWindow() override;

public slots:
    void onAppendLog(const QString &log);

private:
    class MpvLogWindowPrivate;
    QScopedPointer<MpvLogWindowPrivate> d_ptr;
};

} // namespace Mpv

#endif // MPVLOGWINDOW_HPP
