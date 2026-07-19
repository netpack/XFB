#include "ArtworkStore.h"

#include "audio/FxEngine.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QStandardPaths>

namespace
{
constexpr int kMaxParallelJobs = 2;
} // namespace

ArtworkStore::ArtworkStore(QObject *parent)
    : QObject(parent)
{
}

const ArtworkData *ArtworkStore::peek(const QString &filePath) const
{
    const auto it = m_cache.constFind(filePath);
    return it == m_cache.constEnd() ? nullptr : &it.value();
}

const ArtworkData *ArtworkStore::fetch(const QString &filePath)
{
    if (const ArtworkData *known = peek(filePath))
        return known;
    if (filePath.isEmpty() || m_pending.contains(filePath))
        return nullptr;

    // Disk cache: either the scaled cover or a "no artwork" marker
    const QString base = cacheBaseFor(filePath);
    if (QFile::exists(base + QLatin1String(".png"))) {
        ArtworkData data;
        data.pixmap = QPixmap(base + QLatin1String(".png"));
        data.failed = data.pixmap.isNull();
        m_cache.insert(filePath, data);
        return peek(filePath);
    }
    if (QFile::exists(base + QLatin1String(".noart"))) {
        ArtworkData data;
        data.failed = true;
        m_cache.insert(filePath, data);
        return peek(filePath);
    }

    m_pending.insert(filePath);
    m_queue.append(filePath);
    startNext();
    return nullptr;
}

void ArtworkStore::startNext()
{
    while (m_running < kMaxParallelJobs && !m_queue.isEmpty()) {
        const QString path = m_queue.takeFirst();

        const QString ffmpeg = FxEngine::ffmpegExecutable();
        if (ffmpeg.isEmpty() || !QFile::exists(path)) {
            finishJob(path, folderArtworkFor(path));
            continue;
        }

        // The extraction goes straight into the cache location; a scaled
        // PNG keeps the cache small whatever the embedded format was.
        const QString outPath = cacheBaseFor(path) + QLatin1String(".png");
        QDir().mkpath(QFileInfo(outPath).absolutePath());

        ++m_running;
        auto *proc = new QProcess(this);
        connect(proc, &QProcess::finished, this,
                [this, proc, path, outPath](int exitCode, QProcess::ExitStatus status) {
            QImage image;
            if (status == QProcess::NormalExit && exitCode == 0)
                image = QImage(outPath);
            if (image.isNull()) {
                QFile::remove(outPath); // no partial/failed output in the cache
                image = folderArtworkFor(path);
            }
            proc->deleteLater();
            --m_running;
            finishJob(path, image);
            startNext();
        });
        connect(proc, &QProcess::errorOccurred, this,
                [this, proc, path](QProcess::ProcessError) {
            if (proc->state() != QProcess::NotRunning)
                return; // finished() will handle it
            proc->deleteLater();
            --m_running;
            finishJob(path, folderArtworkFor(path));
            startNext();
        });

        proc->start(ffmpeg,
                    {QStringLiteral("-v"), QStringLiteral("error"),
                     QStringLiteral("-nostdin"), QStringLiteral("-y"),
                     QStringLiteral("-i"), path,
                     QStringLiteral("-map"), QStringLiteral("0:v:0"),
                     QStringLiteral("-frames:v"), QStringLiteral("1"),
                     QStringLiteral("-vf"),
                     QStringLiteral("scale='min(iw,%1)':-2").arg(MaxEdge),
                     outPath});
    }
}

