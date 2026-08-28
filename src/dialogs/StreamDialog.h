#ifndef STREAMDIALOG_H
#define STREAMDIALOG_H

#include <QDialog>
#include <QVector>

#include "../services/StreamService.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

/**
 * @brief Where the operator sets up XFB's own way onto the air.
 *
 * The window is a list of mounts on the left and the settings for whichever
 * one is selected on the right, because a station that streams at all
 * usually streams the same programme twice — a fat MP3 for the website and
 * a thin Opus for phones — and setting the second one up should be copying
 * the first, not filling a second form from scratch.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h files in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class StreamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StreamDialog(StreamService *service, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear the state changes too. */
    void announcementRequested(const QString &message);

private slots:
    void selectMount(int row);
    void addMount();
    void duplicateMount();
    void removeMount();
    void applyChanges();
    void toggleOnAir();
    void refreshStatus();

private:
    void buildUi();
    void loadIntoForm(const StreamService::Mount &mount);
    void storeFromForm(StreamService::Mount &mount) const;
    /** Pull the form into m_mounts[m_current] without touching the service. */
    void commitCurrent();
    void refreshList();
    void appendLog(const QString &message);
    void updateBitrateChoices();

    StreamService *m_service = nullptr;
    QVector<StreamService::Mount> m_mounts;
    int m_current = -1;
    bool m_loading = false;   // suppress edit signals while filling the form

    QListWidget *m_list = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_duplicateButton = nullptr;
    QPushButton *m_removeButton = nullptr;

    QCheckBox *m_enabled = nullptr;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox  *m_port = nullptr;
    QLineEdit *m_mount = nullptr;
    QLineEdit *m_user = nullptr;
    QLineEdit *m_password = nullptr;
    QCheckBox *m_usePut = nullptr;
    QComboBox *m_codec = nullptr;
    QComboBox *m_bitrate = nullptr;

    QLineEdit *m_stationName = nullptr;
    QLineEdit *m_genre = nullptr;
    QLineEdit *m_description = nullptr;
    QLineEdit *m_url = nullptr;
    QCheckBox *m_public = nullptr;

    QCheckBox *m_autoStart = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_onAirButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_nowPlayingLabel = nullptr;
    QPlainTextEdit *m_log = nullptr;
};

#endif // STREAMDIALOG_H
