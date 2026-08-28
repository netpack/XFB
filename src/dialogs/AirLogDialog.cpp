#include "AirLogDialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableView>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

/** A length in milliseconds as mm:ss, or an em dash when it is not known. */
QString durationText(qint64 ms)
{
    if (ms < 0)
        return QStringLiteral("—");
    const qint64 seconds = ms / 1000;
    return QStringLiteral("%1:%2")
        .arg(seconds / 60)
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

/** A length in words, which is what a screen reader can convey in one pass. */
QString spokenDuration(qint64 ms)
{
    if (ms < 0)
        return AirLogDialog::tr("unknown length");
    const qint64 seconds = ms / 1000;
    const qint64 minutes = seconds / 60;
    if (minutes <= 0)
        return AirLogDialog::tr("%n seconds", nullptr, int(seconds));
    return AirLogDialog::tr("%1 and %2")
        .arg(AirLogDialog::tr("%n minutes", nullptr, int(minutes)),
             AirLogDialog::tr("%n seconds", nullptr, int(seconds % 60)));
}

/** One CSV field, quoted the way a spreadsheet expects. */
QString csvField(const QString &value)
{
    QString out = value;
    out.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + out + QLatin1Char('"');
}

const QString kDisplayFormat = QStringLiteral("yyyy-MM-dd HH:mm");

} // namespace

