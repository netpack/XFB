#include "DeckTempoControl.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include <cmath>

namespace
{
// Fader ranges, as on a DJ turntable or player: ±8 % for mixing, ±16 % and
// ±50 % for records a long way apart.
constexpr double kRanges[] = { 0.08, 0.16, 0.50 };
constexpr int kRangeCount = int(sizeof(kRanges) / sizeof(kRanges[0]));
// The fader moves in hundredths of a percent: fine enough that a synced
// tempo lands on its value rather than the nearest tenth.
constexpr double kUnitsPerRatio = 10000.0;
// How hard the nudge buttons push, as a fraction of the speed.
constexpr double kNudge = 0.03;
} // namespace

DeckTempoControl::DeckTempoControl(const QString &deckName, QWidget *parent)
    : QWidget(parent)
    , m_deckName(deckName)
{
    auto *column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(2);

    auto *readout = new QHBoxLayout;
    m_bpmLabel = new QLabel(this);
    m_percentLabel = new QLabel(this);
    m_percentLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    readout->addWidget(m_bpmLabel, 1);
    readout->addWidget(m_percentLabel);
    column->addLayout(readout);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setSingleStep(5);  // 0.05 %
    m_slider->setPageStep(100);  // 1 %
    m_slider->setTickPosition(QSlider::TicksBelow);
    m_slider->setAccessibleName(tr("%1 tempo").arg(m_deckName));
    m_slider->setToolTip(tr("Tempo: faster to the right. As on a turntable, "
                            "the pitch moves with the speed."));
    column->addWidget(m_slider);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(2);
    m_slowerButton = new QPushButton(QStringLiteral("−"), this);
    m_fasterButton = new QPushButton(QStringLiteral("+"), this);
    for (QPushButton *b : { m_slowerButton, m_fasterButton }) {
        b->setFixedWidth(26);
        b->setAutoRepeat(false);
    }
    m_slowerButton->setAccessibleName(tr("%1: hold to slow down").arg(m_deckName));
    m_fasterButton->setAccessibleName(tr("%1: hold to speed up").arg(m_deckName));
    m_slowerButton->setToolTip(tr("Hold to slow the record down a little, "
                                  "to bring its beat back in line"));
    m_fasterButton->setToolTip(tr("Hold to push the record a little faster, "
                                  "to bring its beat back in line"));

    m_resetButton = new QPushButton(QStringLiteral("0"), this);
    m_resetButton->setFixedWidth(26);
    m_resetButton->setAccessibleName(tr("%1: reset the tempo").arg(m_deckName));
    m_resetButton->setToolTip(tr("Back to the speed the record was made at"));

    m_rangeBox = new QComboBox(this);
    for (const double r : kRanges)
        m_rangeBox->addItem(tr("±%1 %").arg(qRound(r * 100)));
    m_rangeBox->setAccessibleName(tr("%1 tempo range").arg(m_deckName));
    m_rangeBox->setToolTip(tr("How far the tempo fader reaches either side"));

    m_syncButton = new QPushButton(tr("Sync"), this);
    m_syncButton->setAccessibleName(tr("%1: match the other deck's tempo").arg(m_deckName));
    m_syncButton->setToolTip(tr("Set this deck's tempo so its BPM matches the other "
                                "deck's. Lines up the tempo, not the beats: bring "
                                "the beats together with − and +."));

    buttons->addWidget(m_slowerButton);
    buttons->addWidget(m_fasterButton);
    buttons->addWidget(m_resetButton);
    buttons->addWidget(m_rangeBox, 1);
    buttons->addWidget(m_syncButton);
    column->addLayout(buttons);

    applyRange(0, false);
    refreshLabels();

    connect(m_slider, &QSlider::valueChanged, this, &DeckTempoControl::onSliderMoved);
    connect(m_rangeBox, &QComboBox::currentIndexChanged, this,
            [this](int index) { applyRange(index, true); });
    connect(m_resetButton, &QPushButton::clicked, this, &DeckTempoControl::resetTempo);
    connect(m_syncButton, &QPushButton::clicked, this, &DeckTempoControl::syncRequested);
    connect(m_slowerButton, &QPushButton::pressed, this, [this]() { setNudge(-1); });
    connect(m_fasterButton, &QPushButton::pressed, this, [this]() { setNudge(1); });
    connect(m_slowerButton, &QPushButton::released, this, [this]() { setNudge(0); });
    connect(m_fasterButton, &QPushButton::released, this, [this]() { setNudge(0); });
}

double DeckTempoControl::range() const
{
    return kRanges[qBound(0, m_rangeBox->currentIndex(), kRangeCount - 1)];
}

void DeckTempoControl::applyRange(int index, bool keepTempo)
{
    const int units = qRound(kRanges[qBound(0, index, kRangeCount - 1)] * kUnitsPerRatio);
    const double tempo = m_tempo;
    const QSignalBlocker block(m_slider);
    m_slider->setRange(-units, units);
    m_slider->setTickInterval(units / 4);
    if (keepTempo) {
        // A narrower range than the tempo already set clamps it, and the
        // deck has to hear about that.
        m_slider->setValue(qRound((tempo - 1.0) * kUnitsPerRatio));
        onSliderMoved(m_slider->value());
    }
}

void DeckTempoControl::onSliderMoved(int value)
{
    const double tempo = 1.0 + value / kUnitsPerRatio;
    if (std::abs(tempo - m_tempo) < 1e-9)
        return;
    m_tempo = tempo;
    refreshLabels();
    emit tempoChanged(m_tempo);
}

void DeckTempoControl::setTempo(double ratio)
{
    while (std::abs(ratio - 1.0) > range() + 1e-9 && widenRange()) {
    }
    m_slider->setValue(qRound((ratio - 1.0) * kUnitsPerRatio));
}

void DeckTempoControl::setFaderPosition(double position)
{
    setTempo(1.0 + qBound(-1.0, position, 1.0) * range());
}

void DeckTempoControl::resetTempo()
{
    m_slider->setValue(0);
}

bool DeckTempoControl::widenRange()
{
    const int index = m_rangeBox->currentIndex();
    if (index + 1 >= kRangeCount)
        return false;
    m_rangeBox->setCurrentIndex(index + 1);
    return true;
}

void DeckTempoControl::setNudge(int direction)
{
    emit bendChanged(direction > 0 ? kNudge : direction < 0 ? -kNudge : 0.0);
}

void DeckTempoControl::setTrackBpm(double bpm)
{
    m_trackBpm = bpm > 0 ? bpm : 0.0;
    refreshLabels();
}

void DeckTempoControl::refreshLabels()
{
    const double percent = (m_tempo - 1.0) * 100.0;
    m_percentLabel->setText(QStringLiteral("%1%2 %")
                                .arg(percent >= 0.005 ? QStringLiteral("+") : QString())
                                .arg(percent, 0, 'f', 2));
    if (m_trackBpm > 0) {
        m_bpmLabel->setText(tr("BPM %1").arg(m_trackBpm * m_tempo, 0, 'f', 1));
        m_bpmLabel->setToolTip(tr("Recorded at %1 BPM").arg(m_trackBpm, 0, 'f', 1));
    } else {
        m_bpmLabel->setText(tr("BPM —"));
        m_bpmLabel->setToolTip(tr("The tempo of this record has not been measured"));
    }
    m_slider->setAccessibleDescription(
        m_trackBpm > 0 ? tr("%1 percent, %2 BPM").arg(percent, 0, 'f', 2)
                                                  .arg(m_trackBpm * m_tempo, 0, 'f', 1)
                       : tr("%1 percent").arg(percent, 0, 'f', 2));
}
