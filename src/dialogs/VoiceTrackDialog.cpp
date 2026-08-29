#include "VoiceTrackDialog.h"

#include "../LevelMeter.h"
#include "../PlaylistWaveView.h"
#include "../audio/CueBus.h"
#include "../audio/FxPlayer.h"
#include "../audio/VoiceRecorder.h"
#include "../audio/WaveformStore.h"

#include <QAudioOutput>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDebug>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
constexpr int kLaneHeight = 46;
constexpr int kLaneGap = 8;
constexpr int kSideMargin = 10;
constexpr int kTopMargin = 16;
constexpr double kAuditionVolume = 0.9;

/** How much air to leave either side of the busy part of the timeline. */
constexpr qint64 kPadMs = 3000;

QString formatMs(qint64 ms)
{
    const bool negative = ms < 0;
    const qint64 a = qAbs(ms);
    const qint64 secs = a / 1000;
    return QStringLiteral("%1%2:%3.%4")
        .arg(negative ? QStringLiteral("-") : QString())
        .arg(secs / 60)
        .arg(secs % 60, 2, 10, QLatin1Char('0'))
        .arg((a % 1000) / 100);
}

/** Spoken form: "8.4 seconds". Screen readers make a mess of "0:08.4". */
QString spokenSeconds(qint64 ms)
{
    return QStringLiteral("%1").arg(ms / 1000.0, 0, 'f', 1);
}

void paintPeaks(QPainter *p, const QRect &rect, const WaveformData &data,
                qint64 fromMs, qint64 toMs, const QColor &color)
{
    if (!data.ready() || rect.width() <= 0 || toMs <= fromMs)
        return;
    const double msPerPx = double(toMs - fromMs) / rect.width();
    const double midY = rect.center().y() + 0.5;
    const double halfH = rect.height() / 2.0 - 1.0;
    QVector<QLineF> lines;
    lines.reserve(rect.width());
    for (int x = 0; x < rect.width(); ++x) {
        const qint64 a = fromMs + qint64(x * msPerPx);
        const qint64 b = fromMs + qint64((x + 1) * msPerPx);
        int i0 = int(a / WaveformStore::MsPerPeak);
        int i1 = qMax(int(b / WaveformStore::MsPerPeak), i0 + 1);
        if (i0 < 0 || i0 >= data.peaks.size())
            continue;
        i1 = qMin(i1, int(data.peaks.size()));
        int peak = 0;
        for (int i = i0; i < i1; ++i)
            peak = qMax(peak, int(data.peaks[i]));
        const double amp = qMax(0.6, halfH * peak / 255.0);
        const double px = rect.left() + x + 0.5;
        lines.append(QLineF(px, midY - amp, px, midY + amp));
    }
    p->setPen(QPen(color, 1.0));
    p->drawLines(lines);
}
} // namespace

// ---------------------------------------------------------------------------
// VoiceTrackTimeline
// ---------------------------------------------------------------------------

VoiceTrackTimeline::VoiceTrackTimeline(WaveformStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(3 * kLaneHeight + 2 * kLaneGap + kTopMargin + 24);
    setAccessibleName(tr("Voice track timeline"));
    setAccessibleDescription(tr("The end of the outgoing song, the start of the "
                                "incoming song and the link, on one timeline. "
                                "Use the Left and Right arrow keys to move the "
                                "link a tenth of a second, Shift with an arrow "
                                "for a whole second, Page Up and Page Down for "
                                "five seconds."));
}

QSize VoiceTrackTimeline::sizeHint() const
{
    return QSize(720, 3 * kLaneHeight + 2 * kLaneGap + kTopMargin + 24);
}

void VoiceTrackTimeline::setJoin(const QString &prevPath, const QString &nextPath,
                                 qint64 joinOverlapMs)
{
    m_prevPath = prevPath;
    m_nextPath = nextPath;
    m_joinOverlapMs = qMax<qint64>(0, joinOverlapMs);
    update();
}

void VoiceTrackTimeline::setTake(const QString &takePath, qint64 takeDurationMs)
{
    m_takePath = takePath;
    m_takeDurationMs = qMax<qint64>(0, takeDurationMs);
    update();
}

void VoiceTrackTimeline::setLeadMs(qint64 leadMs)
{
    const qint64 clamped = qBound<qint64>(0, leadMs, maxLeadMs());
    if (clamped == m_leadMs)
        return;
    m_leadMs = clamped;
    update();
}

qint64 VoiceTrackTimeline::maxLeadMs() const
{
    qint64 cap = PlaylistWaveView::maxOverlapMs();
    if (m_store) {
        if (const WaveformData *d = m_store->peek(m_prevPath))
            if (d->ready())
                cap = qMin(cap, d->durationMs);
    }
    return qMax<qint64>(0, cap);
}

void VoiceTrackTimeline::setEnvelopes(const QVector<QPointF> &prevEnv,
                                      const QVector<QPointF> &nextEnv)
{
    m_prevEnv = prevEnv;
    m_nextEnv = nextEnv;
    update();
}

void VoiceTrackTimeline::setPlayhead(qint64 timelineMs, bool visible)
{
    m_playheadMs = timelineMs;
    m_playheadVisible = visible;
    update();
}

void VoiceTrackTimeline::setRecording(bool on, qint64 recordedMs)
{
    m_recording = on;
    m_recordedMs = recordedMs;
    update();
}

qint64 VoiceTrackTimeline::windowStartMs() const
{
    qint64 start = -qMax(m_leadMs, m_joinOverlapMs) - kPadMs;
    return qMin<qint64>(start, -kPadMs);
}

qint64 VoiceTrackTimeline::windowEndMs() const
{
    const qint64 takeEnd = -m_leadMs + qMax(m_takeDurationMs, m_recordedMs);
    qint64 end = qMax<qint64>(takeEnd, 0) + kPadMs;
    // Always show a little of the incoming song past the join, even before
    // anything has been recorded, so the lane is not an empty strip.
    return qMax<qint64>(end, kPadMs + 5000);
}

