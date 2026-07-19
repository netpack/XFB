#ifndef ARTWORKSTORE_H
#define ARTWORKSTORE_H

#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QStringList>
#include <QWidget>

/** Cover art of one audio file (empty pixmap when the file has none). */
struct ArtworkData
{
    QPixmap pixmap;
    bool failed = false;

    bool ready() const { return !pixmap.isNull(); }
};

/**
 * Asynchronous provider of track artwork for local audio files.
 *
 * Embedded cover art is extracted with the same ffmpeg executable the FX
 * engine uses (an attached picture is a video stream, so one frame of the
 * first video stream is the cover). When a file has none, a cover image
 * sitting next to it (cover/folder/front/album.jpg|png) is used instead.
 * Results are scaled down and cached in memory and on disk
 * (CacheLocation/artwork, keyed by path + size + mtime), so a library
 * only pays the extraction cost once per file.
 */
class ArtworkStore : public QObject
{
    Q_OBJECT

public:
    /** Longest edge of cached artwork pixmaps. */
    static constexpr int MaxEdge = 256;

    explicit ArtworkStore(QObject *parent = nullptr);

    /** Returns the artwork if already known (possibly a failed marker). */
    const ArtworkData *peek(const QString &filePath) const;

    /** Like peek(), but schedules a background extraction when unknown. */
    const ArtworkData *fetch(const QString &filePath);

signals:
    /** Emitted when an extraction finishes, successfully or not. */
    void artworkReady(const QString &filePath);

private:
    void startNext();
    void finishJob(const QString &filePath, const QImage &image);
    QString cacheBaseFor(const QString &filePath) const;
    static QImage folderArtworkFor(const QString &filePath);

    QHash<QString, ArtworkData> m_cache;
    QStringList m_queue;
    QSet<QString> m_pending; // queued or currently extracting
    int m_running = 0;
};

/**
 * "Now playing" artwork panel: the cover of the track on air, drawn large
 * with the track title underneath. Lives in its own dock of the main
 * window, so it can be moved, floated or hidden like every other panel.
 */
class NowPlayingArtPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NowPlayingArtPanel(ArtworkStore *store, QWidget *parent = nullptr);

    /** File whose artwork to show; empty clears back to the placeholder. */
    void setTrack(const QString &filePath);
    QString track() const { return m_path; }

    /** Roomy enough for a readable cover at the side panel's width. */
    QSize sizeHint() const override { return QSize(240, 300); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ArtworkStore *m_store = nullptr;
    QString m_path;
};

#endif // ARTWORKSTORE_H
