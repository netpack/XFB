#ifndef WATCHEDFOLDERSDIALOG_H
#define WATCHEDFOLDERSDIALOG_H

#include <QDialog>

#include "../services/LibraryWatcher.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;

/**
 * @brief The folders XFB watches, and which library each one feeds.
 *
 * One row per folder: where it is, what lands in it, and — for music — how the
 * genre is decided. The "Waiting" column is what makes the window worth
 * opening: it says how many files in that folder are not in the library yet,
 * so an operator can see the watcher has something to do before trusting it
 * with the overnight delivery.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h headers in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class WatchedFoldersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WatchedFoldersDialog(LibraryWatcher *watcher, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear what happened. */
    void announcementRequested(const QString &message);

private slots:
    void addFolder();
    void removeFolder();
    void apply();
    void scanNow();
    void refreshStatus();
    void refreshPending();

private:
    void buildUi();
    void loadInto(const LibraryWatcher::Config &config);
    /** Appends one row of controls for @p folder. */
    void appendRow(const LibraryWatcher::Folder &folder);
    LibraryWatcher::Folder folderFromRow(int row) const;
    LibraryWatcher::Config readFromForm() const;
    /** Greys the genre controls out on the rows that are not music. */
    void updateRowEnabling(int row);
    /** Which row a cell widget sits in, looked up rather than remembered. */
    int rowOf(const QWidget *cellWidget) const;
    /** The genre names offered in the combo boxes. */
    QStringList knownGenres() const;

    LibraryWatcher *m_watcher = nullptr;

    QCheckBox    *m_enabled  = nullptr;
    QSpinBox     *m_poll     = nullptr;
    QSpinBox     *m_quiet    = nullptr;
    QTableWidget *m_table    = nullptr;
    QPushButton  *m_remove   = nullptr;
    QLabel       *m_status   = nullptr;
    QStringList   m_genres;
};

#endif // WATCHEDFOLDERSDIALOG_H