int VoiceTrackTimeline::xForMs(qint64 ms) const
{
    const qint64 a = windowStartMs();
    const qint64 b = windowEndMs();
    const int w = qMax(1, width() - 2 * kSideMargin);
    if (b <= a)
        return kSideMargin;
    return kSideMargin + int(double(ms - a) / double(b - a) * w);
}

qint64 VoiceTrackTimeline::msForX(int x) const
{
    const qint64 a = windowStartMs();
    const qint64 b = windowEndMs();
    const int w = qMax(1, width() - 2 * kSideMargin);
    return a + qint64(double(x - kSideMargin) / w * double(b - a));
}

QRect VoiceTrackTimeline::laneRect(int lane) const
{
    const int top = kTopMargin + lane * (kLaneHeight + kLaneGap);
    return QRect(kSideMargin, top, qMax(1, width() - 2 * kSideMargin), kLaneHeight);
}

void VoiceTrackTimeline::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), palette().base());

    const QColor prevColor(120, 150, 200);
    const QColor nextColor(120, 190, 150);
    const QColor takeColor(235, 170, 90);
    const QColor envColor(80, 200, 120);
    const QColor guide(150, 150, 150, 140);

    const WaveformData *prev = m_store ? m_store->peek(m_prevPath) : nullptr;
    const WaveformData *next = m_store ? m_store->peek(m_nextPath) : nullptr;
    const WaveformData *take =
        (m_store && !m_takePath.isEmpty()) ? m_store->peek(m_takePath) : nullptr;

    const qint64 durPrev = (prev && prev->ready()) ? prev->durationMs : 0;
    const qint64 durNext = (next && next->ready()) ? next->durationMs : 0;

    struct Lane { QRect rect; QString label; };
    const Lane lanes[3] = {
        {laneRect(0), tr("Outgoing: %1").arg(QFileInfo(m_prevPath).fileName())},
        {laneRect(1), tr("Incoming: %1").arg(QFileInfo(m_nextPath).fileName())},
        {laneRect(2), m_takePath.isEmpty() ? tr("The link (not recorded yet)")
                                           : tr("The link")}
    };
    for (const Lane &l : lanes) {
        p.fillRect(l.rect, palette().alternateBase());
        p.setPen(palette().mid().color());
        p.drawRect(l.rect.adjusted(0, 0, -1, -1));
    }

    // --- Lane 0: the outgoing song, ending at t = 0 -------------------------
    if (prev && prev->ready()) {
        const QRect r = lanes[0].rect.adjusted(1, 1, -1, -1);
        const qint64 fromT = qMax(windowStartMs(), -durPrev);
        const int x0 = xForMs(fromT);
        const int x1 = xForMs(0);
        if (x1 > x0) {
            const QRect sub(x0, r.top(), x1 - x0, r.height());
            paintPeaks(&p, sub, *prev, durPrev + fromT, durPrev, prevColor);
            if (!m_prevEnv.isEmpty()) {
                QPolygonF line;
                for (int x = sub.left(); x <= sub.right(); ++x) {
                    const qint64 trackMs = durPrev + msForX(x);
                    const double g = PlaylistWaveView::envelopeGainAt(m_prevEnv, trackMs);
                    line << QPointF(x, sub.bottom() - g * (sub.height() - 2));
                }
                p.setPen(QPen(envColor, 1.6));
                p.drawPolyline(line);
            }
        }
    }

    // --- Lane 1: the incoming song, starting at t = -joinOverlap ------------
    if (next && next->ready()) {
        const QRect r = lanes[1].rect.adjusted(1, 1, -1, -1);
        const int x0 = qMax(r.left(), xForMs(-m_joinOverlapMs));
        const int x1 = qMin(r.right(), xForMs(-m_joinOverlapMs + durNext));
        if (x1 > x0) {
            const QRect sub(x0, r.top(), x1 - x0, r.height());
            const qint64 fromMs = msForX(x0) + m_joinOverlapMs;
            const qint64 toMs = msForX(x1) + m_joinOverlapMs;
            paintPeaks(&p, sub, *next, qMax<qint64>(0, fromMs), toMs, nextColor);
            if (!m_nextEnv.isEmpty()) {
                QPolygonF line;
                for (int x = sub.left(); x <= sub.right(); ++x) {
                    const qint64 trackMs = msForX(x) + m_joinOverlapMs;
                    const double g = PlaylistWaveView::envelopeGainAt(m_nextEnv, trackMs);
                    line << QPointF(x, sub.bottom() - g * (sub.height() - 2));
                }
                p.setPen(QPen(envColor, 1.6));
                p.drawPolyline(line);
            }
        }
    }

    // --- Lane 2: the take ---------------------------------------------------
    const qint64 shownTakeDur = m_recording ? m_recordedMs : m_takeDurationMs;
    if (shownTakeDur > 0) {
        const QRect r = lanes[2].rect.adjusted(1, 1, -1, -1);
        const int x0 = qMax(r.left(), xForMs(-m_leadMs));
        const int x1 = qMin(r.right(), xForMs(-m_leadMs + shownTakeDur));
        if (x1 > x0) {
            const QRect sub(x0, r.top(), x1 - x0, r.height());
            if (take && take->ready()) {
                paintPeaks(&p, sub, *take, msForX(x0) + m_leadMs,
                           msForX(x1) + m_leadMs, takeColor);
            } else {
                // No waveform yet (still recording, or ffmpeg has not run):
                // a solid block still shows WHERE the link sits, which is the
                // thing the presenter is moving.
                p.fillRect(sub, QColor(takeColor.red(), takeColor.green(),
                                       takeColor.blue(), 70));
            }
            p.setPen(QPen(takeColor, 1.4));
            p.drawRect(sub.adjusted(0, 0, -1, -1));
        }
    }

    // --- Guides -------------------------------------------------------------
    p.setPen(QPen(guide, 1, Qt::DashLine));
    const int joinX = xForMs(-m_joinOverlapMs);
    p.drawLine(joinX, kTopMargin - 6, joinX, lanes[2].rect.bottom());
    const int endX = xForMs(0);
    p.drawLine(endX, kTopMargin - 6, endX, lanes[2].rect.bottom());

    p.setPen(palette().text().color());
    QFont small = p.font();
    small.setPointSizeF(qMax(7.0, small.pointSizeF() - 2.0));
    p.setFont(small);
    p.drawText(QRect(joinX - 60, 0, 120, kTopMargin - 3),
               Qt::AlignHCenter | Qt::AlignVCenter, tr("next song in"));
    p.drawText(QRect(endX - 60, 0, 120, kTopMargin - 3),
               Qt::AlignHCenter | Qt::AlignVCenter, tr("song ends"));
    for (const Lane &l : lanes)
        p.drawText(l.rect.adjusted(4, 2, -4, 0), Qt::AlignLeft | Qt::AlignTop, l.label);

    // --- Playhead -----------------------------------------------------------
    if (m_playheadVisible) {
        const int px = xForMs(m_playheadMs);
        p.setPen(QPen(QColor(220, 80, 80), 1.5));
        p.drawLine(px, kTopMargin - 6, px, lanes[2].rect.bottom());
    }

    if (hasFocus()) {
        p.setPen(QPen(palette().highlight().color(), 2, Qt::DotLine));
        p.drawRect(rect().adjusted(1, 1, -2, -2));
    }
}