void ArtworkStore::finishJob(const QString &filePath, const QImage &image)
{
    ArtworkData result;
    if (!image.isNull()) {
        QImage scaled = image;
        if (scaled.width() > MaxEdge || scaled.height() > MaxEdge)
            scaled = scaled.scaled(MaxEdge, MaxEdge, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        result.pixmap = QPixmap::fromImage(scaled);
    }
    result.failed = result.pixmap.isNull();

    const QString base = cacheBaseFor(filePath);
    if (result.failed) {
        // Remember the miss so the file is not probed again next session
        QDir().mkpath(QFileInfo(base).absolutePath());
        QFile marker(base + QLatin1String(".noart"));
        marker.open(QIODevice::WriteOnly);
    } else if (!QFile::exists(base + QLatin1String(".png"))) {
        // Folder-image fallback: persist the scaled copy ourselves
        result.pixmap.save(base + QLatin1String(".png"));
    }

    m_cache.insert(filePath, result);
    m_pending.remove(filePath);
    emit artworkReady(filePath);
}

QString ArtworkStore::cacheBaseFor(const QString &filePath) const
{
    const QFileInfo info(filePath);
    const QByteArray key = QString(filePath + QLatin1Char('|')
                                   + QString::number(info.size()) + QLatin1Char('|')
                                   + QString::number(info.lastModified().toSecsSinceEpoch()))
                               .toUtf8();
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                        + QStringLiteral("/artwork");
    return dir + QLatin1Char('/')
           + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex());
}

QImage ArtworkStore::folderArtworkFor(const QString &filePath)
{
    const QFileInfo info(filePath);
    const QDir dir = info.dir();
    if (!dir.exists())
        return QImage();

    static const QStringList kNames = {
        QStringLiteral("cover"), QStringLiteral("folder"),
        QStringLiteral("front"), QStringLiteral("album")};
    const QFileInfoList candidates = dir.entryInfoList(
        {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png")},
        QDir::Files | QDir::Readable);
    for (const QFileInfo &candidate : candidates) {
        if (kNames.contains(candidate.completeBaseName().toLower()))
            return QImage(candidate.absoluteFilePath());
    }
    return QImage();
}

// --- NowPlayingArtPanel ---------------------------------------------------

NowPlayingArtPanel::NowPlayingArtPanel(ArtworkStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    setMinimumSize(140, 150);
    if (m_store) {
        connect(m_store, &ArtworkStore::artworkReady, this,
                [this](const QString &path) {
            if (path == m_path)
                update();
        });
    }
}

void NowPlayingArtPanel::setTrack(const QString &filePath)
{
    if (m_path == filePath)
        return;
    m_path = filePath;
    if (m_store && !m_path.isEmpty())
        m_store->fetch(m_path);
    update();
}

void NowPlayingArtPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QPalette &pal = palette();
    const int titleH = fontMetrics().height() + 8;
    QRect artArea = rect().adjusted(8, 8, -8, -8 - titleH);

    // Largest centered square that fits the art area
    const int edge = qMin(artArea.width(), artArea.height());
    QRect square(0, 0, edge, edge);
    square.moveCenter(artArea.center());

    const ArtworkData *art =
        (m_store && !m_path.isEmpty()) ? m_store->peek(m_path) : nullptr;

    QPainterPath clip;
    clip.addRoundedRect(square, 8, 8);

    if (art && art->ready()) {
        // Fit the cover inside the square, centered, aspect kept
        const QPixmap &pix = art->pixmap;
        QSize fitted = pix.size().scaled(square.size(), Qt::KeepAspectRatio);
        QRect target(QPoint(0, 0), fitted);
        target.moveCenter(square.center());

        QPainterPath artClip;
        artClip.addRoundedRect(target, 8, 8);
        painter.save();
        painter.setClipPath(artClip);
        painter.drawPixmap(target, pix);
        painter.restore();
    } else {
        // Placeholder: quiet tile with a music note
        painter.save();
        painter.setClipPath(clip);
        painter.fillRect(square, pal.alternateBase());
        painter.restore();
        painter.setPen(QPen(pal.mid().color(), 1));
        painter.drawPath(clip);

        QFont noteFont = font();
        noteFont.setPointSizeF(qMax(18.0, edge / 4.0));
        painter.setFont(noteFont);
        QColor noteColor = pal.text().color();
        noteColor.setAlpha(70);
        painter.setPen(noteColor);
        painter.drawText(square, Qt::AlignCenter, QStringLiteral("♪"));
    }

    // Track title under the artwork
    if (!m_path.isEmpty()) {
        const QString title = QFileInfo(m_path).completeBaseName();
        QRect titleRect(rect().left() + 8, artArea.bottom() + 4,
                        rect().width() - 16, titleH);
        painter.setFont(font());
        painter.setPen(pal.text().color());
        painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,
                         fontMetrics().elidedText(title, Qt::ElideMiddle,
                                                  titleRect.width()));
    }
}
