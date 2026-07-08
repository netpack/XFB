#ifndef EXTERNALDOWNLOADER_H
#define EXTERNALDOWNLOADER_H

#include <QWidget>
#include <QtSql>
#include <QtDebug>
#include <QFileInfo>



class QProcess;
class QTimer;

namespace Ui {
class externaldownloader;
}

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
    QString m_lastAutoArtist;
    QString m_lastAutoSong;
};

#endif // EXTERNALDOWNLOADER_H