void VoiceTrackTimeline::mousePressEvent(QMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    if (event->button() != Qt::LeftButton || m_takeDurationMs <= 0) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (!laneRect(2).contains(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_dragging = true;
    m_dragStartX = event->pos().x();
    m_dragStartLead = m_leadMs;
    event->accept();
}

void VoiceTrackTimeline::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    // Dragging right moves the link later, which is a SMALLER lead.
    const qint64 deltaMs = msForX(event->pos().x()) - msForX(m_dragStartX);
    applyLead(m_dragStartLead - deltaMs, false);
    event->accept();
}

void VoiceTrackTimeline::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_dragging = false;
        emit nudgeAnnounced(positionSummary());
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void VoiceTrackTimeline::keyPressEvent(QKeyEvent *event)
{
    qint64 step = 0;
    switch (event->key()) {
    case Qt::Key_Left:
        step = (event->modifiers() & Qt::ShiftModifier) ? 1000 : 100;
        break;
    case Qt::Key_Right:
        step = -((event->modifiers() & Qt::ShiftModifier) ? 1000 : 100);
        break;
    case Qt::Key_PageUp:
        step = 5000;
        break;
    case Qt::Key_PageDown:
        step = -5000;
        break;
    case Qt::Key_Home:
        applyLead(maxLeadMs(), true);
        event->accept();
        return;
    case Qt::Key_End:
        applyLead(0, true);
        event->accept();
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    applyLead(m_leadMs + step, true);
    event->accept();
}

void VoiceTrackTimeline::applyLead(qint64 leadMs, bool announce)
{
    const qint64 clamped = qBound<qint64>(0, leadMs, maxLeadMs());
    if (clamped != m_leadMs) {
        m_leadMs = clamped;
        update();
        emit leadMsChanged(m_leadMs);
    }
    if (announce)
        emit nudgeAnnounced(positionSummary());
}

QString VoiceTrackTimeline::positionSummary() const
{
    // Where the link sits relative to the join is the thing that has no
    // non-visual equivalent, so it is phrased the way a presenter thinks
    // about it rather than as a raw offset.
    const qint64 beforeNext = m_leadMs - m_joinOverlapMs;
    QString where;
    if (beforeNext > 0)
        where = tr("%1 seconds before the next song starts").arg(spokenSeconds(beforeNext));
    else if (beforeNext < 0)
        where = tr("%1 seconds after the next song starts").arg(spokenSeconds(-beforeNext));
    else
        where = tr("exactly where the next song starts");

    return tr("Link starts %1 seconds before the outgoing song ends, %2.")
        .arg(spokenSeconds(m_leadMs), where);
}

// ---------------------------------------------------------------------------
// VoiceTrackDialog
// ---------------------------------------------------------------------------

VoiceTrackDialog::VoiceTrackDialog(const QString &prevPath, const QString &nextPath,
                                   qint64 joinOverlapMs, WaveformStore *store,
                                   CueBus *cueBus, QWidget *parent)
    : QDialog(parent)
    , m_prevPath(prevPath)
    , m_nextPath(nextPath)
    , m_joinOverlapMs(qMax<qint64>(0, joinOverlapMs))
    , m_store(store)
    , m_cueBus(cueBus)
{
    setWindowTitle(tr("Voice track"));
    setModal(true);

    m_recorder = new VoiceRecorder(this);
    connect(m_recorder, &VoiceRecorder::levels, this, [this](float l, float r) {
        if (m_meter)
            m_meter->setLevels(l, r);
    });
    connect(m_recorder, &VoiceRecorder::progress, this, [this](qint64 ms) {
        if (m_elapsed)
            m_elapsed->setText(formatMs(ms));
        if (m_timeline)
            m_timeline->setRecording(true, ms);
    });
    connect(m_recorder, &VoiceRecorder::finished, this,
            [this](const QString &path, qint64 durationMs) {
        m_takePath = path;
        m_takeDurationMs = durationMs;
        if (m_opus && m_opus->isChecked()) {
            QString why;
            const QString converted = VoiceRecorder::convertToOpus(path, &why);
            if (converted != path)
                m_takePath = converted;
            else if (!why.isEmpty())
                qInfo() << "VoiceTrackDialog:" << why;
        }
        m_timeline->setRecording(false, 0);
        m_timeline->setTake(m_takePath, m_takeDurationMs);
        m_takeAnalysed = false;
        m_prevEnvEdited = false;
        m_nextEnvEdited = false;
        refreshWaveforms();
        analyseTake();
        updateEnabledState();
        updateSummary();
        // One utterance, not three: a screen reader given several messages in
        // quick succession routinely drops all but the last.
        announce(tr("Recording stopped. The link is %1 seconds long. %2")
                     .arg(spokenSeconds(m_takeDurationMs),
                          m_timeline->positionSummary()));
    });
    connect(m_recorder, &VoiceRecorder::failed, this, [this](const QString &msg) {
        stopMonitor();
        if (m_timeline)
            m_timeline->setRecording(false, 0);
        if (m_meter)
            m_meter->clear();
        updateEnabledState();
        announce(msg);
        QMessageBox::warning(this, tr("Voice track"), msg);
    });

    buildUi();

    // A sensible first position: the link starts a few seconds before the
    // incoming song does, so the presenter talks over the outro and hands
    // over into the intro — the shape of an ordinary link.
    const qint64 defaultLead = qMax<qint64>(m_joinOverlapMs + 4000, 10000);
    m_timeline->setJoin(m_prevPath, m_nextPath, m_joinOverlapMs);
    m_timeline->setLeadMs(defaultLead);
    m_leadSpin->setValue(int(m_timeline->leadMs()));

    if (m_store) {
        connect(m_store, &WaveformStore::waveformReady, this, [this](const QString &) {
            // The lead cap depends on the outgoing song's length, which only
            // becomes known when its waveform lands.
            m_leadSpin->setMaximum(int(qMax<qint64>(1000, m_timeline->maxLeadMs())));
            m_timeline->update();
            if (!m_takePath.isEmpty() && !m_takeAnalysed)
                analyseTake();
            updateSummary();
        });
    }
    refreshWaveforms();
    updateEnabledState();
    updateSummary();
}

VoiceTrackDialog::~VoiceTrackDialog()
{
    if (m_recorder && m_recorder->isRecording())
        m_recorder->stop();
    stopMonitor();
    stopPlayback();
}

void VoiceTrackDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);

    auto *heading = new QLabel(tr("Record your link across the join between "
                                  "<b>%1</b> and <b>%2</b>.")
                                   .arg(QFileInfo(m_prevPath).fileName().toHtmlEscaped(),
                                        QFileInfo(m_nextPath).fileName().toHtmlEscaped()),
                               this);
    heading->setWordWrap(true);
    heading->setAccessibleName(tr("Voice track: what this join is"));
    root->addWidget(heading);

    m_timeline = new VoiceTrackTimeline(m_store, this);
    connect(m_timeline, &VoiceTrackTimeline::leadMsChanged,
            this, &VoiceTrackDialog::onLeadChanged);
    connect(m_timeline, &VoiceTrackTimeline::nudgeAnnounced,
            this, &VoiceTrackDialog::announce);
    root->addWidget(m_timeline, 1);

    // --- Recording ---------------------------------------------------------
    auto *recBox = new QGroupBox(tr("The take"), this);
    auto *recLayout = new QHBoxLayout(recBox);

    auto *deviceLabel = new QLabel(tr("&Input:"), recBox);
    m_deviceBox = new QComboBox(recBox);
    m_deviceBox->addItems(VoiceRecorder::inputDeviceNames());
    const QString preferred = VoiceRecorder::defaultInputDeviceName();
    if (!preferred.isEmpty() && m_deviceBox->findText(preferred) >= 0)
        m_deviceBox->setCurrentText(preferred);
    m_deviceBox->setAccessibleName(tr("Recording input device"));
    deviceLabel->setBuddy(m_deviceBox);
    recLayout->addWidget(deviceLabel);
    recLayout->addWidget(m_deviceBox, 1);

    m_recordButton = new QPushButton(tr("&Record"), recBox);
    m_recordButton->setAccessibleName(tr("Record the link"));
    m_recordButton->setToolTip(tr("Start recording at the point the link is set "
                                  "to start; press again to stop."));
    connect(m_recordButton, &QPushButton::clicked, this, &VoiceTrackDialog::toggleRecord);
    recLayout->addWidget(m_recordButton);

    m_retakeButton = new QPushButton(tr("Re&take"), recBox);
    m_retakeButton->setAccessibleName(tr("Throw this take away and record again"));
    connect(m_retakeButton, &QPushButton::clicked, this, &VoiceTrackDialog::retake);
    recLayout->addWidget(m_retakeButton);

    m_meter = new LevelMeter(recBox);
    m_meter->setMeterOrientation(Qt::Horizontal);
    m_meter->setAccessibleName(tr("Recording level"));
    recLayout->addWidget(m_meter);

    m_elapsed = new QLabel(QStringLiteral("0:00.0"), recBox);
    m_elapsed->setAccessibleName(tr("Length recorded"));
    recLayout->addWidget(m_elapsed);

    root->addWidget(recBox);

    // --- Monitoring --------------------------------------------------------
    m_monitorNote = new QLabel(this);
    m_monitorNote->setWordWrap(true);
    m_monitorNote->setAccessibleName(tr("Monitoring"));
    root->addWidget(m_monitorNote);

    m_monitorMain = new QCheckBox(tr("Monitor on the &main output while recording "
                                     "(this output may be feeding air)"), this);
    m_monitorMain->setAccessibleName(tr("Monitor on the main output while recording"));
    root->addWidget(m_monitorMain);

    QString why;
    const bool cueOk = m_cueBus && m_cueBus->isAvailable(&why);
    if (cueOk) {
        m_monitorNote->setText(tr("You will hear the join in the cue headphones "
                                  "while you record."));
        m_monitorMain->setVisible(false);
    } else {
        m_monitorNote->setText(tr("No cue monitoring: %1 You can still record — "
                                  "tick the box below to hear the join on the main "
                                  "output instead, or record without hearing it.")
                                   .arg(why.isEmpty()
                                            ? tr("no cue bus is available.") : why));
    }

    // --- Where the link sits -----------------------------------------------
    auto *posBox = new QGroupBox(tr("Where the link sits"), this);
    auto *posForm = new QFormLayout(posBox);
    m_leadSpin = new QSpinBox(posBox);
    m_leadSpin->setRange(0, int(qMax<qint64>(1000, PlaylistWaveView::maxOverlapMs())));
    m_leadSpin->setSingleStep(100);
    m_leadSpin->setSuffix(tr(" ms"));
    m_leadSpin->setAccessibleName(tr("Link starts this long before the outgoing song ends"));
    connect(m_leadSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (m_timeline->leadMs() != qint64(value)) {
            m_timeline->setLeadMs(value);
            onLeadChanged(m_timeline->leadMs());
        }
    });
    posForm->addRow(tr("Starts &before the outgoing song ends:"), m_leadSpin);

    m_summary = new QLabel(posBox);
    m_summary->setWordWrap(true);
    m_summary->setAccessibleName(tr("Where the link sits relative to the join"));
    posForm->addRow(m_summary);
    root->addWidget(posBox);

    // --- Ducking ------------------------------------------------------------
    auto *duckBox = new QGroupBox(tr("Ducking (written onto both songs as an "
                                     "ordinary volume line)"), this);
    auto *duckForm = new QFormLayout(duckBox);

    m_duckSpin = new QSpinBox(duckBox);
    m_duckSpin->setRange(0, 100);
    m_duckSpin->setValue(25);
    m_duckSpin->setSuffix(tr(" %"));
    m_duckSpin->setAccessibleName(tr("Music level under the voice, as a percentage"));
    m_duckSpin->setToolTip(tr("How loud the music stays while you are talking. "
                              "This is a percentage of the track's own level, "
                              "not decibels."));
    duckForm->addRow(tr("Music &under the voice:"), m_duckSpin);

    m_fadeSpin = new QSpinBox(duckBox);
    m_fadeSpin->setRange(50, 3000);
    m_fadeSpin->setSingleStep(50);
    m_fadeSpin->setValue(400);
    m_fadeSpin->setSuffix(tr(" ms"));
    m_fadeSpin->setAccessibleName(tr("Duck fade length in milliseconds"));
    duckForm->addRow(tr("&Fade down and up over:"), m_fadeSpin);

    m_holdSpin = new QSpinBox(duckBox);
    m_holdSpin->setRange(0, 5000);
    m_holdSpin->setSingleStep(100);
    m_holdSpin->setValue(700);
    m_holdSpin->setSuffix(tr(" ms"));
    m_holdSpin->setAccessibleName(tr("How long the music stays down after you stop talking"));
    duckForm->addRow(tr("&Hold down after the voice:"), m_holdSpin);

    m_regenButton = new QPushButton(tr("Re&generate the ducking"), duckBox);
    m_regenButton->setAccessibleName(tr("Generate the ducking again from the take"));
    m_regenButton->setToolTip(tr("Rebuild both volume lines from the take. Any "
                                 "hand edits you made in the wave view are kept "
                                 "until you press this."));
    connect(m_regenButton, &QPushButton::clicked, this, &VoiceTrackDialog::regenerateDuck);
    duckForm->addRow(m_regenButton);
    root->addWidget(duckBox);

    // --- Audition -----------------------------------------------------------
    auto *playRow = new QHBoxLayout;
    m_playButton = new QPushButton(tr("&Play the link in place"), this);
    m_playButton->setAccessibleName(tr("Play the link in place"));
    m_playButton->setToolTip(tr("Hear the end of the outgoing song, the link and "
                                "the start of the incoming song, with the ducking "
                                "as it will go out."));
    connect(m_playButton, &QPushButton::clicked, this, &VoiceTrackDialog::togglePlayback);
    playRow->addWidget(m_playButton);
    playRow->addStretch(1);
    root->addLayout(playRow);

    m_opus = new QCheckBox(tr("Save the take as &Opus instead of WAV (needs an "
                              "ffmpeg with Opus support)"), this);
    m_opus->setAccessibleName(tr("Save the take as Opus"));
    root->addWidget(m_opus);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("&Keep the voice track"));
    m_buttons->button(QDialogButtonBox::Ok)->setAccessibleName(
        tr("Keep the voice track and add it to the playlist"));
    m_buttons->button(QDialogButtonBox::Cancel)->setText(tr("&Discard"));
    m_buttons->button(QDialogButtonBox::Cancel)->setAccessibleName(
        tr("Discard the take and close"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, [this]() {
        applyResult();
        accept();
    });
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(m_buttons);
}

