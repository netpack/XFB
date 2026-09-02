#include "CoverArtDialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QVBoxLayout>
#include <QtConcurrent>

CoverArtDialog::CoverArtDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Cover art"));
    setModal(false);
    resize(720, 560);

    auto *layout = new QVBoxLayout(this);

    m_explanation = new QLabel(
        tr("XFB has embedded a cover into every download since the tagging pass "
           "learned to keep one. Anything downloaded before that has none, and "
           "nothing on this desk knows what it looked like — so the picture has "
           "to be fetched again.\n\n"
           "Nothing is written to your files until you have seen what was found "
           "and left it ticked."), this);
    m_explanation->setWordWrap(true);
    layout->addWidget(m_explanation);

    m_list = new QListWidget(this);
    m_list->setIconSize(QSize(72, 72));
    m_list->setAccessibleName(tr("Tracks with no cover art"));
    m_list->setAccessibleDescription(
        tr("Each row is one track. Tick a row to have its picture written into "
           "the file."));
    layout->addWidget(m_list, 1);

    m_progress = new QProgressBar(this);
    m_progress->setVisible(false);
    m_progress->setAccessibleName(tr("Progress"));
    layout->addWidget(m_progress);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setAccessibleName(tr("Status"));
    layout->addWidget(m_status);

    auto *buttons = new QHBoxLayout;
    m_scanButton  = new QPushButton(tr("&Find tracks with no cover"), this);
    m_findButton  = new QPushButton(tr("&Look for their covers"), this);
    m_writeButton = new QPushButton(tr("&Write the ticked ones in"), this);
    m_cancelButton = new QPushButton(tr("&Stop"), this);
    m_closeButton = new QPushButton(tr("&Close"), this);
    m_findButton->setEnabled(false);
    m_writeButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
    buttons->addWidget(m_scanButton);
    buttons->addWidget(m_findButton);
    buttons->addWidget(m_writeButton);
    buttons->addStretch(1);
    buttons->addWidget(m_cancelButton);
    buttons->addWidget(m_closeButton);
    layout->addLayout(buttons);

    connect(m_scanButton,  &QPushButton::clicked, this, &CoverArtDialog::scanLibrary);
    connect(m_findButton,  &QPushButton::clicked, this, &CoverArtDialog::findCovers);
    connect(m_writeButton, &QPushButton::clicked, this, &CoverArtDialog::writeCovers);
    connect(m_cancelButton, &QPushButton::clicked, this, &CoverArtDialog::cancelWork);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);

    report(tr("Ready. Start by finding the tracks that have no cover."));
}

void CoverArtDialog::report(const QString &message)
{
    m_status->setText(message);
    // The status line is the running commentary for anyone not watching the
    // list, so it is also what the window announces itself as.
    setAccessibleDescription(message);
}

QString CoverArtDialog::describe(const CoverArtFetcher::Candidate &candidate) const
{
    QString label = candidate.artist.isEmpty()
        ? candidate.song
        : candidate.artist + QStringLiteral(" — ") + candidate.song;
    if (label.trimmed().isEmpty())
        label = QFileInfo(candidate.filePath).fileName();

    if (!candidate.problem.isEmpty())
        return tr("%1 — nothing written: %2").arg(label, candidate.problem);
    if (candidate.found())
        return tr("%1 — cover found from %2").arg(label, candidate.foundVia);
    return label;
}

void CoverArtDialog::refreshList()
{
    m_list->clear();
    for (const CoverArtFetcher::Candidate &candidate : m_candidates) {
        auto *item = new QListWidgetItem(describe(candidate), m_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        // Only what was actually found can be written, so only that starts
        // ticked; a row with nothing found is left visible and unticked so the
        // operator can see it was tried.
        item->setCheckState(candidate.found() ? Qt::Checked : Qt::Unchecked);
        if (!candidate.preview.isNull())
            item->setIcon(QIcon(QPixmap::fromImage(candidate.preview)));
        item->setToolTip(candidate.filePath);
        item->setData(Qt::AccessibleTextRole, describe(candidate));
        item->setData(Qt::AccessibleDescriptionRole, candidate.filePath);
    }
}

void CoverArtDialog::setWorking(bool working, const QString &what)
{
    m_busy = working;
    m_scanButton->setEnabled(!working);
    m_findButton->setEnabled(!working && !m_candidates.isEmpty());
    m_writeButton->setEnabled(!working && !m_candidates.isEmpty());
    m_cancelButton->setEnabled(working);
    m_closeButton->setEnabled(!working);
    m_progress->setVisible(working);
    if (working) {
        m_cancelled.storeRelaxed(0);
        m_progress->setRange(0, 0);
        report(what);
    }
}

void CoverArtDialog::cancelWork()
{
    m_cancelled.storeRelaxed(1);
    report(tr("Stopping…"));
}

void CoverArtDialog::scanLibrary()
{
    m_scanError.clear();
    setWorking(true, tr("Reading every track in the library…"));

    auto *watcher = new QFutureWatcher<QVector<CoverArtFetcher::Candidate>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
        m_candidates = watcher->result();
        watcher->deleteLater();
        refreshList();
        setWorking(false);
        if (!m_scanError.isEmpty()) {
            // Never let a failed scan read as good news.
            report(m_scanError);
        } else if (m_candidates.isEmpty()) {
            report(tr("Every track in the library already has a cover."));
        } else {
            report(tr("%n track(s) have no cover. Look for their covers next.",
                      "", m_candidates.size()));
            m_list->setFocus();
        }
    });

    watcher->setFuture(QtConcurrent::run([this]() {
        // A connection of its own: this runs off the GUI thread, and a
        // QSqlDatabase belongs to the thread that opened it.
        QVector<CoverArtFetcher::Candidate> found;
        const QString name = QStringLiteral("covers_scan");
        {
            // Cloned from the connection *name*, not from a QSqlDatabase
            // fetched here: QSqlDatabase::database() refuses to hand a
            // connection to a thread that does not own it, and the clone made
            // from what it returns is invalid. It fails quietly — an empty
            // scan and a cheerful "every track already has a cover" — which is
            // exactly how it got this far unnoticed. The name overload exists
            // for this.
            QSqlDatabase scan = QSqlDatabase::contains(name)
                ? QSqlDatabase::database(name)
                : QSqlDatabase::cloneDatabase(QStringLiteral("xfb_connection"), name);
            if (!scan.isValid()) {
                m_scanError = tr("XFB's library database could not be reached "
                                 "from the scan.");
                return found;
            }
            if (!scan.isOpen() && !scan.open()) {
                m_scanError = tr("The library database could not be opened: %1")
                                  .arg(scan.lastError().text());
                return found;
            }

            found = CoverArtFetcher::findTracksWithoutCover(
                scan, [this](int done, int total, const QString &what) {
                    QMetaObject::invokeMethod(this, [this, done, total, what]() {
                        m_progress->setRange(0, total);
                        m_progress->setValue(done);
                        if (!what.isEmpty())
                            report(tr("Reading %1 (%2 of %3)…").arg(what).arg(done + 1).arg(total));
                    }, Qt::QueuedConnection);
                    return m_cancelled.loadRelaxed() == 0;
                });
        }
        QSqlDatabase::removeDatabase(name);
        return found;
    }));
}

