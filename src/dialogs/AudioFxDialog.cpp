#include "AudioFxDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

#include "../audio/FxPlayer.h"

namespace
{
const char *kBandLabels[FxParams::kBands] = {
    "31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"
};

struct EqPreset
{
    const char *name;
    double gains[FxParams::kBands];
};

// Classic 10-band presets (dB)
const EqPreset kPresets[] = {
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Flat"),        { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Bass Boost"),  { 6,  5,  4,  2,  0,  0,  0,  0,  0,  0}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Treble Boost"),{ 0,  0,  0,  0,  0,  1,  2,  4,  5,  6}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Loudness"),    { 5,  4,  2,  0, -1,  0,  1,  3,  5,  4}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Vocal Presence"),{-2, -1,  0,  2,  4,  4,  3,  1,  0, -1}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Rock"),        { 4,  3,  1, -1, -2,  0,  2,  3,  4,  4}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Pop"),         {-1,  1,  3,  4,  3,  0, -1, -1,  1,  2}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Jazz"),        { 3,  2,  0,  1,  2,  2,  0,  1,  2,  3}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Classical"),   { 3,  2,  1,  0,  0,  0, -1,  1,  2,  3}},
    {QT_TRANSLATE_NOOP("FxChannelPanel", "Radio Punch"), { 2,  3,  2,  1,  0,  1,  3,  3,  2,  1}},
};
} // namespace

// ------------------------------------------------------------- FxChannelPanel

