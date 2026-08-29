#ifndef VOICETRACKDIALOG_H
#define VOICETRACKDIALOG_H

#include <QDialog>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

#include "../audio/VoiceDuck.h"

class CueBus;
class FxPlayer;
class LevelMeter;
class VoiceRecorder;
class WaveformStore;

class QAudioOutput;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimer;

/**
 * @brief The two songs and the link, laid out on one timeline around the join.
 *
 * Time zero is the moment the OUTGOING song ends, because that is the only
 * fixed point everything else is measured from: the incoming song already
 * starts `joinOverlap` ms before it (the crossfade the operator prepared in
 * the wave view), and the link starts `lead` ms before it.
 *
 * Three lanes, top to bottom: the outgoing song's tail, the incoming song's
 * head, and the take. The take lane is the one that moves — dragged with the
 * mouse or nudged with the arrow keys — and moving it changes nothing about
 * where the two songs sit. That is the whole point: the presenter slides
 * their link around inside a join they have already set up.
 */
class VoiceTrackTimeline : public QWidget
{
    Q_OBJECT

public:
    explicit VoiceTrackTimeline(WaveformStore *store, QWidget *parent = nullptr);

    void setJoin(const QString &prevPath, const QString &nextPath, qint64 joinOverlapMs);
    void setTake(const QString &takePath, qint64 takeDurationMs);
    void setLeadMs(qint64 leadMs);
    qint64 leadMs() const { return m_leadMs; }
    /** Largest lead the outgoing song can carry (its length, capped by the
     *  wave view's Max overlap setting). */
    qint64 maxLeadMs() const;

    /** The ducking to draw over the two songs. Purely presentational — the
     *  dialog owns the real envelopes. */
    void setEnvelopes(const QVector<QPointF> &prevEnv, const QVector<QPointF> &nextEnv);

    /** Playhead in timeline coordinates (ms, 0 = the outgoing song's end).
     *  Pass the minimum qint64 to hide it. */
    void setPlayhead(qint64 timelineMs, bool visible);

    /** Live level bars drawn into the take lane while recording. */
    void setRecording(bool on, qint64 recordedMs);

    /** Where the link sits, phrased for speech. This is the one piece of
     *  state on this widget with no non-visual equivalent, so it is public:
     *  the dialog says it after every nudge and after every take. */
    QString positionSummary() const;

    QSize sizeHint() const override;

signals:
    void leadMsChanged(qint64 leadMs);
    /** A nudge that should be spoken (arrow keys, drag release). */
    void nudgeAnnounced(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    qint64 windowStartMs() const;
    qint64 windowEndMs() const;
    int xForMs(qint64 ms) const;
    qint64 msForX(int x) const;
    QRect laneRect(int lane) const; // 0 = outgoing, 1 = incoming, 2 = take
    void applyLead(qint64 leadMs, bool announce);

    WaveformStore *m_store = nullptr;
    QString m_prevPath;
    QString m_nextPath;
    QString m_takePath;
    qint64 m_joinOverlapMs = 0;
    qint64 m_takeDurationMs = 0;
    qint64 m_leadMs = 0;

    QVector<QPointF> m_prevEnv;
    QVector<QPointF> m_nextEnv;

    qint64 m_playheadMs = 0;
    bool m_playheadVisible = false;
    bool m_recording = false;
    qint64 m_recordedMs = 0;

    bool m_dragging = false;
    int m_dragStartX = 0;
    qint64 m_dragStartLead = 0;
};

/**
 * @brief Record the link over a join, and let XFB write the ducking.
 *
 * The flow this dialog exists to support, in the presenter's words: pick the
 * join between two songs, hear the end of one and the start of the next,
 * talk across it, hear it back in place, keep it.
 *
 * What comes out is deliberately ordinary. The take is a file, and it goes
 * into the playlist as a track like any other, positioned by the same
 * OverlapRole the wave view has always used for crossfades. The ducking is a
 * plain volume line (VolumeEnvelopeRole) on the two songs — the operator can
 * drag it afterwards, and it saves and loads with the playlist XML because
 * nothing about it is new. An older XFB opening the same playlist plays the
 * link as a track and honours the ducking without knowing what a voice track
 * is.
 *
 * Built in code with no .ui file: the committed ui_*.h headers in src/ shadow
 * AUTOUIC, so a .ui here would be a silent staleness trap.
 *
 * ## Accessibility
 * A blind presenter is an intended user of this, not an afterthought. Every
 * control is named for a screen reader, the whole flow works from the
 * keyboard (the timeline itself is a focusable control the arrow keys nudge),
 * and the things that have no visual substitute — the take started, the take
 * stopped, where the link now sits relative to the join — are announced
 * through announcementRequested(), which player::announceAccessible() routes
 * to the screen reader and to the cue ear.
 */
class VoiceTrackDialog : public QDialog
{
    Q_OBJECT

public:
    /** What the operator kept. Empty takePath = nothing to insert. */
    struct Result
    {
        QString takePath;
        qint64 takeDurationMs = 0;
        /** The take's overlap against the OUTGOING song: how long before that
         *  song ends the link starts. Goes straight into OverlapRole. */
        qint64 leadMs = 0;
        /** The incoming song's overlap against the TAKE, recomputed so the
         *  music join itself does not move when the link is nudged. */
        qint64 nextOverlapMs = 0;
        /** Encoded volume lines. Empty = leave that track's line alone. */
        QString prevEnvelope;
        QString nextEnvelope;
    };

