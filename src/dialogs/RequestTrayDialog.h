#ifndef REQUESTTRAYDIALOG_H
#define REQUESTTRAYDIALOG_H

#include <QDialog>
#include <QList>

#include "../services/RequestLine.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTimer;

class MobileSyncServer;

/**
 * @brief What listeners have asked for, and the switch that puts the public
 *        page on the network in the first place.
 *
 * One window rather than two, because the two halves are the same decision:
 * an operator turning the page on wants to see immediately what starts
 * arriving, and an operator reading requests wants the switch that stops them
 * within reach.
 *
 * Nothing here happens on its own. A request becomes airtime when the person
 * sitting in front of this window presses a button, and not before — which is
 * also why "Add to the playlist" only adds, and never plays.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h headers in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class RequestTrayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RequestTrayDialog(MobileSyncServer *server, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear status changes too. */
    void announcementRequested(const QString &message);

    /**
     * Put this file at the end of the running order. The player decides what
     * that means; this window only asks, exactly as the listener did.
     */
    void addToPlaylistRequested(const QString &path);

private slots:
    void refresh();
    void applySettings();
    void addSelectedToPlaylist();
    void markSelectedPlayed();
    void dismissSelected();
    void showSelectedDetail();

private:
    void buildUi();
    void loadSettings();
    void refreshAddresses();
    /** The row the operator has selected, or a default-constructed one. */
    RequestLine::Entry selectedEntry() const;
    void setSelectedStatus(const QString &status, const QString &spoken);

    MobileSyncServer *m_server = nullptr;

    // the switch, and the warning that goes with it
    QCheckBox      *m_pageEnabled = nullptr;
    QCheckBox      *m_requestsEnabled = nullptr;
    QLineEdit      *m_stationName = nullptr;
    QLineEdit      *m_tagline = nullptr;
    QLabel         *m_addresses = nullptr;
    QPushButton    *m_serveButton = nullptr;

    // the tray
    QTableWidget   *m_table = nullptr;
    QCheckBox      *m_showHandled = nullptr;
    QPlainTextEdit *m_detail = nullptr;
    QPushButton    *m_addButton = nullptr;
    QPushButton    *m_playedButton = nullptr;
    QPushButton    *m_dismissButton = nullptr;
    QLabel         *m_summary = nullptr;

    QList<RequestLine::Entry> m_rows;
    QTimer *m_refreshTimer = nullptr;
    /// Suppresses applySettings() while loadSettings() is filling the form.
    bool m_loading = false;
};

#endif // REQUESTTRAYDIALOG_H