FxChannelPanel::FxChannelPanel(const QString &channelKey, FxPlayer *targetPlayer, QWidget *parent)
    : QWidget(parent)
    , m_channelKey(channelKey)
    , m_player(targetPlayer)
{
    m_params = FxSettings::loadChannel(m_channelKey);

    auto *mainLayout = new QVBoxLayout(this);

    // --- Equalizer group ---
    auto *eqGroup = new QGroupBox(tr("10-Band Equalizer"), this);
    auto *eqLayout = new QVBoxLayout(eqGroup);

    auto *eqTopRow = new QHBoxLayout();
    m_eqEnable = new QCheckBox(tr("Enable equalizer"), eqGroup);
    eqTopRow->addWidget(m_eqEnable);
    eqTopRow->addStretch();
    eqTopRow->addWidget(new QLabel(tr("Preset:"), eqGroup));
    m_presetBox = new QComboBox(eqGroup);
    for (const EqPreset &p : kPresets)
        m_presetBox->addItem(tr(p.name));
    eqTopRow->addWidget(m_presetBox);
    eqLayout->addLayout(eqTopRow);

    auto *slidersRow = new QHBoxLayout();

    // Preamp slider
    {
        auto *col = new QVBoxLayout();
        m_preampValue = new QLabel("0 dB", eqGroup);
        m_preampValue->setAlignment(Qt::AlignHCenter);
        m_preampSlider = new QSlider(Qt::Vertical, eqGroup);
        m_preampSlider->setRange(-12, 12);
        m_preampSlider->setTickPosition(QSlider::TicksBothSides);
        m_preampSlider->setTickInterval(6);
        m_preampSlider->setMinimumHeight(140);
        m_preampSlider->setAccessibleName(tr("Preamp gain"));
        auto *name = new QLabel(tr("Pre"), eqGroup);
        name->setAlignment(Qt::AlignHCenter);
        col->addWidget(m_preampValue);
        col->addWidget(m_preampSlider, 1, Qt::AlignHCenter);
        col->addWidget(name);
        slidersRow->addLayout(col);
    }

    auto *sep = new QFrame(eqGroup);
    sep->setFrameShape(QFrame::VLine);
    slidersRow->addWidget(sep);

    for (int i = 0; i < FxParams::kBands; ++i) {
        auto *col = new QVBoxLayout();
        m_bandValueLabels[i] = new QLabel("0", eqGroup);
        m_bandValueLabels[i]->setAlignment(Qt::AlignHCenter);
        m_bandSliders[i] = new QSlider(Qt::Vertical, eqGroup);
        m_bandSliders[i]->setRange(-12, 12);
        m_bandSliders[i]->setTickPosition(QSlider::TicksBothSides);
        m_bandSliders[i]->setTickInterval(6);
        m_bandSliders[i]->setMinimumHeight(140);
        m_bandSliders[i]->setAccessibleName(tr("%1 Hz band gain").arg(kBandLabels[i]));
        auto *name = new QLabel(QString::fromLatin1(kBandLabels[i]), eqGroup);
        name->setAlignment(Qt::AlignHCenter);
        col->addWidget(m_bandValueLabels[i]);
        col->addWidget(m_bandSliders[i], 1, Qt::AlignHCenter);
        col->addWidget(name);
        slidersRow->addLayout(col);
    }
    eqLayout->addLayout(slidersRow);
    mainLayout->addWidget(eqGroup);

    // --- Compressor group ---
    auto *compGroup = new QGroupBox(tr("Compressor"), this);
    auto *compLayout = new QVBoxLayout(compGroup);
    m_compEnable = new QCheckBox(tr("Enable compressor"), compGroup);
    compLayout->addWidget(m_compEnable);

    auto *form = new QFormLayout();
    m_threshold = new QDoubleSpinBox(compGroup);
    m_threshold->setRange(-60.0, 0.0);
    m_threshold->setSuffix(" dB");
    m_threshold->setSingleStep(1.0);
    form->addRow(tr("Threshold:"), m_threshold);

    m_ratio = new QDoubleSpinBox(compGroup);
    m_ratio->setRange(1.0, 20.0);
    m_ratio->setSuffix(" : 1");
    m_ratio->setSingleStep(0.5);
    form->addRow(tr("Ratio:"), m_ratio);

    m_attack = new QDoubleSpinBox(compGroup);
    m_attack->setRange(0.1, 200.0);
    m_attack->setSuffix(" ms");
    m_attack->setSingleStep(1.0);
    form->addRow(tr("Attack:"), m_attack);

    m_release = new QDoubleSpinBox(compGroup);
    m_release->setRange(10.0, 2000.0);
    m_release->setSuffix(" ms");
    m_release->setSingleStep(10.0);
    form->addRow(tr("Release:"), m_release);

    m_makeup = new QDoubleSpinBox(compGroup);
    m_makeup->setRange(0.0, 24.0);
    m_makeup->setSuffix(" dB");
    m_makeup->setSingleStep(0.5);
    form->addRow(tr("Makeup gain:"), m_makeup);
    compLayout->addLayout(form);
    mainLayout->addWidget(compGroup);

    // --- Reset row ---
    auto *bottomRow = new QHBoxLayout();
    auto *resetBtn = new QPushButton(tr("Reset this player to defaults"), this);
    bottomRow->addWidget(resetBtn);
    bottomRow->addStretch();
    mainLayout->addLayout(bottomRow);

    loadIntoUi();

    // Live-apply on any change
    connect(m_eqEnable, &QCheckBox::toggled, this, &FxChannelPanel::applyAndSave);
    connect(m_presetBox, QOverload<int>::of(&QComboBox::activated),
            this, &FxChannelPanel::applyPreset);
    connect(m_preampSlider, &QSlider::valueChanged, this, &FxChannelPanel::applyAndSave);
    for (int i = 0; i < FxParams::kBands; ++i)
        connect(m_bandSliders[i], &QSlider::valueChanged, this, &FxChannelPanel::applyAndSave);
    connect(m_compEnable, &QCheckBox::toggled, this, &FxChannelPanel::applyAndSave);
    for (QDoubleSpinBox *box : {m_threshold, m_ratio, m_attack, m_release, m_makeup})
        connect(box, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &FxChannelPanel::applyAndSave);
    connect(resetBtn, &QPushButton::clicked, this, &FxChannelPanel::resetChannel);
}

void FxChannelPanel::loadIntoUi()
{
    m_loading = true;
    m_eqEnable->setChecked(m_params.eqEnabled);
    m_preampSlider->setValue(static_cast<int>(m_params.preampDb));
    m_preampValue->setText(QString::number(static_cast<int>(m_params.preampDb)) + " dB");
    for (int i = 0; i < FxParams::kBands; ++i) {
        m_bandSliders[i]->setValue(static_cast<int>(m_params.eqGainDb[i]));
        m_bandValueLabels[i]->setText(QString::number(static_cast<int>(m_params.eqGainDb[i])));
    }
    m_compEnable->setChecked(m_params.compEnabled);
    m_threshold->setValue(m_params.compThresholdDb);
    m_ratio->setValue(m_params.compRatio);
    m_attack->setValue(m_params.compAttackMs);
    m_release->setValue(m_params.compReleaseMs);
    m_makeup->setValue(m_params.compMakeupDb);
    m_loading = false;
}

