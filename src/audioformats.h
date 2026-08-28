#ifndef AUDIOFORMATS_H
#define AUDIOFORMATS_H

#include <QString>
#include <QStringList>

// One definition of "an audio file XFB will import", used by every place that
// scans a folder, filters a file dialog or decides whether a path is playable
// material. Before this existed each site carried its own hand-written list:
// the folder importer knew about Opus, the programme scanner did not, the
// "Audio Files" dialogs disagreed with both, and the artist/song splitter
// stripped five extensions out of the nine it was being handed.
//
// Matching is always case-insensitive and always done on the file's suffix,
// never on wildcard semantics that differ between platforms.
namespace AudioFormats {

// Lowercase, no leading dot. The canonical list.
const QStringList &suffixes();

// "*.mp3", "*.ogg", ... — for QDir/QDirIterator name filters. Prefer
// findAudioFiles() below when you are walking a folder.
const QStringList &nameFilters();

// "Audio Files (*.mp3 *.ogg ...)" — the first entry of a file dialog's
// filter list. Translated.
QString fileDialogFilter();

// fileDialogFilter() followed by an "All Files (*)" entry.
QStringList fileDialogFilters();

// The same two entries as one ";;"-joined string, for the QFileDialog
// convenience functions (getOpenFileName and friends).
QString fileDialogFilterString();

// True when the path's extension is one XFB imports. Case-insensitive.
// Takes a file name or a full path; the file need not exist.
bool isAudioFile(const QString &filePath);

// The file's name with a trailing known audio extension removed, whatever the
// case it was written in ("TRACK.OPUS" -> "TRACK"). A name that does not end
// in a known extension is returned unchanged, so a song title that happens to
// contain a dot survives intact.
QString stripSuffix(const QString &fileNameOrPath);

// Splits "Artist - Song.opus" into its two halves, extension removed. Only the
// first dash separates: "Artist - Song - Live" keeps "Song - Live" as the
// title. When there is no dash the whole (de-extensioned) name is the artist
// and the song comes back empty — the callers decide what to store then.
void splitArtistAndSong(const QString &fileNameOrPath,
                        QString *artist,
                        QString *song);

// Every audio file under `directory`. Recursive by default.
//
// Symlinked folders ARE followed (a station library is very often a tree of
// links into other volumes), with loop protection: each directory is entered
// at most once, keyed on its canonical path, so a link pointing back at an
// ancestor terminates instead of walking for ever. The paths returned are
// built from the directory as the caller spelled it — the link path, not its
// target — because that is what goes in the database.
//
// Returns absolute paths, sorted, no duplicates.
QStringList findAudioFiles(const QString &directory,
                           bool recursive = true,
                           bool followSymlinks = true);

} // namespace AudioFormats

#endif // AUDIOFORMATS_H