void VoiceTrackDialog::refreshWaveforms()
{
    if (!m_store)
        return;
    m_store->fetch(m_prevPath);
    m_store->fetch(m_nextPath);
    if (!m_takePath.isEmpty())
        m_store->fetch(m_takePath);
}

qint64 VoiceTrackDialog::prevDurationMs() const
{
    if (!m_store)
        return 0;
    const WaveformData *d = m_store->peek(m_prevPath);
    return (d && d->ready()) ? d->durationMs : 0;
}

qint64 VoiceTrackDialog::nextDurationMs() const
{
    if (!m_store)
        return 0;
    const WaveformData *d = m_store->peek(m_nextPath);
    return (d && d->ready()) ? d->durationMs : 0;
}

qint64 VoiceTrackDialog::nextOverlapForLead(qint64 leadMs) const
{
    // The incoming song must keep starting exactly where it started before
    // the link existed. It used to overlap the outgoing song by
    // m_joinOverlapMs; now it overlaps the TAKE, which sits between them.
    const qint64 raw = m_takeDurationMs - leadMs + m_joinOverlapMs;
    qint64 cap = m_takeDurationMs;
    if (const qint64 durNext = nextDurationMs())
        cap = qMin(cap, durNext);
    return qBound<qint64>(0, raw, qMax<qint64>(0, cap));
}

