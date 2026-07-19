#ifndef LEVELMETER_H
#define LEVELMETER_H

#include <QWidget>

class QTimer;

/**
 * @brief Stereo LED-style peak meter (L and R rows/columns of segments).
 *
 * Feed it linear peak levels (0..1) via setLevels(); segments light green
 * through yellow to red like a classic console meter. Levels decay on a
 * timer so the display falls back smoothly when the audio goes quiet or
 * the feed stops (e.g. passthrough playback, which has no sample tap).
 *
 * Supports two orientations: Qt::Horizontal draws two stacked rows
 * (for the volume-slider strip), Qt::Vertical draws two side-by-side
 * columns rising bottom-up (for docking beside the side panel).
 */
class LevelMeter : public QWidget
{
    Q_OBJECT

public:
    explicit LevelMeter(QWidget *parent = nullptr);

    void setMeterOrientation(Qt::Orientation orientation);
    Qt::Orientation meterOrientation() const { return m_orientation; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    /** Linear peak levels 0..1 for the left and right channels. */
    void setLevels(float left, float right);
    /** Snap both channels straight to silence (stop/pause). */
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void decayTick();
    static QColor segmentBaseColor(int segment, int total);

    static constexpr int kSegments = 20;

    Qt::Orientation m_orientation = Qt::Horizontal;
    float m_left = 0.0f;
    float m_right = 0.0f;
    float m_peakLeft = 0.0f;  // peak-hold markers
    float m_peakRight = 0.0f;
    QTimer *m_decayTimer = nullptr;
};

#endif // LEVELMETER_H
