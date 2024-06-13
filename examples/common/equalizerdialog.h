#ifndef EQUALIZERDIALOG_H
#define EQUALIZERDIALOG_H

#include <mediaconfig/equalizer.hpp>

#include <QDialog>

class EqualizerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EqualizerDialog(QWidget *parent = nullptr);
    ~EqualizerDialog() override;

    void setEqualizer(const MediaConfig::Equalizer &equalizer);
    [[nodiscard]] auto equalizer() const -> MediaConfig::Equalizer;

signals:
    void equalizerChanged();

private slots:
    void onContrastSliderChanged(int value);
    void onContrastSpinBoxChanged(int value);

    void onSaturationSliderChanged(int value);
    void onSaturationSpinBoxChanged(int value);

    void onBrightnessSliderChanged(int value);
    void onBrightnessSpinBoxChanged(int value);

    void onGammaSliderChanged(int value);
    void onGammaSpinBoxChanged(int value);

    void onHueSliderChanged(int value);
    void onHueSpinBoxChanged(int value);

    void onReset();

private:
    void buildConnect();

    class EqualizerDialogPrivate;
    QScopedPointer<EqualizerDialogPrivate> d_ptr;
};

#endif // EQUALIZERDIALOG_H
