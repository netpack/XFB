#ifndef EXTERNALDOWNLOADER_H
#define EXTERNALDOWNLOADER_H

#include <QWidget>
#include <QtSql>
#include <QtDebug>
#include <QFileInfo>
#include <QFutureWatcher>

#include "streamingcatalog.h"

class QProcess;
class QTimer;

namespace Ui {
class externaldownloader;
}

/**
 * Where yt-dlp actually is on this machine, or an empty string when it is
 * nowhere XFB knows to look. Preference order: the self-updating copy XFB
 * keeps in ~/.local/bin, then PATH, then the usual install locations (a GUI
 * app on macOS does not inherit the shell's PATH). Shared with the options
 * dialog, which reports it under Diagnostics.
 */
QString findYtDlpExecutable();

class externaldownloader : public QWidget
{
    Q_OBJECT

public:
    explicit externaldownloader(QWidget *parent = 0);
    ~externaldownloader();
     QSqlDatabase adb;

signals:
    void musicAdded(); // Signal emitted when music is successfully added to database

private slots:
    void on_bt_youtube_getIt_clicked();
    void on_bt_youtube_getPlaylist_clicked();
    void on_pushButton_clicked();
    void getFile();
    void getPlaylist();
    void showLoadingFrame();
    void on_bt_close_clicked();
    void on_bt_clear_clicked();

    /** Scrape title/artist for the pasted link and auto-fill the fields. */
    void fetchVideoDetails();

private:
    /** Auto-fill path for Spotify / Apple Music links, which yt-dlp can't read. */
    void fetchStreamingDetails(const QString &url);

    /**
     * Put scraped details into the Artist/Song fields, leaving anything the
     * user typed themselves alone.
     */
    void applyAutoFill(const QString &artist, const QString &song);

    Ui::externaldownloader *ui;

    // Make sure the external tools a download needs (yt-dlp and, crucially,
    // ffmpeg/ffprobe for audio extraction) are present BEFORE the download is
    // handed to the background worker. The worker runs off the UI thread and
    // cannot show install prompts, so if ffmpeg is missing there yt-dlp fails
    // post-processing with "ffprobe and ffmpeg not found". Doing it here lets
    // XFB install the dependencies on demand (Homebrew on macOS, etc.). Returns
    // true when the download may proceed.
    bool ensureDownloadDependencies();

    // Auto-fill of artist/song from a pasted link
    QTimer *m_metaDebounce = nullptr;
    QProcess *m_metaFetch = nullptr;
    QFutureWatcher<StreamingCatalog::Listing> *m_streamingFetch = nullptr;
    QString m_lastAutoArtist;
    QString m_lastAutoSong;
};

#endif // EXTERNALDOWNLOADER_H
