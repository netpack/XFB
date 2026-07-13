#ifndef PLAYLISTWAVEVIEW_H
#define PLAYLISTWAVEVIEW_H

#include <QPointF>
#include <QStyledItemDelegate>
#include <QVector>
#include <QWidget>

#include <functional>

class QAudioOutput;
class QContextMenuEvent;
class QListWidget;
class QMouseEvent;
class QPaintEvent;
class QTimer;
class FxPlayer;
class WaveformStore;
struct WaveformData;

/**
 * Optional "sound wave" presentation for the playlist, used to prepare
 * crossfades between consecutive tracks.
 *
 * When active, every playlist row shows:
 *  - the track's full waveform (with a marker where the *next* track will
 *    come in, when that one defines an overlap), and
 *  - a zoomed transition strip: the tail of the previous track (ghosted)
 *    and the head of this track on a shared time axis. Dragging this
 *    track's wave horizontally sets how many seconds before the end of
 *    the previous track it should start (the crossfade overlap).
 *
 * The overlap is stored on each playlist item under OverlapRole, saved
 * into playlist XML files, and honoured by the segue playback in
 * player.cpp. The small play button in the strip auditions just the
 * transition on dedicated preview players, without touching the on-air
 * player.
 */
class PlaylistWaveView : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /** Item data role: overlap in ms with the end of the previous track. */
    static constexpr int OverlapRole = Qt::UserRole + 101;

    /** Item data role: volume line (envelope), encoded as "ms:gain;..." */
    static constexpr int VolumeEnvelopeRole = Qt::UserRole + 102;

    /** Longest overlap the transition strip supports — how early the next
     *  track can be dragged to start before the previous one ends. It is
     *  user-configurable (MaxOverlapSeconds in xfb.conf): tracks with a
     *  long silent tail need a bigger window than the 25 s default. */
    static qint64 maxOverlapMs();
    static void setMaxOverlapMs(qint64 ms);

    // Volume line helpers, shared with the playback code in player.cpp.
    // Points are (x = position ms, y = gain 0..1), kept sorted by x.
    static QVector<QPointF> parseEnvelope(const QString &encoded);
    static QString encodeEnvelope(const QVector<QPointF> &points);
    static double envelopeGainAt(const QVector<QPointF> &points, qint64 positionMs);

    PlaylistWaveView(QListWidget *list, WaveformStore *store, QObject *parent = nullptr);

    void setActive(bool active);
    bool isActive() const { return m_active; }

    /** Provider of the file currently playing on the main player; used as
     *  the "previous track" for the first playlist row. */
    void setNowPlayingProvider(std::function<QString()> provider);

    /** Repaint hook for external changes (e.g. the playing track changed). */
    void refresh();

    void stopPreview();

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct StripGeometry
    {
        QRect strip;      // whole transition strip
        QRect previewBtn; // audition button
        QRect wave;       // draggable wave area
        int prevEndX = 0; // x where the previous track ends
        double pxPerMs = 0.0;
    };

    StripGeometry stripGeometry(const QRect &itemRect) const;
    QRect fullWaveRect(const QRect &itemRect) const;
    QString previousTrackPath(int row) const;
    void applyRowSizeHints(int first, int last) const;
    void requestVisibleWaveforms();
    qint64 clampedOverlap(int row, qint64 overlapMs) const;
    void setRowOverlap(int row, qint64 overlapMs);

    void togglePreview(int row);
    void startPreview(int row);
    void previewTick();

    QListWidget *m_list = nullptr;
    WaveformStore *m_store = nullptr;
    std::function<QString()> m_nowPlaying;

    bool m_active = false;
    bool m_hadMouseTracking = false;

    // Overlap dragging
    int m_dragRow = -1;
    int m_dragStartX = 0;
    qint64 m_dragStartOverlap = 0;
    int m_pressedPreviewRow = -1;

    // Volume line editing (Sonar-style envelope over the full waveform)
    int m_envDragRow = -1;
    int m_envDragIndex = -1;
    int m_envHoverRow = -1;
    int m_envHoverIndex = -1;
    // Segment drag: grabbing the line between nodes moves them together
    int m_envSegRow = -1;
    int m_envSegFirst = -1;
    int m_envSegLast = -1;
    int m_envSegStartY = 0;
    QVector<double> m_envSegStartGains;

    // Transition audition (independent of the on-air player)
    FxPlayer *m_prevPlayer = nullptr;
    FxPlayer *m_nextPlayer = nullptr;
    QAudioOutput *m_prevOutput = nullptr;
    QAudioOutput *m_nextOutput = nullptr;
    QTimer *m_previewTimer = nullptr;
    int m_previewRow = -1;
    bool m_previewNextStarted = false;
    qint64 m_previewPrevDurMs = 0;
    qint64 m_previewOverlapMs = 0;
    QVector<QPointF> m_previewPrevEnv;
    QVector<QPointF> m_previewNextEnv;

    static qint64 s_maxOverlapMs;
};

/**
 * "Now playing" strip shown above the playlist while the wave view is on:
 * the playing track's waveform with a playhead and its volume line. The
 * playlist item is consumed when a track starts, so this is where the
 * user keeps seeing — and editing — the line of the track on air; edits
 * are pushed back to the player live through envelopeEdited().
 */
class NowPlayingWaveStrip : public QWidget
{
    Q_OBJECT

public:
    explicit NowPlayingWaveStrip(WaveformStore *store, QWidget *parent = nullptr);

    void setTrack(const QString &filePath);
    QString track() const { return m_path; }
    void setEnvelope(const QVector<QPointF> &points);
    void setPlayhead(qint64 positionMs);

signals:
    void envelopeEdited(const QVector<QPointF> &points);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRect waveRect() const;
    qint64 durationMs() const;

    WaveformStore *m_store = nullptr;
    QString m_path;
    QVector<QPointF> m_env;
    qint64 m_positionMs = 0;

    int m_dragNode = -1;
    int m_hoverNode = -1;
    int m_segFirst = -1;
    int m_segLast = -1;
    int m_segStartY = 0;
    QVector<double> m_segStartGains;
};

#endif // PLAYLISTWAVEVIEW_H