    VoiceTrackDialog(const QString &prevPath, const QString &nextPath,
                     qint64 joinOverlapMs, WaveformStore *store,
                     CueBus *cueBus, QWidget *parent = nullptr);
    ~VoiceTrackDialog() override;

    Result result() const { return m_result; }

signals:
    void announcementRequested(const QString &message);

private slots:
    void toggleRecord();
    void retake();
    void togglePlayback();
    void regenerateDuck();
    void onLeadChanged(qint64 leadMs);

private:
    void buildUi();
    void refreshWaveforms();
    void updateEnabledState();
    void updateSummary();
    void startMonitor();
    void stopMonitor();
    void monitorTick();
    void playbackTick();
    void stopPlayback();
    void analyseTake();
    void applyResult();
    qint64 prevDurationMs() const;
    qint64 nextDurationMs() const;
    /** The incoming song's overlap against the take for the current lead. */
    qint64 nextOverlapForLead(qint64 leadMs) const;
    VoiceDuck::Params duckParams() const;
    void announce(const QString &message);

    // Context
    QString m_prevPath;
    QString m_nextPath;
    qint64 m_joinOverlapMs = 0;
    WaveformStore *m_store = nullptr;
    CueBus *m_cueBus = nullptr;

    // The take
    VoiceRecorder *m_recorder = nullptr;
    QString m_takePath;
    qint64 m_takeDurationMs = 0;
    QVector<VoiceDuck::Span> m_spans;
    bool m_takeAnalysed = false;
    /** The operator dragged the line by hand: do not overwrite it on a nudge. */
    bool m_prevEnvEdited = false;
    bool m_nextEnvEdited = false;
    QVector<QPointF> m_prevEnv;
    QVector<QPointF> m_nextEnv;

    Result m_result;

    // Widgets
    VoiceTrackTimeline *m_timeline = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QPushButton *m_recordButton = nullptr;
    QPushButton *m_retakeButton = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_regenButton = nullptr;
    LevelMeter *m_meter = nullptr;
    QLabel *m_elapsed = nullptr;
    QLabel *m_monitorNote = nullptr;
    QLabel *m_summary = nullptr;
    QCheckBox *m_monitorMain = nullptr;
    QCheckBox *m_opus = nullptr;
    QSpinBox *m_leadSpin = nullptr;
    QSpinBox *m_duckSpin = nullptr;
    QSpinBox *m_fadeSpin = nullptr;
    QSpinBox *m_holdSpin = nullptr;
    QDialogButtonBox *m_buttons = nullptr;

    // Monitoring while recording (cue bus) and audition afterwards (local)
    QTimer *m_monitorTimer = nullptr;
    bool m_monitorOnNext = false;
    bool m_monitorViaCue = false;

    QTimer *m_playTimer = nullptr;
    qint64 m_playT0Ms = 0;      // timeline ms at which the audition started
    qint64 m_playStartedAt = 0; // wall clock (ms since epoch)
    bool m_playTakeStarted = false;
    bool m_playNextStarted = false;
    FxPlayer *m_prevPlayer = nullptr;
    FxPlayer *m_takePlayer = nullptr;
    FxPlayer *m_nextPlayer = nullptr;
    QAudioOutput *m_prevOut = nullptr;
    QAudioOutput *m_takeOut = nullptr;
    QAudioOutput *m_nextOut = nullptr;
};

#endif // VOICETRACKDIALOG_H