AirLogDialog::AirLogDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("As-Run Log"));
    setModal(false);            // reading the log must not stop the station
    setAccessibleName(tr("As-run log"));
    setAccessibleDescription(
        tr("A record of everything that went to air, and a report of when a "
           "given advertisement ran."));

    auto *layout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);
    tabs->setAccessibleName(tr("As-run log sections"));
    tabs->addTab(buildLogTab(), tr("&Log"));
    tabs->addTab(buildReportTab(), tr("&Advertiser report"));
    layout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setAccessibleName(tr("Close the as-run log"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    resize(980, 620);

    reloadAdvertisements();
    refreshLog();
}

// ------------------------------------------------------------- the log tab --

QWidget *AirLogDialog::buildLogTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("Everything XFB put to air, newest first. The times are the times "
           "the item actually started and stopped, not the times it was "
           "scheduled for."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    // --- filters ---
    auto *filterBox = new QGroupBox(tr("Show"), page);
    filterBox->setAccessibleName(tr("Log filters"));
    auto *filterLayout = new QFormLayout(filterBox);

    const QDateTime now = QDateTime::currentDateTime();

    m_from = new QDateTimeEdit(now.addDays(-7), filterBox);
    m_from->setCalendarPopup(true);
    m_from->setDisplayFormat(kDisplayFormat);
    m_from->setAccessibleName(tr("Show entries from"));
    m_from->setAccessibleDescription(tr("The start of the period to list, "
                                        "date and time of day."));
    filterLayout->addRow(tr("&From:"), m_from);

    m_to = new QDateTimeEdit(now, filterBox);
    m_to->setCalendarPopup(true);
    m_to->setDisplayFormat(kDisplayFormat);
    m_to->setAccessibleName(tr("Show entries until"));
    m_to->setAccessibleDescription(tr("The end of the period to list, "
                                      "date and time of day."));
    filterLayout->addRow(tr("&To:"), m_to);

    m_source = new QComboBox(filterBox);
    m_source->addItem(tr("Everything"), QString());
    for (const QString &key : AirLog::sourceKeys())
        m_source->addItem(AirLog::sourceLabel(key), key);
    m_source->setAccessibleName(tr("Kind of item"));
    m_source->setAccessibleDescription(
        tr("Limit the list to music, jingles, advertisements, programmes, "
           "pads or other files."));
    filterLayout->addRow(tr("&Kind:"), m_source);

    m_search = new QLineEdit(filterBox);
    m_search->setClearButtonEnabled(true);
    m_search->setPlaceholderText(tr("artist, title or file name"));
    m_search->setAccessibleName(tr("Search text"));
    m_search->setAccessibleDescription(
        tr("Only entries whose artist, title or file name contain this text."));
    filterLayout->addRow(tr("&Search:"), m_search);

    layout->addWidget(filterBox);

    auto *actions = new QHBoxLayout;
    m_refresh = new QPushButton(tr("&Show these entries"), page);
    m_refresh->setAccessibleName(tr("Show these entries"));
    m_refresh->setDefault(true);
    m_exportLog = new QPushButton(tr("&Export as CSV..."), page);
    m_exportLog->setAccessibleName(tr("Export the listed entries as a CSV file"));
    actions->addWidget(m_refresh);
    actions->addWidget(m_exportLog);
    actions->addStretch();
    layout->addLayout(actions);

    // --- the table ---
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({
        tr("Started"), tr("Ended"), tr("Kind"), tr("Artist"), tr("Title"),
        tr("Planned"), tr("Played"), tr("Ended because"), tr("Chosen by"),
        tr("File")
    });
    m_table = new QTableView(page);
    m_table->setModel(m_model);
    prepareTable(m_table, tr("As-run entries"),
                 tr("One row per item that went to air. Use the arrow keys to "
                    "move between rows and columns; press a column header with "
                    "the space bar to sort by it."));
    layout->addWidget(m_table, 1);

    m_summary = new QLabel(page);
    m_summary->setWordWrap(true);
    m_summary->setAccessibleName(tr("How many entries are listed"));
    layout->addWidget(m_summary);

    // --- retention ---
    auto *retentionBox = new QGroupBox(tr("How long the log is kept"), page);
    retentionBox->setAccessibleName(tr("Log retention"));
    auto *retentionLayout = new QHBoxLayout(retentionBox);
    auto *retentionLabel = new QLabel(tr("&Keep entries for:"), retentionBox);
    m_retention = new QSpinBox(retentionBox);
    m_retention->setRange(0, 36500);
    m_retention->setSuffix(tr(" days"));
    m_retention->setSpecialValueText(tr("forever"));
    m_retention->setValue(AirLog::retentionDays());
    m_retention->setAccessibleName(tr("Days of log to keep"));
    m_retention->setAccessibleDescription(
        tr("Older entries are removed when XFB starts. Set it to zero to keep "
           "the log forever. Advertisers may ask for proof months later, so "
           "keep it at least as long as your invoices are disputed."));
    retentionLabel->setBuddy(m_retention);
    retentionLayout->addWidget(retentionLabel);
    retentionLayout->addWidget(m_retention);
    retentionLayout->addStretch();
    layout->addWidget(retentionBox);

    connect(m_refresh, &QPushButton::clicked, this, &AirLogDialog::refreshLog);
    connect(m_exportLog, &QPushButton::clicked, this, &AirLogDialog::exportLogCsv);
    connect(m_search, &QLineEdit::returnPressed, this, &AirLogDialog::refreshLog);
    connect(m_source, &QComboBox::currentIndexChanged, this, &AirLogDialog::refreshLog);
    connect(m_retention, &QSpinBox::valueChanged, this, &AirLogDialog::applyRetention);

    return page;
}

void AirLogDialog::prepareTable(QTableView *view, const QString &name,
                                const QString &description)
{
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSortingEnabled(true);
    view->setAlternatingRowColors(true);
    view->setTabKeyNavigation(false);   // Tab leaves the table; arrows move in it
    view->setWordWrap(false);
    view->setAccessibleName(name);
    view->setAccessibleDescription(description);
    view->horizontalHeader()->setStretchLastSection(true);
    view->horizontalHeader()->setSectionsClickable(true);
    view->horizontalHeader()->setAccessibleName(AirLogDialog::tr("Columns"));
    // A blind operator navigates by row, so the row header is dead weight and
    // one more thing to arrow past.
    view->verticalHeader()->setVisible(false);
}

void AirLogDialog::refreshLog()
{
    AirLog::Filter filter;
    filter.from = m_from->dateTime();
    filter.to = m_to->dateTime();
    filter.source = m_source->currentData().toString();
    filter.text = m_search->text().trimmed();

    const QList<AirLog::Record> rows = AirLog::query(filter);

    // Sorting is re-applied after the fill, so switch it off while the model
    // is being rebuilt — otherwise every appended row triggers a sort.
    m_table->setSortingEnabled(false);
    m_model->removeRows(0, m_model->rowCount());

    for (const AirLog::Record &rec : rows) {
        QList<QStandardItem *> items;
        auto *started = new QStandardItem(
            rec.startedAt.isValid() ? rec.startedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                    : QString());
        // Sort on the instant, not on the rendered text: two rows either side
        // of a daylight-saving change do not sort correctly as strings.
        started->setData(rec.startedAt, Qt::UserRole + 1);
        items << started;

        items << new QStandardItem(rec.endedAt.isValid()
                                       ? rec.endedAt.toString(QStringLiteral("HH:mm:ss"))
                                       : tr("still on air"));
        items << new QStandardItem(AirLog::sourceLabel(rec.source));
        items << new QStandardItem(rec.artist);
        items << new QStandardItem(rec.title);
        items << new QStandardItem(durationText(rec.plannedMs));
        items << new QStandardItem(durationText(rec.playedMs));
        items << new QStandardItem(AirLog::reasonLabel(rec.endedReason));
        items << new QStandardItem(rec.operatorMode == QLatin1String("auto")
                                       ? tr("Auto Mode") : tr("Operator"));
        items << new QStandardItem(rec.path);

        // Every cell speaks the whole row: a screen reader reading column 6
        // alone would say "3:41" and nothing else.
        const QString spoken =
            tr("%1, %2, started %3, played %4, %5, chosen by %6")
                .arg(rec.title.isEmpty() ? tr("untitled") : rec.title,
                     rec.artist.isEmpty() ? AirLog::sourceLabel(rec.source) : rec.artist,
                     rec.startedAt.isValid()
                         ? QLocale().toString(rec.startedAt, QLocale::ShortFormat)
                         : tr("at an unknown time"),
                     spokenDuration(rec.playedMs),
                     AirLog::reasonLabel(rec.endedReason),
                     rec.operatorMode == QLatin1String("auto") ? tr("Auto Mode")
                                                               : tr("the operator"));
        for (QStandardItem *item : std::as_const(items)) {
            item->setEditable(false);
            item->setData(spoken, Qt::AccessibleTextRole);
            item->setToolTip(spoken);
        }
        m_model->appendRow(items);
    }

    m_table->setSortingEnabled(true);
    m_table->resizeColumnsToContents();

    const QString summary = rows.isEmpty()
        ? tr("Nothing aired in that period.")
        : tr("%n entries listed.", nullptr, int(rows.size()));
    m_summary->setText(summary);
    emit announcementRequested(summary);
}

void AirLogDialog::exportLogCsv()
{
    const int rowCount = m_model->rowCount();
    if (rowCount == 0) {
        QMessageBox::information(this, tr("As-Run Log"),
                                 tr("There is nothing listed to export."));
        return;
    }

    QStringList header;
    for (int col = 0; col < m_model->columnCount(); ++col)
        header << m_model->headerData(col, Qt::Horizontal).toString();

    QList<QStringList> rows;
    rows.reserve(rowCount);
    // The table's own order, so what is exported is what is on screen.
    for (int row = 0; row < rowCount; ++row) {
        QStringList cells;
        for (int col = 0; col < m_model->columnCount(); ++col)
            cells << m_model->item(row, col)->text();
        rows << cells;
    }

    const QStringList preamble = {
        tr("XFB as-run log"),
        tr("Period: %1 to %2").arg(m_from->dateTime().toString(kDisplayFormat),
                                   m_to->dateTime().toString(kDisplayFormat)),
        tr("Kind: %1").arg(m_source->currentText()),
        tr("Exported: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)),
    };

    writeCsv(QStringLiteral("xfb-as-run-%1.csv")
                 .arg(QDate::currentDate().toString(Qt::ISODate)),
             preamble, header, rows);
}

void AirLogDialog::applyRetention()
{
    AirLog::setRetentionDays(m_retention->value());
    const QString said = m_retention->value() == 0
        ? tr("The log will be kept forever.")
        : tr("The log will be kept for %n days. Older entries go on the next start.",
             nullptr, m_retention->value());
    m_summary->setText(said);
    emit announcementRequested(said);
}

// -------------------------------------------------- the advertiser report --

QWidget *AirLogDialog::buildReportTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("Pick an advertisement and a period to get the number of times it "
           "aired and the exact time of every airing — the sheet to attach to "
           "an invoice."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *form = new QFormLayout;

    m_advert = new QComboBox(page);
    m_advert->setAccessibleName(tr("Advertisement"));
    m_advert->setAccessibleDescription(
        tr("The advertisement to report on, from the Ads list."));
    form->addRow(tr("&Advertisement:"), m_advert);

    const QDate today = QDate::currentDate();
    m_reportFrom = new QDateTimeEdit(QDateTime(QDate(today.year(), today.month(), 1),
                                               QTime(0, 0)), page);
    m_reportFrom->setCalendarPopup(true);
    m_reportFrom->setDisplayFormat(kDisplayFormat);
    m_reportFrom->setAccessibleName(tr("Report period start"));
    form->addRow(tr("F&rom:"), m_reportFrom);

    m_reportTo = new QDateTimeEdit(QDateTime(today, QTime(23, 59, 59)), page);
    m_reportTo->setCalendarPopup(true);
    m_reportTo->setDisplayFormat(kDisplayFormat);
    m_reportTo->setAccessibleName(tr("Report period end"));
    form->addRow(tr("T&o:"), m_reportTo);

    layout->addLayout(form);

    auto *actions = new QHBoxLayout;
    m_buildReport = new QPushButton(tr("&Build the report"), page);
    m_buildReport->setAccessibleName(tr("Build the report"));
    m_exportReport = new QPushButton(tr("Ex&port as CSV..."), page);
    m_exportReport->setAccessibleName(tr("Export the report as a CSV file"));
    actions->addWidget(m_buildReport);
    actions->addWidget(m_exportReport);
    actions->addStretch();
    layout->addLayout(actions);

    m_reportHeader = new QTextBrowser(page);
    m_reportHeader->setMaximumHeight(150);
    m_reportHeader->setAccessibleName(tr("Report summary"));
    m_reportHeader->setAccessibleDescription(
        tr("Who the report is for, the period it covers and how many times "
           "the advertisement aired."));
    layout->addWidget(m_reportHeader);

    m_reportModel = new QStandardItemModel(this);
    m_reportModel->setHorizontalHeaderLabels({
        tr("No."), tr("Date"), tr("Time"), tr("Length aired"), tr("Ended because"),
        tr("Chosen by")
    });
    m_reportTable = new QTableView(page);
    m_reportTable->setModel(m_reportModel);
    prepareTable(m_reportTable, tr("Airings"),
                 tr("One row per airing, oldest first. Arrow keys move "
                    "between rows and columns."));
    layout->addWidget(m_reportTable, 1);

    connect(m_buildReport, &QPushButton::clicked, this, &AirLogDialog::refreshReport);
    connect(m_exportReport, &QPushButton::clicked, this, &AirLogDialog::exportReportCsv);
    connect(m_advert, &QComboBox::currentIndexChanged, this, [this]() {
        m_reportModel->removeRows(0, m_reportModel->rowCount());
        m_reportRows.clear();
        m_reportHeader->clear();
    });

    return page;
}

void AirLogDialog::reloadAdvertisements()
{
    m_advert->clear();
    const auto ads = AirLog::advertisements();
    for (const auto &ad : ads)
        m_advert->addItem(ad.second.isEmpty()
                              ? tr("Advertisement %1").arg(ad.first) : ad.second,
                          ad.first);
    if (ads.isEmpty()) {
        m_advert->addItem(tr("(no advertisements in the library)"), -1);
        m_advert->setEnabled(false);
        if (m_buildReport)
            m_buildReport->setEnabled(false);
    }
}

void AirLogDialog::refreshReport()
{
    const qint64 pubId = m_advert->currentData().toLongLong();
    if (pubId < 0)
        return;

    const QDateTime from = m_reportFrom->dateTime();
    const QDateTime to = m_reportTo->dateTime();
    m_reportRows = AirLog::advertiserReport(pubId, from, to);
    m_reportTitle = m_advert->currentText();

    m_reportTable->setSortingEnabled(false);
    m_reportModel->removeRows(0, m_reportModel->rowCount());

    qint64 totalMs = 0;
    int number = 0;
    for (const AirLog::Record &rec : std::as_const(m_reportRows)) {
        ++number;
        if (rec.playedMs > 0)
            totalMs += rec.playedMs;

        auto *index = new QStandardItem;
        index->setData(number, Qt::DisplayRole);   // sorts numerically, not as text
        QList<QStandardItem *> items;
        items << index;
        items << new QStandardItem(rec.startedAt.toString(QStringLiteral("yyyy-MM-dd")));
        items << new QStandardItem(rec.startedAt.toString(QStringLiteral("HH:mm:ss")));
        items << new QStandardItem(durationText(rec.playedMs));
        items << new QStandardItem(AirLog::reasonLabel(rec.endedReason));
        items << new QStandardItem(rec.operatorMode == QLatin1String("auto")
                                       ? tr("Auto Mode") : tr("Operator"));

        const QString spoken = tr("Airing %1, %2, lasting %3, %4")
            .arg(QString::number(number),
                 QLocale().toString(rec.startedAt, QLocale::LongFormat),
                 spokenDuration(rec.playedMs),
                 AirLog::reasonLabel(rec.endedReason));
        for (QStandardItem *item : std::as_const(items)) {
            item->setEditable(false);
            item->setData(spoken, Qt::AccessibleTextRole);
            item->setToolTip(spoken);
        }
        m_reportModel->appendRow(items);
    }

    m_reportTable->setSortingEnabled(true);
    m_reportTable->resizeColumnsToContents();

    // The summary is what somebody actually reads before signing the invoice,
    // so it says the four things a dispute turns on: who, what, when, how many.
    const QString stationName = QStringLiteral("XFB");
    const QString html = QStringLiteral(
        "<h3>%1</h3><p><b>%2</b><br>%3<br>%4<br>%5</p>")
        .arg(tr("Airing certificate"),
             tr("Advertisement: %1").arg(m_reportTitle.toHtmlEscaped()),
             tr("Period: %1 to %2").arg(QLocale().toString(from, QLocale::LongFormat),
                                        QLocale().toString(to, QLocale::LongFormat)),
             tr("Times aired: %1").arg(m_reportRows.size()),
             tr("Total airtime: %1 — issued by %2 on %3")
                 .arg(spokenDuration(totalMs), stationName,
                      QLocale().toString(QDateTime::currentDateTime(), QLocale::LongFormat)));
    m_reportHeader->setHtml(html);

    const QString said = m_reportRows.isEmpty()
        ? tr("%1 did not air in that period.").arg(m_reportTitle)
        : tr("%1 aired %n times in that period.", nullptr, int(m_reportRows.size()))
              .arg(m_reportTitle);
    m_reportHeader->setAccessibleDescription(said);
    emit announcementRequested(said);
}

void AirLogDialog::exportReportCsv()
{
    if (m_reportRows.isEmpty()) {
        QMessageBox::information(this, tr("Advertiser Report"),
                                 tr("Build a report first — there is nothing to export."));
        return;
    }

    qint64 totalMs = 0;
    for (const AirLog::Record &rec : std::as_const(m_reportRows))
        if (rec.playedMs > 0)
            totalMs += rec.playedMs;

    const QStringList preamble = {
        tr("XFB airing certificate"),
        tr("Advertisement: %1").arg(m_reportTitle),
        tr("Period: %1 to %2").arg(m_reportFrom->dateTime().toString(kDisplayFormat),
                                   m_reportTo->dateTime().toString(kDisplayFormat)),
        tr("Times aired: %1").arg(m_reportRows.size()),
        tr("Total airtime: %1").arg(durationText(totalMs)),
        tr("Issued: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)),
    };

    const QStringList header = {
        tr("No."), tr("Date"), tr("Time"), tr("Started (ISO 8601)"),
        tr("Length aired"), tr("Ended because"), tr("Chosen by"),
    };

    QList<QStringList> rows;
    int number = 0;
    for (const AirLog::Record &rec : std::as_const(m_reportRows)) {
        ++number;
        rows << QStringList{
            QString::number(number),
            rec.startedAt.toString(QStringLiteral("yyyy-MM-dd")),
            rec.startedAt.toString(QStringLiteral("HH:mm:ss")),
            AirLog::formatTimestamp(rec.startedAt),
            durationText(rec.playedMs),
            AirLog::reasonLabel(rec.endedReason),
            rec.operatorMode == QLatin1String("auto") ? tr("Auto Mode") : tr("Operator"),
        };
    }

    QString slug = m_reportTitle;
    slug.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")),
                 QStringLiteral("-"));
    writeCsv(QStringLiteral("xfb-airings-%1-%2.csv")
                 .arg(slug, QDate::currentDate().toString(Qt::ISODate)),
             preamble, header, rows);
}

// ------------------------------------------------------------------- CSV ---

bool AirLogDialog::writeCsv(const QString &suggestedName,
                            const QStringList &preamble,
                            const QStringList &header,
                            const QList<QStringList> &rows)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export as CSV"),
        dir.isEmpty() ? suggestedName : dir + QLatin1Char('/') + suggestedName,
        tr("Comma-separated values (*.csv)"));
    if (path.isEmpty())
        return false;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export as CSV"),
                             tr("Could not write %1: %2").arg(path, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    // A BOM: without it Excel opens accented artist names as mojibake, and
    // this file is going to a client.
    out.setGenerateByteOrderMark(true);

    for (const QString &line : preamble)
        out << csvField(line) << "\n";
    if (!preamble.isEmpty())
        out << "\n";

    QStringList headerFields;
    for (const QString &cell : header)
        headerFields << csvField(cell);
    out << headerFields.join(QLatin1Char(',')) << "\n";

    for (const QStringList &row : rows) {
        QStringList fields;
        for (const QString &cell : row)
            fields << csvField(cell);
        out << fields.join(QLatin1Char(',')) << "\n";
    }
    out.flush();

    if (!file.commit()) {
        QMessageBox::warning(this, tr("Export as CSV"),
                             tr("Could not write %1: %2").arg(path, file.errorString()));
        return false;
    }

    const QString said = tr("Exported %n rows to %1.", nullptr, int(rows.size()))
                             .arg(QFileInfo(path).fileName());
    emit announcementRequested(said);
    QMessageBox::information(this, tr("Export as CSV"), said);
    return true;
}