VoiceDuck::Params VoiceTrackDialog::duckParams() const
{
    VoiceDuck::Params p;
    if (m_duckSpin)
        p.duckGain = m_duckSpin->value() / 100.0; // LINEAR, not dB
    if (m_fadeSpin)
        p.fadeMs = m_fadeSpin->value();
    if (m_holdSpin)
        p.holdMs = m_holdSpin->value();
    return p;
}

void VoiceTrackDialog::analyseTake()
{
    if (m_takePath.isEmpty() || !m_store)
        return;
    const WaveformData *take = m_store->peek(m_takePath);
    if (!take) {
        m_store->fetch(m_takePath); // may come back synchronously
        take = m_store->peek(m_takePath);
    }
    if (!take || !take->ready()) {
        if (take && take->failed) {
            m_takeAnalysed = true; // no point retrying
            announce(tr("The take could not be analysed, so no ducking was "
                        "written. The link itself is fine."));
        }
        return;
    }
    // The recorder knows the take's length to the sample; prefer it over the
    // waveform's, which is rounded to a 20 ms bucket.
    if (m_takeDurationMs <= 0)
        m_takeDurationMs = take->durationMs;

    m_spans = VoiceDuck::voiceSpans(*take, duckParams());
    m_takeAnalysed = true;
    m_timeline->setTake(m_takePath, m_takeDurationMs);
    regenerateDuck();
}