void FxChannelPanel::applyPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0])))
        return;
    m_loading = true;
    for (int i = 0; i < FxParams::kBands; ++i)
        m_bandSliders[i]->setValue(static_cast<int>(kPresets[index].gains[i]));
    m_eqEnable->setChecked(true);
    m_loading = false;
    applyAndSave();
}

void FxChannelPanel::applyAndSave()
{
    if (m_loading)
        return;

    // The 432 Hz switch lives in the Options dialog; always take the
    // current global value so an EQ tweak can never apply a stale one.
    m_params.retune432 = FxSettings::loadRetune432();

    m_params.eqEnabled = m_eqEnable->isChecked();
    m_params.preampDb = m_preampSlider->value();
    m_preampValue->setText(QString::number(m_preampSlider->value()) + " dB");
    for (int i = 0; i < FxParams::kBands; ++i) {
        m_params.eqGainDb[i] = m_bandSliders[i]->value();
        m_bandValueLabels[i]->setText(QString::number(m_bandSliders[i]->value()));
    }
    m_params.compEnabled = m_compEnable->isChecked();
    m_params.compThresholdDb = m_threshold->value();
    m_params.compRatio = m_ratio->value();
    m_params.compAttackMs = m_attack->value();
    m_params.compReleaseMs = m_release->value();
    m_params.compMakeupDb = m_makeup->value();

    FxSettings::saveChannel(m_channelKey, m_params);
    if (m_player)
        m_player->setFxParams(m_params);
}

void FxChannelPanel::resetChannel()
{
    m_params = FxParams();
    m_params.retune432 = FxSettings::loadRetune432(); // global flag is not reset here
    loadIntoUi();
    m_presetBox->setCurrentIndex(0);
    FxSettings::saveChannel(m_channelKey, m_params);
    if (m_player)
        m_player->setFxParams(m_params);
}

void FxChannelPanel::reloadFromSettings()
{
    m_params = FxSettings::loadChannel(m_channelKey);
    loadIntoUi();
}

// -------------------------------------------------------------- AudioFxWidget

AudioFxWidget::AudioFxWidget(FxPlayer *mainPlayer, FxPlayer *lp1, FxPlayer *lp2, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    if (!FxPlayer::fxAvailable()) {
        auto *warn = new QLabel(tr("⚠ ffmpeg was not found on this system — the live FX engine is disabled. "
                                   "Install ffmpeg (e.g. \"brew install ffmpeg\" on macOS or your distribution's "
                                   "package manager on Linux) and restart XFB."), this);
        warn->setWordWrap(true);
        warn->setStyleSheet("color:#b06000;");
        layout->addWidget(warn);
    }

    m_tabs = new QTabWidget(this);
    m_panels[0] = new FxChannelPanel(QStringLiteral("Main"), mainPlayer, this);
    m_panels[1] = new FxChannelPanel(QStringLiteral("LP1"), lp1, this);
    m_panels[2] = new FxChannelPanel(QStringLiteral("LP2"), lp2, this);
    m_tabs->addTab(m_panels[0], tr("Main Player"));
    m_tabs->addTab(m_panels[1], tr("LP Player 1"));
    m_tabs->addTab(m_panels[2], tr("LP Player 2"));
    layout->addWidget(m_tabs, 1);
}

void AudioFxWidget::reloadFromSettings()
{
    for (FxChannelPanel *panel : m_panels) {
        if (panel)
            panel->reloadFromSettings();
    }
}

// -------------------------------------------------------------- AudioFxDialog

AudioFxDialog::AudioFxDialog(FxPlayer *mainPlayer, FxPlayer *lp1, FxPlayer *lp2, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Audio FX — Equalizer, Compressor & 432 Hz"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new AudioFxWidget(mainPlayer, lp1, lp2, this), 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
