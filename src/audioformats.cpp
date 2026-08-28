#include "audioformats.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace AudioFormats {
namespace {
// Q_DECLARE_TR_FUNCTIONS only expands inside a class, so the translation
// context is spelled out instead. lupdate picks these up as "AudioFormats".
QString tr(const char *sourceText)
{
    return QCoreApplication::translate("AudioFormats", sourceText);
}
} // namespace

const QStringList &suffixes()
{
    // Ordered the way a station thinks about them, not alphabetically: the
    // formats a library is actually made of first. "oga" is here because
    // Opus/Vorbis in an Ogg container is very often named that way, and
    // "opus" because the built-in downloader produces it.
    static const QStringList kSuffixes = {
        QStringLiteral("mp3"),  QStringLiteral("ogg"),  QStringLiteral("oga"),
        QStringLiteral("opus"), QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("m4a"),  QStringLiteral("mp4"),  QStringLiteral("aac"),
        QStringLiteral("wma"),  QStringLiteral("aiff"), QStringLiteral("aif")};
    return kSuffixes;
}

const QStringList &nameFilters()
{
    static const QStringList kFilters = []() {
        QStringList out;
        const QStringList &exts = suffixes();
        out.reserve(exts.size());
        for (const QString &ext : exts)
            out << QStringLiteral("*.") + ext;
        return out;
    }();
    return kFilters;
}

QString fileDialogFilter()
{
    return tr("Audio Files (%1)").arg(nameFilters().join(QLatin1Char(' ')));
}

QStringList fileDialogFilters()
{
    return QStringList{fileDialogFilter(), tr("All Files (*)")};
}

QString fileDialogFilterString()
{
    return fileDialogFilters().join(QStringLiteral(";;"));
}

bool isAudioFile(const QString &filePath)
{
    if (filePath.isEmpty())
        return false;
    // QFileInfo::suffix() is purely textual here — it does not touch the disk,
    // so this works for names that do not exist yet.
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return !suffix.isEmpty() && suffixes().contains(suffix);
}

QString stripSuffix(const QString &fileNameOrPath)
{
    const QFileInfo info(fileNameOrPath);
    const QString name = info.fileName().isEmpty() ? fileNameOrPath : info.fileName();
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    if (dot <= 0)
        return name;
    const QString ext = name.mid(dot + 1).toLower();
    if (!suffixes().contains(ext))
        return name;
    return name.left(dot);
}

void splitArtistAndSong(const QString &fileNameOrPath, QString *artist, QString *song)
{
    QString base = stripSuffix(fileNameOrPath);
    base.replace(QLatin1Char('_'), QLatin1Char(' '));

    const int dash = base.indexOf(QLatin1Char('-'));
    if (dash < 0) {
        if (artist) *artist = base.trimmed();
        if (song)   song->clear();
        return;
    }

    if (artist) *artist = base.left(dash).trimmed();
    if (song)   *song = base.mid(dash + 1).trimmed();
}

QStringList findAudioFiles(const QString &directory, bool recursive, bool followSymlinks)
{
    QStringList files;
    if (directory.trimmed().isEmpty())
        return files;

    QSet<QString> visited;      // canonical directory paths already entered
    QSet<QString> seenFiles;    // canonical file paths already collected
    QStringList pending{QDir::cleanPath(directory)};

    while (!pending.isEmpty()) {
        const QString current = pending.takeLast();

        // The loop guard. A symlink chain that comes back to a folder already
        // walked resolves to the same canonical path and is dropped here;
        // without this a link pointing at its own ancestor never ends.
        const QString canonical = QFileInfo(current).canonicalFilePath();
        if (canonical.isEmpty())
            continue; // broken link, or gone since we queued it
        if (visited.contains(canonical))
            continue;
        visited.insert(canonical);

        const QDir dir(current);
        if (!dir.exists())
            continue;

        const QFileInfoList entries = dir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
            QDir::Name);

        for (const QFileInfo &info : entries) {
            if (info.isDir()) {
                if (!recursive)
                    continue;
                if (info.isSymLink() && !followSymlinks)
                    continue;
                // Queue the path as the caller would spell it, so anything we
                // find below keeps the link path rather than its target.
                pending << info.absoluteFilePath();
                continue;
            }
            if (!isAudioFile(info.fileName()))
                continue;
            // Two links to the same track in one tree should be imported once.
            const QString fileCanonical = info.canonicalFilePath();
            if (!fileCanonical.isEmpty()) {
                if (seenFiles.contains(fileCanonical))
                    continue;
                seenFiles.insert(fileCanonical);
            }
            files << info.absoluteFilePath();
        }
    }

    files.sort();
    return files;
}

} // namespace AudioFormats
