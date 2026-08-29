#ifndef HOURCLOCKWHEEL_H
#define HOURCLOCKWHEEL_H

#include <QWidget>

#include "../services/HourClock.h"

/**
 * @brief The hour drawn as a clock face: one wedge per slot, read clockwise
 *        from twelve.
 *
 * This is the picture half of the editor. It is deliberately **not** the only
 * half — everything that can be done here can be done from the keyboard in the
 * list beside it, because a picture is worth nothing to an operator using a
 * screen reader, and this application has those. Both views drive the same
 * HourClock::Clock; this one never owns it.
 *
 * Interaction:
 *   - click a wedge to select it (and double-click, or Return, to edit it)
 *   - drag a wedge's body to move it: a hard-timed slot's fixed time changes,
 *     a floating slot moves in the running order
 *   - drag a wedge's trailing edge to change its length
 *
 * All colours come from ThemeManager / QPalette. Nothing here is a literal —
 * a hardcoded colour is invisible in the studio theme and glaring in the
 * light one.
 */
class HourClockWheel : public QWidget
{
    Q_OBJECT

public:
    explicit HourClockWheel(QWidget *parent = nullptr);

    void setClock(const HourClock::Clock &clock);
    const HourClock::Clock &clock() const { return m_clock; }

    /** Index into the clock's slot list, or -1. */
    int  selectedIndex() const { return m_selected; }
    void setSelectedIndex(int index);

    /** Second of the hour drawn as the "now" hand; < 0 hides it. */
    void setNowSecond(int second);

    /**
     * The editor moved the slot currently being dragged to a new index (a
     * floating slot dragged past its neighbour changes place in the running
     * order). Re-baselines the drag so the wedge keeps following the pointer
     * instead of jumping.
     */
    void retargetDrag(int newIndex);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void selectionChanged(int index);
    void editRequested(int index);
    /** The operator dragged the wedge for slot @a index to a new start. */
    void slotMoved(int index, int newStartSeconds);
    void slotResized(int index, int newLengthSeconds);
    /** Something a screen reader should hear (the wheel is not silent even
     *  though it is a picture — a sighted operator's drag is announced too). */
    void announcementRequested(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    enum class Drag { None, Move, ResizeEnd };

    QRectF faceRect() const;
    /** Seconds of the hour under a point, 0..3599; -1 outside the face. */
    int    secondAt(const QPointF &pos) const;
    int    indexAt(const QPointF &pos) const;
    /** True when @a pos is within a few pixels of the trailing edge of the
     *  wedge for @a index — the grab handle for resizing. */
    bool   nearTrailingEdge(const QPointF &pos, int index) const;
    QColor colourFor(HourClock::SlotType type, bool selected) const;
    void   announceSelection();

    HourClock::Clock    m_clock;
    HourClock::Timeline m_timeline;
    int   m_selected  = -1;
    int   m_nowSecond = -1;

    Drag  m_drag = Drag::None;
    int   m_dragIndex = -1;
    int   m_dragGrabSecond = 0;    ///< where in the wedge the grab happened
    int   m_dragOriginalStart = 0;
    int   m_dragOriginalLength = 0;
    bool  m_dragMoved = false;
};

#endif // HOURCLOCKWHEEL_H
