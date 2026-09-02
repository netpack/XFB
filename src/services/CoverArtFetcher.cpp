#include "CoverArtFetcher.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <functional>

namespace {

/// GUI processes do not inherit the shell PATH on macOS or Windows, so the
/// same widening the downloader does is needed here.
QString resolveTool(const QString &name)
{
    QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) return found;

    QStringList places;
#ifdef Q_OS_WIN
    places << QDir::homePath() + "/.local/bin/" + name + ".exe";
    found = QStandardPaths::findExecutable(name + ".exe");
    if (!found.isEmpty()) return found;
#else
    places << QDir::homePath() + "/.local/bin/" + name
           << "/opt/homebrew/bin/" + name
           << "/usr/local/bin/" + name
           << "/usr/bin/" + name;
#endif
    for (const QString &place : places) {
        const QFileInfo info(place);
        if (info.exists() && info.isExecutable()) return place;
    }
    return QString();
}

/// Runs a tool and returns true when it finished cleanly. Output is merged so
/// a failure can be reported in the tool's own words.
bool run(const QString &program, const QStringList &arguments, int timeoutMs, QString *output)
{
    if (program.isEmpty()) return false;
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, arguments);
    const bool ok = process.waitForStarted(8000)
                    && process.waitForFinished(timeoutMs)
                    && process.exitStatus() == QProcess::NormalExit
                    && process.exitCode() == 0;
    if (output) *output = QString::fromUtf8(process.readAll()).trimmed();
    return ok;
}

/// A file name for the cache that cannot collide and cannot escape it.
QString keyFor(const QString &filePath)
{
    QByteArray key;
    for (const QChar character : filePath) {
        if (character.isLetter() || character.isDigit())
            key += character.toLatin1();
    }
    return QString::fromLatin1(key.right(40))
           + QString::number(qHash(filePath), 16);
}

} // namespace

QString CoverArtFetcher::ffmpegPath()  { return resolveTool(QStringLiteral("ffmpeg")); }
QString CoverArtFetcher::ffprobePath() { return resolveTool(QStringLiteral("ffprobe")); }
QString CoverArtFetcher::ytDlpPath()   { return resolveTool(QStringLiteral("yt-dlp")); }

QString CoverArtFetcher::cacheDirectory()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString directory = QDir(base).filePath(QStringLiteral("covers"));
    QDir().mkpath(directory);
    return directory;
}

bool CoverArtFetcher::hasCover(const QString &filePath)
{
    // One probe per file, and it is the same question for every container:
    // ffmpeg's ogg demuxer presents an Opus METADATA_BLOCK_PICTURE comment as
    // a synthetic video stream, exactly as MP3 and M4A present a real attached
    // picture. So "has a video stream" is "has a cover" throughout.
    QString output;
    const bool ok = run(ffprobePath(),
                        {"-v", "error", "-select_streams", "v:0",
                         "-show_entries", "stream=codec_name",
                         "-of", "default=noprint_wrappers=1:nokey=1", filePath},
                        20000, &output);
    return ok && !output.isEmpty();
}

QVector<CoverArtFetcher::Candidate> CoverArtFetcher::findTracksWithoutCover(
    QSqlDatabase &db,
    const std::function<bool(int, int, const QString &)> &keepGoing)
{
    QVector<Candidate> missing;

    const bool keepsSource =
        db.record(QStringLiteral("musics")).contains(QStringLiteral("source_url"));

    QSqlQuery query(db);
    query.prepare(keepsSource
        ? QStringLiteral("SELECT artist, song, path, source_url FROM musics "
                         "WHERE path IS NOT NULL AND path <> '' ORDER BY id DESC")
        : QStringLiteral("SELECT artist, song, path, '' FROM musics "
                         "WHERE path IS NOT NULL AND path <> '' ORDER BY id DESC"));
    if (!query.exec())
        return missing;

    QVector<Candidate> rows;
    while (query.next()) {
        Candidate candidate;
        candidate.artist    = query.value(0).toString();
        candidate.song      = query.value(1).toString();
        candidate.filePath  = query.value(2).toString();
        candidate.sourceUrl = query.value(3).toString();
        if (QFileInfo::exists(candidate.filePath))
            rows.append(candidate);
    }

    int done = 0;
    for (Candidate &candidate : rows) {
        if (keepGoing && !keepGoing(done, rows.size(), candidate.song))
            break;
        if (!hasCover(candidate.filePath))
            missing.append(candidate);
        ++done;
    }
    if (keepGoing) keepGoing(done, rows.size(), QString());
    return missing;
}