void CoverArtDialog::findCovers()
{
    setWorking(true, tr("Looking for covers…"));

    auto *watcher = new QFutureWatcher<QVector<CoverArtFetcher::Candidate>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
        m_candidates = watcher->result();
        watcher->deleteLater();
        refreshList();
        setWorking(false);

        int found = 0;
        for (const CoverArtFetcher::Candidate &candidate : m_candidates)
            if (candidate.found()) ++found;
        report(tr("Found a cover for %1 of %2. Look through them, untick anything "
                  "that is wrong, then write the rest in.")
                   .arg(found).arg(m_candidates.size()));
        m_list->setFocus();
    });

    const QVector<CoverArtFetcher::Candidate> work = m_candidates;
    watcher->setFuture(QtConcurrent::run([this, work]() {
        QVector<CoverArtFetcher::Candidate> done = work;
        for (int i = 0; i < done.size(); ++i) {
            if (m_cancelled.loadRelaxed() != 0) break;
            if (done[i].found()) continue;    // already have one from a previous pass

            const QString label = done[i].song;
            QMetaObject::invokeMethod(this, [this, i, label, total = done.size()]() {
                m_progress->setRange(0, total);
                m_progress->setValue(i);
                report(tr("Looking for the cover of %1 (%2 of %3)…")
                           .arg(label).arg(i + 1).arg(total));
            }, Qt::QueuedConnection);

            CoverArtFetcher::findCoverFor(done[i]);
        }
        return done;
    }));
}

void CoverArtDialog::writeCovers()
{
    QVector<int> chosen;
    for (int i = 0; i < m_candidates.size() && i < m_list->count(); ++i) {
        if (m_list->item(i)->checkState() == Qt::Checked && m_candidates[i].found())
            chosen.append(i);
    }
    if (chosen.isEmpty()) {
        report(tr("Nothing is ticked that has a cover to write."));
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Write the covers in"),
        tr("This rewrites %n file(s) to add the cover you have seen. The audio "
           "is copied across untouched and the tags are kept.\n\nGo ahead?",
           "", chosen.size()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    setWorking(true, tr("Writing covers in…"));

    auto *watcher = new QFutureWatcher<QVector<CoverArtFetcher::Candidate>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
        m_candidates = watcher->result();
        watcher->deleteLater();

        // What worked leaves the list: it has a cover now, and showing it again
        // would invite writing it twice.
        QVector<CoverArtFetcher::Candidate> left;
        int written = 0;
        for (const CoverArtFetcher::Candidate &candidate : m_candidates) {
            if (candidate.problem.isEmpty() && candidate.found()) ++written;
            else left.append(candidate);
        }
        m_candidates = left;

        refreshList();
        setWorking(false);
        report(tr("Wrote the cover into %1 file(s). %2 left in the list.")
                   .arg(written).arg(m_candidates.size()));
        m_list->setFocus();
    });

    const QVector<CoverArtFetcher::Candidate> work = m_candidates;
    watcher->setFuture(QtConcurrent::run([this, work, chosen]() {
        QVector<CoverArtFetcher::Candidate> done = work;
        int step = 0;
        for (int index : chosen) {
            if (m_cancelled.loadRelaxed() != 0) break;

            const QString label = done[index].song;
            QMetaObject::invokeMethod(this, [this, step, label, total = chosen.size()]() {
                m_progress->setRange(0, total);
                m_progress->setValue(step);
                report(tr("Writing the cover into %1 (%2 of %3)…")
                           .arg(label).arg(step + 1).arg(total));
            }, Qt::QueuedConnection);

            CoverArtFetcher::embedCover(done[index]);
            ++step;
        }
        return done;
    }));
}
