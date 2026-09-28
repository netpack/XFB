#ifndef DECKTEMPOCONTROL_H
#define DECKTEMPOCONTROL_H

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

/**
 * The tempo section of one DJ deck: a pitch fader, its range, a reset, two
 * nudge buttons and Sync, with the deck's tempo shown as a percentage and as
 * a BPM.
 *
 * It only holds the numbers and says when they change. What a tempo does to
 * the audio is FxPlayer's business; what Sync matches against is the
 * player's, which knows both decks. That keeps this usable from the MIDI
 * controller too: a fader on the desk and a fader on screen end up calling
 * the same setTempo().
 *
 * Varispeed, as on a turntable: the pitch moves with the speed.
 */
class DeckTempoControl : public QWidget
{
    Q_OBJECT

public:
    /** @param deckName  "Deck 1", for the accessible names. */
    DeckTempoControl(const QString &deckName, QWidget *parent = nullptr);

    /** Current tempo, 1.0 = as recorded. */
    double tempo() const { return m_tempo; }
    /** Largest deviation the fader reaches, as a fraction (0.08 = ±8 %). */
    double range() const;

    /** Tempo of the loaded record as measured, 0 when unknown. */
    void setTrackBpm(double bpm);
    double trackBpm() const { return m_trackBpm; }

    /** Fader position, -1 (slowest) .. +1 (fastest), in the current range.
     *  What a MIDI fader or knob sends. */
    void setFaderPosition(double position);

public slots:
    /** Moves the fader to @a ratio, widening the range if it is outside it. */
    void setTempo(double ratio);
    void resetTempo();
    /** Widens the range one step (8 → 16 → 50 %); false when already widest. */
    bool widenRange();
    /** A push on the platter edge: +1 faster, -1 slower, 0 lets go. */
    void setNudge(int direction);

signals:
    void tempoChanged(double ratio);
    /** Temporary push on top of the tempo, as a fraction; 0 = let go. */
    void bendChanged(double fraction);
    void syncRequested();

private:
    void applyRange(int index, bool keepTempo);
    void onSliderMoved(int value);
    void refreshLabels();

    QString m_deckName;
    double m_tempo = 1.0;
    double m_trackBpm = 0.0;
    QSlider *m_slider = nullptr;
    QComboBox *m_rangeBox = nullptr;
    QLabel *m_percentLabel = nullptr;
    QLabel *m_bpmLabel = nullptr;
    QPushButton *m_resetButton = nullptr;
    QPushButton *m_syncButton = nullptr;
    QPushButton *m_slowerButton = nullptr;
    QPushButton *m_fasterButton = nullptr;
};

#endif // DECKTEMPOCONTROL_H