void VoiceTrackDialog::regenerateDuck()
{
    if (!m_takeAnalysed || m_spans.isEmpty()) {
        m_prevEnv.clear();
        m_nextEnv.clear();
        if (m_timeline)
            m_timeline->setEnvelopes(m_prevEnv, m_nextEnv);
        return;
    }
    const VoiceDuck::Params p = duckParams();
    const qint64 lead = m_timeline->leadMs();

    // The take starts at (durPrev - lead) on the outgoing song's own timeline,
    // and at (joinOverlap - lead) on the incoming song's — negative when the
    // link is already running before that song comes up.
    const qint64 durPrev = prevDurationMs();
    const qint64 durNext = nextDurationMs();
    m_prevEnv = durPrev > 0
        ? VoiceDuck::envelopeFromSpans(m_spans, durPrev - lead, durPrev, p)
        : QVector<QPointF>();
    m_nextEnv = durNext > 0
        ? VoiceDuck::envelopeFromSpans(m_spans, m_joinOverlapMs - lead, durNext, p)
        : QVector<QPointF>();

    m_prevEnvEdited = false;
    m_nextEnvEdited = false;
    if (m_timeline)
        m_timeline->setEnvelopes(m_prevEnv, m_nextEnv);
    updateSummary();
}

void VoiceTrackDialog::onLeadChanged(qint64 leadMs)
{
    if (m_leadSpin && m_leadSpin->value() != int(leadMs)) {
        QSignalBlocker block(m_leadSpin);
        m_leadSpin->setValue(int(leadMs));
    }
    // Moving the link moves what it is talking over, so the ducking has to
    // move with it. Cheap: the spans are already computed, only the mapping
    // onto each song's timeline changes.
    if (m_takeAnalysed)
        regenerateDuck();
    updateSummary();
}

void VoiceTrackDialog::updateSummary()
{
    if (!m_summary)
        return;
    QString text = m_timeline->positionSummary();
    if (m_takeDurationMs > 0) {
        text += QLatin1Char(' ');
        text += tr("The link is %1 seconds long.").arg(spokenSeconds(m_takeDurationMs));
        if (m_takeAnalysed) {
            text += QLatin1Char(' ');
            text += m_spans.isEmpty()
                ? tr("No speech was found in it, so no ducking was written.")
                : tr("%1 ducking %2 written onto the songs.")
                      .arg(m_spans.size())
                      .arg(m_spans.size() == 1 ? tr("section was") : tr("sections were"));
        }
    }
    m_summary->setText(text);
    m_summary->setAccessibleDescription(text);
}

void VoiceTrackDialog::updateEnabledState()
{
    const bool recording = m_recorder && m_recorder->isRecording();
    const bool haveTake = !m_takePath.isEmpty() && m_takeDurationMs > 0;
    m_recordButton->setText(recording ? tr("&Stop") : tr("&Record"));
    m_recordButton->setAccessibleName(recording ? tr("Stop recording the link")
                                                : tr("Record the link"));
    m_deviceBox->setEnabled(!recording);
    m_retakeButton->setEnabled(haveTake && !recording);
    m_playButton->setEnabled(haveTake && !recording);
    m_regenButton->setEnabled(m_takeAnalysed && !recording);
    m_leadSpin->setEnabled(!recording);
    m_opus->setEnabled(!recording && !haveTake);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(haveTake && !recording);
}

// --- Recording --------------------------------------------------------------

void VoiceTrackDialog::toggleRecord()
{
    if (m_recorder->isRecording()) {
        m_recorder->stop();
        stopMonitor();
        if (m_meter)
            m_meter->clear();
        updateEnabledState();
        return;
    }

    stopPlayback();

    const QString hint = QFileInfo(m_nextPath).completeBaseName();
    const QString path = VoiceRecorder::newTakePath(hint);
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Voice track"),
                             tr("XFB could not create the folder its voice tracks "
                                "are kept in, so there is nowhere to record to."));
        return;
    }

    QString error;
    if (!m_recorder->start(m_deviceBox->currentText(), path, &error)) {
        announce(error);
        QMessageBox::warning(this, tr("Voice track"), error);
        return;
    }

    m_takePath.clear();
    m_takeDurationMs = 0;
    m_takeAnalysed = false;
    m_spans.clear();
    m_prevEnv.clear();
    m_nextEnv.clear();
    m_timeline->setEnvelopes(m_prevEnv, m_nextEnv);
    m_timeline->setTake(QString(), 0);
    m_timeline->setRecording(true, 0);
    m_elapsed->setText(QStringLiteral("0:00.0"));

    startMonitor();
    updateEnabledState();
    announce(tr("Recording. Talk over the join; press Stop when you are done."));
}

