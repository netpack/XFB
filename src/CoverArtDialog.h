#ifndef COVERARTDIALOG_H
#define COVERARTDIALOG_H

#include "services/CoverArtFetcher.h"

#include <QAtomicInt>
#include <QDialog>
#include <QVector>

class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;

/**
 * Finds the tracks in the library with no cover art and offers to put one on
 * them, a step at a time and never without being shown first.
 *
 * Three buttons in the order the work happens, because each step costs
 * something different: reading every file on the desk, asking the internet
 * about each track, and rewriting the files. Nothing is written until the
 * operator has looked at what was found and left it ticked — a search by
 * artist and title is a good guess and not more than that.
 */
class CoverArtDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoverArtDialog(QWidget *parent = nullptr);

private slots:
    void scanLibrary();
    void findCovers();
    void writeCovers();
    void cancelWork();

private:
    void setWorking(bool working, const QString &what = QString());
    void refreshList();
    void report(const QString &message);
    QString describe(const CoverArtFetcher::Candidate &candidate) const;

    QLabel *m_explanation = nullptr;
    QListWidget *m_list = nullptr;
    QProgressBar *m_progress = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_findButton = nullptr;
    QPushButton *m_writeButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPushButton *m_closeButton = nullptr;

    QVector<CoverArtFetcher::Candidate> m_candidates;
    /// Why the scan came back with nothing, when that was a fault rather than
    /// an answer. Written by the worker, read once it has finished.
    QString m_scanError;
    /// Read from the worker thread, written from the GUI one.
    QAtomicInt m_cancelled{0};
    bool m_busy = false;
};

#endif // COVERARTDIALOG_H
