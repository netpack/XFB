#ifndef AUDIOFXDIALOG_H
#define AUDIOFXDIALOG_H

#include <QDialog>
#include <QWidget>

#include "../audio/FxParams.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QSlider;
class QTabWidget;
class FxPlayer;

/**
 * @brief One tab of FX controls (EQ + compressor) for a single player channel.
 */
class FxChannelPanel : public QWidget
{
    Q_OBJECT

public:
    FxChannelPanel(const QString &channelKey, FxPlayer *targetPlayer, QWidget *parent = nullptr);

    /** Re-read this channel's parameters from xfb.conf into the controls. */
    void reloadFromSettings();

private slots:
    void applyPreset(int index);
    void applyAndSave();
    void resetChannel();

private:
    void loadIntoUi();

    QString m_channelKey;
    FxPlayer *m_player;
    FxParams m_params;
    bool m_loading = false;

    QCheckBox *m_eqEnable = nullptr;
    QComboBox *m_presetBox = nullptr;
    QSlider *m_bandSliders[FxParams::kBands] = {};
    QLabel *m_bandValueLabels[FxParams::kBands] = {};
    QSlider *m_preampSlider = nullptr;
    QLabel *m_preampValue = nullptr;

    QCheckBox *m_compEnable = nullptr;
    QDoubleSpinBox *m_threshold = nullptr;
    QDoubleSpinBox *m_ratio = nullptr;
    QDoubleSpinBox *m_attack = nullptr;
    QDoubleSpinBox *m_release = nullptr;
    QDoubleSpinBox *m_makeup = nullptr;
};

/**
 * @brief The Audio FX control surface: per-player EQ/compressor tabs.
 *        (The global 432 Hz retune switch lives in the Options dialog.)
 *
 * Reusable: embedded both in the AudioFxDialog (menu) and in the main
 * window's "Audio FX" tab next to the DJ tab. Changes apply to the live
 * players immediately and are persisted to xfb.conf.
 */
class AudioFxWidget : public QWidget
{
    Q_OBJECT

public:
    AudioFxWidget(FxPlayer *mainPlayer, FxPlayer *lp1, FxPlayer *lp2,
                  QWidget *parent = nullptr);

    /**
     * Re-read all settings into the controls. Called when the widget is
     * shown again, so edits made through another instance (dialog vs. tab)
     * are reflected.
     */
    void reloadFromSettings();

private:
    QTabWidget *m_tabs = nullptr;
    FxChannelPanel *m_panels[3] = {};
};

/**
 * @brief Modal wrapper around AudioFxWidget for the XFB menu entry.
 */
class AudioFxDialog : public QDialog
{
    Q_OBJECT

public:
    AudioFxDialog(FxPlayer *mainPlayer, FxPlayer *lp1, FxPlayer *lp2,
                  QWidget *parent = nullptr);
};

#endif // AUDIOFXDIALOG_H