QString CoverArtFetcher::fetchThumbnail(const QString &target, const QString &basePath)
{
    const QString ytDlp = ytDlpPath();
    if (ytDlp.isEmpty()) return QString();

    // --convert-thumbnails leans on ffmpeg, which is also what writes the
    // picture in later, so a desk that can do one can do the other.
    QStringList arguments;
    arguments << "--skip-download" << "--write-thumbnail"
              << "--convert-thumbnails" << "jpg"
              << "--no-playlist" << "--playlist-items" << "1"
              << "--no-warnings" << "--socket-timeout" << "20"
              << "-o" << basePath + ".%(ext)s"
              << target;

    QString output;
    run(ytDlp, arguments, 120000, &output);

    // yt-dlp names the file after the output template, and which extension it
    // lands on depends on what the site had; take whichever appeared.
    const QFileInfo base(basePath);
    const QDir directory(base.absolutePath());
    const QStringList found =
        directory.entryList({base.fileName() + ".*"}, QDir::Files, QDir::Time);
    for (const QString &name : found) {
        const QString suffix = QFileInfo(name).suffix().toLower();
        if (suffix == "jpg" || suffix == "jpeg" || suffix == "png" || suffix == "webp")
            return directory.filePath(name);
    }
    return QString();
}

void CoverArtFetcher::findCoverFor(Candidate &candidate)
{
    candidate.imagePath.clear();
    candidate.preview = QImage();
    candidate.foundVia.clear();
    candidate.problem.clear();

    if (ytDlpPath().isEmpty()) {
        candidate.problem = QObject::tr("yt-dlp was not found on this computer.");
        return;
    }

    const QString basePath =
        QDir(cacheDirectory()).filePath(keyFor(candidate.filePath));

    QString image;
    if (!candidate.sourceUrl.isEmpty()) {
        image = fetchThumbnail(candidate.sourceUrl, basePath);
        if (!image.isEmpty())
            candidate.foundVia = QObject::tr("the link it was downloaded from");
    }

    if (image.isEmpty()) {
        // Nothing to go back to, so ask for the track by name. The artist is
        // included because a title alone matches far too much.
        QString terms = (candidate.artist + QLatin1Char(' ') + candidate.song).trimmed();
        if (terms.isEmpty()) {
            candidate.problem = QObject::tr("this track has no artist or title to search for.");
            return;
        }
        image = fetchThumbnail(QStringLiteral("ytsearch1:") + terms, basePath);
        if (!image.isEmpty())
            candidate.foundVia = QObject::tr("a search for \"%1\"").arg(terms);
    }

    if (image.isEmpty()) {
        candidate.problem = QObject::tr("no picture was found for it.");
        return;
    }

    QImageReader reader(image);
    reader.setAutoTransform(true);
    const QImage loaded = reader.read();
    if (loaded.isNull()) {
        QFile::remove(image);
        candidate.problem = QObject::tr("what came back was not an image.");
        return;
    }

    candidate.imagePath = image;
    candidate.preview = loaded.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QByteArray CoverArtFetcher::pictureBlockFor(const QString &imagePath)
{
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) return QByteArray();
    const QByteArray imageData = file.readAll();
    file.close();
    if (imageData.isEmpty()) return QByteArray();

    QImageReader reader(imagePath);
    const QSize size = reader.size();
    const QString format = QString::fromLatin1(reader.format()).toLower();

    QByteArray mime;
    if (format == QLatin1String("jpeg") || format == QLatin1String("jpg")) mime = "image/jpeg";
    else if (format == QLatin1String("png")) mime = "image/png";
    else return QByteArray();

    // The FLAC picture block a Vorbis comment carries: every field big-endian,
    // MIME type and description length-prefixed. Same shape the downloader
    // rebuilds when it has to move a cover back into a comment.
    QByteArray block;
    auto appendBE32 = [&block](quint32 value) {
        block.append(char((value >> 24) & 0xFF));
        block.append(char((value >> 16) & 0xFF));
        block.append(char((value >> 8) & 0xFF));
        block.append(char(value & 0xFF));
    };
    appendBE32(3);                       // picture type: front cover
    appendBE32(quint32(mime.size()));
    block.append(mime);
    appendBE32(0);                       // empty description
    appendBE32(quint32(qMax(0, size.width())));
    appendBE32(quint32(qMax(0, size.height())));
    appendBE32(24);                      // colour depth
    appendBE32(0);                       // not palette-indexed
    appendBE32(quint32(imageData.size()));
    block.append(imageData);
    return block.toBase64();
}