void VoiceTrackDialog::retake()
{
    // The previous take's file is left on disk deliberately: an operator who
    // retakes by accident should be able to go and find what they lost, and
    // these files are small.
    m_takePath.clear();
    m_takeDurationMs = 0;
    m_takeAnalysed = false;
    m_spans.clear();
    m_prevEnv.clear();
    m_nextEnv.clear();
    stopPlayback();
    m_timeline->setTake(QString(), 0);
    m_timeline->setEnvelopes(m_prevEnv, m_nextEnv);
    m_elapsed->setText(QStringLiteral("0:00.0"));
    updateEnabledState();
    updateSummary();
    announce(tr("Take discarded. Press Record to try again."));
    toggleRecord();
}

// --- Monitoring the join while recording ------------------------------------

void VoiceTrackDialog::startMonitor()
{
    const qint64 lead = m_timeline->leadMs();
    const qint64 durPrev = prevDurationMs();

    QString why;
    m_monitorViaCue = m_cueBus && m_cueBus->isAvailable(&why);

    if (!m_monitorViaCue && !(m_monitorMain && m_monitorMain->isChecked())) {
        // Nothing to do: the presenter chose to (or has to) record blind. Said
        // once, plainly, rather than silently doing nothing.
        if (!why.isEmpty())
            qInfo() << "VoiceTrackDialog: recording without monitoring —" << why;
        return;
    }

    if (!m_monitorTimer) {
        m_monitorTimer = new QTimer(this);
        m_monitorTimer->setInterval(50);
        connect(m_monitorTimer, &QTimer::timeout, this, &VoiceTrackDialog::monitorTick);
    }
    m_monitorOnNext = (lead <= m_joinOverlapMs) || durPrev <= 0;

    if (m_monitorViaCue) {
        if (m_monitorOnNext) {
            m_cueBus->cueFrom(QUrl::fromLocalFile(m_nextPath),
                              QFileInfo(m_nextPath).fileName(),
                              qMax<qint64>(0, m_joinOverlapMs - lead));
        } else {
            m_cueBus->cueFrom(QUrl::fromLocalFile(m_prevPath),
                              QFileInfo(m_prevPath).fileName(),
                              qMax<qint64>(0, durPrev - lead));
        }
    } else {
        // Fallback path the operator explicitly asked for: the same local
        // players the audition uses, on the main output.
        stopPlayback();
        if (!m_prevPlayer) {
            m_prevOut = new QAudioOutput(this);
            m_nextOut = new QAudioOutput(this);
            m_takeOut = new QAudioOutput(this);
            m_prevPlayer = new FxPlayer(this);
            m_nextPlayer = new FxPlayer(this);
            m_takePlayer = new FxPlayer(this);
            m_prevPlayer->setAudioOutput(m_prevOut);
            m_nextPlayer->setAudioOutput(m_nextOut);
            m_takePlayer->setAudioOutput(m_takeOut);
        }
        m_prevOut->setVolume(kAuditionVolume);
        m_nextOut->setVolume(kAuditionVolume);
        if (m_monitorOnNext) {
            m_nextPlayer->setSource(QUrl::fromLocalFile(m_nextPath));
            m_nextPlayer->play();
            m_nextPlayer->setPosition(qMax<qint64>(0, m_joinOverlapMs - lead));
        } else {
            m_prevPlayer->setSource(QUrl::fromLocalFile(m_prevPath));
            m_prevPlayer->play();
            m_prevPlayer->setPosition(qMax<qint64>(0, durPrev - lead));
        }
    }
    m_monitorTimer->start();
}

void VoiceTrackDialog::monitorTick()
{
    if (m_monitorOnNext)
        return; // already on the incoming song; nothing left to switch

    const qint64 durPrev = prevDurationMs();
    if (durPrev <= 0)
        return;
    // Switch to the incoming song at exactly the moment it would come up on
    // air, so the presenter hears the handover where it will really happen.
    const qint64 switchAt = qMax<qint64>(0, durPrev - m_joinOverlapMs);

    qint64 pos = -1;
    if (m_monitorViaCue)
        pos = m_cueBus ? m_cueBus->cuePosition() : -1;
    else if (m_prevPlayer)
        pos = m_prevPlayer->position();

    const bool ended = (pos < 0);
    if (!ended && pos < switchAt)
        return;

    m_monitorOnNext = true;
    if (m_monitorViaCue) {
        m_cueBus->cueFrom(QUrl::fromLocalFile(m_nextPath),
                          QFileInfo(m_nextPath).fileName(), 0);
    } else if (m_nextPlayer) {
        m_nextPlayer->setSource(QUrl::fromLocalFile(m_nextPath));
        m_nextPlayer->play();
    }
}

void VoiceTrackDialog::stopMonitor()
{
    if (m_monitorTimer)
        m_monitorTimer->stop();
    if (m_monitorViaCue && m_cueBus)
        m_cueBus->stopCue();
    if (m_prevPlayer)
        m_prevPlayer->stop();
    if (m_nextPlayer)
        m_nextPlayer->stop();
    m_monitorViaCue = false;
    m_monitorOnNext = false;
}

// --- Audition of the finished link ------------------------------------------

void VoiceTrackDialog::togglePlayback()
{
    if (m_playTimer && m_playTimer->isActive()) {
        stopPlayback();
        announce(tr("Stopped."));
        return;
    }
    if (m_takePath.isEmpty() || m_takeDurationMs <= 0)
        return;

    const qint64 durPrev = prevDurationMs();
    const qint64 lead = m_timeline->leadMs();

    if (!m_prevPlayer) {
        m_prevOut = new QAudioOutput(this);
        m_nextOut = new QAudioOutput(this);
        m_takeOut = new QAudioOutput(this);
        m_prevPlayer = new FxPlayer(this);
        m_nextPlayer = new FxPlayer(this);
        m_takePlayer = new FxPlayer(this);
        m_prevPlayer->setAudioOutput(m_prevOut);
        m_nextPlayer->setAudioOutput(m_nextOut);
        m_takePlayer->setAudioOutput(m_takeOut);
    }
    // When the operator has a cue output, the audition belongs in their
    // headphones rather than on whatever the main output is feeding.
    if (m_cueBus && m_cueBus->isAvailable(nullptr)) {
        const QByteArray cueId = m_cueBus->cueDeviceId();
        m_prevPlayer->setOutputDeviceId(cueId);
        m_nextPlayer->setOutputDeviceId(cueId);
        m_takePlayer->setOutputDeviceId(cueId);
    }

    // Start a few seconds before the earliest thing that happens.
    m_playT0Ms = -(qMax(lead, m_joinOverlapMs) + 4000);
    if (durPrev > 0)
        m_playT0Ms = qMax(m_playT0Ms, -durPrev);
    m_playStartedAt = QDateTime::currentMSecsSinceEpoch();
    m_playTakeStarted = false;
    m_playNextStarted = false;

    m_prevOut->setVolume(kAuditionVolume);
    m_takeOut->setVolume(kAuditionVolume);
    m_nextOut->setVolume(kAuditionVolume);

    if (durPrev > 0) {
        m_prevPlayer->setSource(QUrl::fromLocalFile(m_prevPath));
        m_prevPlayer->play();
        m_prevPlayer->setPosition(qMax<qint64>(0, durPrev + m_playT0Ms));
    }

    if (!m_playTimer) {
        m_playTimer = new QTimer(this);
        m_playTimer->setInterval(40);
        connect(m_playTimer, &QTimer::timeout, this, &VoiceTrackDialog::playbackTick);
    }
    m_playTimer->start();
    m_playButton->setText(tr("&Stop the audition"));
    announce(tr("Playing the link in place."));
}

void VoiceTrackDialog::playbackTick()
{
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_playStartedAt;
    const qint64 t = m_playT0Ms + elapsed;
    const qint64 lead = m_timeline->leadMs();
    const qint64 durPrev = prevDurationMs();

    m_timeline->setPlayhead(t, true);

    // The outgoing song, ducked by the very line that will be saved on it.
    if (durPrev > 0 && m_prevOut) {
        const qint64 trackMs = durPrev + t;
        const double g = PlaylistWaveView::envelopeGainAt(m_prevEnv, trackMs);
        m_prevOut->setVolume(float(kAuditionVolume * g));
        if (t >= 0)
            m_prevPlayer->stop();
    }

    if (!m_playTakeStarted && t >= -lead) {
        m_playTakeStarted = true;
        m_takePlayer->setSource(QUrl::fromLocalFile(m_takePath));
        m_takePlayer->play();
        const qint64 into = t + lead;
        if (into > 100)
            m_takePlayer->setPosition(into);
    }

    if (!m_playNextStarted && t >= -m_joinOverlapMs) {
        m_playNextStarted = true;
        m_nextPlayer->setSource(QUrl::fromLocalFile(m_nextPath));
        m_nextPlayer->play();
        const qint64 into = t + m_joinOverlapMs;
        if (into > 100)
            m_nextPlayer->setPosition(into);
    }
    if (m_playNextStarted && m_nextOut) {
        const double g = PlaylistWaveView::envelopeGainAt(m_nextEnv, t + m_joinOverlapMs);
        m_nextOut->setVolume(float(kAuditionVolume * g));
    }

    // Run on a little past the end of the link so the presenter hears the
    // music come back up — which is the half of the duck that is easy to get
    // wrong and impossible to check by looking at it.
    const qint64 stopAt = qMax<qint64>(-lead + m_takeDurationMs, 0) + 4000;
    if (t >= stopAt) {
        stopPlayback();
        announce(tr("End of the audition."));
    }
}

void VoiceTrackDialog::stopPlayback()
{
    if (m_playTimer)
        m_playTimer->stop();
    if (m_prevPlayer)
        m_prevPlayer->stop();
    if (m_takePlayer)
        m_takePlayer->stop();
    if (m_nextPlayer)
        m_nextPlayer->stop();
    if (m_timeline)
        m_timeline->setPlayhead(0, false);
    if (m_playButton)
        m_playButton->setText(tr("&Play the link in place"));
}

// --- Result -----------------------------------------------------------------

void VoiceTrackDialog::applyResult()
{
    stopPlayback();
    stopMonitor();

    m_result = Result{};
    if (m_takePath.isEmpty() || m_takeDurationMs <= 0)
        return;

    const qint64 lead = m_timeline->leadMs();
    m_result.takePath = m_takePath;
    m_result.takeDurationMs = m_takeDurationMs;
    m_result.leadMs = lead;
    m_result.nextOverlapMs = nextOverlapForLead(lead);
    m_result.prevEnvelope = m_prevEnv.isEmpty()
        ? QString() : PlaylistWaveView::encodeEnvelope(m_prevEnv);
    m_result.nextEnvelope = m_nextEnv.isEmpty()
        ? QString() : PlaylistWaveView::encodeEnvelope(m_nextEnv);
}

void VoiceTrackDialog::announce(const QString &message)
{
    if (message.trimmed().isEmpty())
        return;
    emit announcementRequested(message);
}