bool CoverArtFetcher::embedCover(Candidate &candidate)
{
    const QString ffmpeg = ffmpegPath();
    if (ffmpeg.isEmpty()) {
        candidate.problem = QObject::tr("ffmpeg was not found on this computer.");
        return false;
    }
    if (!candidate.found() || !QFileInfo::exists(candidate.imagePath)) {
        candidate.problem = QObject::tr("there is no picture to write.");
        return false;
    }

    const QFileInfo info(candidate.filePath);
    const QString suffix = info.suffix().toLower();
    const bool oggFamily = (suffix == QLatin1String("opus")
                            || suffix == QLatin1String("ogg")
                            || suffix == QLatin1String("oga"));

    const QString written = candidate.filePath + QStringLiteral(".cover.") + info.suffix();
    QString metaFile;
    QStringList arguments;
    arguments << "-y" << "-v" << "error" << "-i" << candidate.filePath;

    if (oggFamily) {
        // Ogg cannot carry the picture as a stream, so it goes in as a
        // METADATA_BLOCK_PICTURE comment — far too big for a command line,
        // hence a metadata file. The tags already on the file are copied into
        // that same file: -map_metadata replaces the lot, so anything left out
        // here would be lost.
        const QByteArray block = pictureBlockFor(candidate.imagePath);
        if (block.isEmpty()) {
            candidate.problem = QObject::tr("the picture could not be prepared for an Opus file.");
            return false;
        }

        QString probeOutput;
        run(ffprobePath(), {"-v", "error", "-show_entries", "format_tags:stream_tags",
                            "-of", "json", candidate.filePath}, 20000, &probeOutput);

        QMap<QString, QString> tags;
        const QJsonObject root = QJsonDocument::fromJson(probeOutput.toUtf8()).object();
        auto collect = [&tags](const QJsonObject &object) {
            const QJsonObject found = object.value(QStringLiteral("tags")).toObject();
            for (auto it = found.begin(); it != found.end(); ++it) {
                const QString key = it.key().toUpper();
                if (key == QLatin1String("METADATA_BLOCK_PICTURE")) continue;
                tags.insert(it.key(), it.value().toString());
            }
        };
        collect(root.value(QStringLiteral("format")).toObject());
        for (const QJsonValue &stream : root.value(QStringLiteral("streams")).toArray())
            collect(stream.toObject());

        metaFile = candidate.filePath + QStringLiteral(".cover.ffmeta");
        QFile meta(metaFile);
        if (!meta.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            candidate.problem = QObject::tr("a temporary metadata file could not be written.");
            return false;
        }
        meta.write(";FFMETADATA1\n");
        for (auto it = tags.begin(); it != tags.end(); ++it) {
            QByteArray key = it.key().toUtf8();
            QByteArray value = it.value().toUtf8();
            // '=', ';', '#' and newlines all mean something in ffmetadata.
            for (const char character : {'=', ';', '#', '\\'}) {
                key.replace(character, QByteArray("\\") + character);
                value.replace(character, QByteArray("\\") + character);
            }
            value.replace('\n', "\\\n");
            meta.write(key + "=" + value + "\n");
        }
        QByteArray escaped = block;
        escaped.replace('=', "\\=");
        meta.write("METADATA_BLOCK_PICTURE=");
        meta.write(escaped);
        meta.write("\n");
        meta.close();

        // Audio only: mapping a picture stream is exactly what the ogg muxer
        // refuses, and here there is no picture stream to map anyway.
        arguments << "-f" << "ffmetadata" << "-i" << metaFile
                  << "-map" << "0:a" << "-map_metadata" << "1" << "-c" << "copy";
    } else {
        // MP3 and M4A hold a real attached picture, and the tags travel with
        // the streams, so nothing has to be copied out and back.
        arguments << "-i" << candidate.imagePath
                  << "-map" << "0:a" << "-map" << "1:v"
                  << "-c" << "copy" << "-disposition:v:0" << "attached_pic"
                  // Without these the APIC frame goes in as picture type 0,
                  // "Other", and a player looking for the front cover finds
                  // nothing. This is the documented way to say which it is.
                  << "-metadata:s:v" << "title=Album cover"
                  << "-metadata:s:v" << "comment=Cover (front)";
        if (suffix == QLatin1String("mp3")) {
            arguments << "-id3v2_version" << "3";
            // Keeps the no-Xing-header property the downloader is careful
            // about; a Xing header added here stalls Qt's FFmpeg backend on
            // some MP3s.
            arguments << "-write_xing" << "0";
        }
    }
    arguments << written;

    QString output;
    const bool ok = run(ffmpeg, arguments, 120000, &output)
                    && QFileInfo(written).size() > 0;
    if (!metaFile.isEmpty()) QFile::remove(metaFile);

    if (!ok) {
        QFile::remove(written);
        candidate.problem = output.isEmpty()
            ? QObject::tr("ffmpeg could not write the cover in.")
            : QObject::tr("ffmpeg could not write the cover in: %1").arg(output.left(200));
        return false;
    }

    // The original is only removed once there is a whole file to put in its
    // place, and it is put back if the swap does not complete.
    const QString backup = candidate.filePath + QStringLiteral(".cover.backup");
    QFile::remove(backup);
    if (!QFile::rename(candidate.filePath, backup)) {
        QFile::remove(written);
        candidate.problem = QObject::tr("the original file could not be set aside.");
        return false;
    }
    if (!QFile::rename(written, candidate.filePath)) {
        QFile::rename(backup, candidate.filePath);
        QFile::remove(written);
        candidate.problem = QObject::tr("the file with the cover could not be put in place.");
        return false;
    }
    QFile::remove(backup);

    candidate.problem.clear();
    return true;
}
