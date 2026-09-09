#include "ScheduleDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

/** One CSV field, quoted the way a spreadsheet expects. */
QString csvField(const QString &value)
{
    QString out = value;
    out.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + out + QLatin1Char('"');
}

const QString kDisplayFormat = QStringLiteral("yyyy-MM-dd HH:mm");

/** The name to show for a booking whose item was never given one. */
QString itemName(const ProgrammeSchedule::Rule &rule)
{
    if (!rule.name.isEmpty())
        return rule.name;
    if (!rule.path.isEmpty())
        return QFileInfo(rule.path).fileName();
    return ScheduleDialog::tr("(unnamed)");
}

} // namespace

using ProgrammeSchedule::Airing;
using ProgrammeSchedule::Kind;
using ProgrammeSchedule::Rule;

ScheduleDialog::ScheduleDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("What Is Scheduled"));
    setModal(false);            // reading the timetable must not stop the station
    setAccessibleName(tr("What is scheduled"));
    setAccessibleDescription(
        tr("The advertisements and programmes that are booked to go on air, "
           "when each one is due, and whether XFB can honour the booking."));

    auto *layout = new QVBoxLayout(this);

    auto *tabs = new QTabWidget(this);
    tabs->setAccessibleName(tr("Schedule sections"));
    tabs->addTab(buildUpcomingTab(), tr("&Coming up"));
    tabs->addTab(buildBookingsTab(), tr("&All bookings"));
    layout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)
        ->setAccessibleName(tr("Close the schedule"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);

    resize(980, 620);
}

void ScheduleDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Bookings are made in another window, and this one is kept alive between
    // openings: without this it would still be showing last week.
    const QDateTime now = QDateTime::currentDateTime();
    if (m_from->dateTime() < now.addDays(-1)) {
        m_from->setDateTime(now);
        m_to->setDateTime(now.addDays(7));
    }
    refreshUpcoming();
    refreshBookings();
}

// -------------------------------------------------------- what is coming up --

QWidget *ScheduleDialog::buildUpcomingTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("Every advertisement and programme due to go on air in this period, "
           "earliest first. Each one is put at the top of the running order "
           "when its minute comes round, so the item on air at the time "
           "finishes first."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *filterBox = new QGroupBox(tr("Show"), page);
    filterBox->setAccessibleName(tr("Schedule filters"));
    auto *filterLayout = new QFormLayout(filterBox);

    const QDateTime now = QDateTime::currentDateTime();

    m_from = new QDateTimeEdit(now, filterBox);
    m_from->setCalendarPopup(true);
    m_from->setDisplayFormat(kDisplayFormat);
    m_from->setAccessibleName(tr("Show airings from"));
    m_from->setAccessibleDescription(tr("The start of the period to list, "
                                        "date and time of day."));
    filterLayout->addRow(tr("&From:"), m_from);

    m_to = new QDateTimeEdit(now.addDays(7), filterBox);
    m_to->setCalendarPopup(true);
    m_to->setDisplayFormat(kDisplayFormat);
    m_to->setAccessibleName(tr("Show airings until"));
    m_to->setAccessibleDescription(tr("The end of the period to list, "
                                      "date and time of day."));
    filterLayout->addRow(tr("&To:"), m_to);

    m_kind = new QComboBox(filterBox);
    m_kind->addItem(tr("Everything"), QString());
    m_kind->addItem(ProgrammeSchedule::kindLabel(Kind::Advert),
                    QStringLiteral("pub"));
    m_kind->addItem(ProgrammeSchedule::kindLabel(Kind::Programme),
                    QStringLiteral("programs"));
    m_kind->setAccessibleName(tr("Kind of item"));
    m_kind->setAccessibleDescription(
        tr("Limit the list to advertisements or to programmes."));
    filterLayout->addRow(tr("&Kind:"), m_kind);

    m_search = new QLineEdit(filterBox);
    m_search->setClearButtonEnabled(true);
    m_search->setPlaceholderText(tr("name or file name"));
    m_search->setAccessibleName(tr("Search text"));
    m_search->setAccessibleDescription(
        tr("Only airings whose name or file name contain this text."));
    filterLayout->addRow(tr("&Search:"), m_search);

    layout->addWidget(filterBox);

    auto *actions = new QHBoxLayout;
    m_refresh = new QPushButton(tr("&Show this period"), page);
    m_refresh->setAccessibleName(tr("Show this period"));
    m_refresh->setDefault(true);
    m_export = new QPushButton(tr("&Export as CSV..."), page);
    m_export->setAccessibleName(tr("Export the listed airings as a CSV file"));
    actions->addWidget(m_refresh);
    actions->addWidget(m_export);
    actions->addStretch();
    layout->addLayout(actions);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({
        tr("Due"), tr("In"), tr("Kind"), tr("Name"), tr("Booking"), tr("File")
    });
    m_table = new QTableView(page);
    m_table->setModel(m_model);
    prepareTable(m_table, tr("Airings due"),
                 tr("One row per airing. Use the arrow keys to move between "
                    "rows and columns; press a column header with the space "
                    "bar to sort by it."));
    layout->addWidget(m_table, 1);

    m_summary = new QLabel(page);
    m_summary->setWordWrap(true);
    m_summary->setAccessibleName(tr("What is due next"));
    layout->addWidget(m_summary);

    connect(m_refresh, &QPushButton::clicked, this, &ScheduleDialog::refreshUpcoming);
    connect(m_export, &QPushButton::clicked, this, &ScheduleDialog::exportUpcomingCsv);
    connect(m_search, &QLineEdit::returnPressed, this, &ScheduleDialog::refreshUpcoming);
    connect(m_kind, &QComboBox::currentIndexChanged, this, &ScheduleDialog::refreshUpcoming);

    return page;
}

void ScheduleDialog::prepareTable(QTableView *view, const QString &name,
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
    view->horizontalHeader()->setAccessibleName(ScheduleDialog::tr("Columns"));
    // A blind operator navigates by row, so the row header is dead weight and
    // one more thing to arrow past.
    view->verticalHeader()->setVisible(false);
}

QString ScheduleDialog::relativeText(const QDateTime &when, const QDateTime &now)
{
    const qint64 seconds = now.secsTo(when);
    if (seconds < 0)
        return tr("now");
    if (seconds < 60)
        return tr("in under a minute");

    const qint64 minutes = seconds / 60;
    if (minutes < 60)
        return tr("in %n minutes", nullptr, int(minutes));

    const qint64 hours = minutes / 60;
    if (hours < 24 && when.date() == now.date())
        return tr("in %n hours", nullptr, int(hours));

    const qint64 days = now.date().daysTo(when.date());
    if (days == 1)
        return tr("tomorrow at %1").arg(when.toString(QStringLiteral("HH:mm")));
    return tr("in %n days", nullptr, int(days));
}

void ScheduleDialog::refreshUpcoming()
{
    const QDateTime from = m_from->dateTime();
    const QDateTime to = m_to->dateTime();
    if (to < from) {
        const QString said = tr("The end of the period is before its start.");
        m_summary->setText(said);
        emit announcementRequested(said);
        return;
    }

    const QString kindFilter = m_kind->currentData().toString();
    const QString search = m_search->text().trimmed();
    const QDateTime now = QDateTime::currentDateTime();

    // Filter first, into a list that is still in time order, and fill from
    // that: the summary below has to name the *earliest* airing, and reading
    // it back off the table would name whichever row the operator's last
    // click on a column header put at the top.
    QList<Airing> airings;
    for (const Airing &airing : ProgrammeSchedule::upcoming(from, to)) {
        const bool isProgramme = airing.rule.kind == Kind::Programme;
        if (!kindFilter.isEmpty()
            && kindFilter != (isProgramme ? QLatin1String("programs")
                                          : QLatin1String("pub")))
            continue;

        const QString name = itemName(airing.rule);
        if (!search.isEmpty()
            && !name.contains(search, Qt::CaseInsensitive)
            && !airing.rule.path.contains(search, Qt::CaseInsensitive))
            continue;

        airings.append(airing);
    }

    // Sorting is re-applied after the fill, so switch it off while the model
    // is being rebuilt — otherwise every appended row triggers a sort.
    m_table->setSortingEnabled(false);
    m_model->removeRows(0, m_model->rowCount());

    for (const Airing &airing : airings) {
        const QString name = itemName(airing.rule);

        QList<QStandardItem *> items;
        auto *due = new QStandardItem(airing.at.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        // The instant itself rides along: the summary line below reads the
        // first row back, and the column's own text is only ever sorted as the
        // ISO string it is written as.
        due->setData(airing.at, Qt::UserRole + 1);
        items << due;
        items << new QStandardItem(relativeText(airing.at, now));
        items << new QStandardItem(ProgrammeSchedule::kindLabel(airing.rule.kind));
        items << new QStandardItem(name);
        items << new QStandardItem(ProgrammeSchedule::describe(airing.rule));
        items << new QStandardItem(airing.rule.path);

        // Every cell speaks the whole row: a screen reader reading the second
        // column alone would say "in 20 minutes" and nothing else.
        const QString spoken =
            tr("%1, %2, due %3, %4")
                .arg(name, ProgrammeSchedule::kindLabel(airing.rule.kind),
                     QLocale().toString(airing.at, QLocale::ShortFormat),
                     relativeText(airing.at, now));
        for (QStandardItem *item : std::as_const(items)) {
            item->setEditable(false);
            item->setData(spoken, Qt::AccessibleTextRole);
            item->setToolTip(spoken);
        }
        m_model->appendRow(items);
    }

    m_table->setSortingEnabled(true);
    // Soonest first, which is the only order this list is any use in. Without
    // this the table takes whatever sort indicator the header happens to hold
    // and can open showing next week at the top.
    m_table->sortByColumn(0, Qt::AscendingOrder);
    m_table->resizeColumnsToContents();

    QString summary;
    if (airings.isEmpty()) {
        summary = tr("Nothing is booked to air in that period.");
    } else {
        const Airing &first = airings.constFirst();
        summary = tr("%n airings listed. First: %1, %2.", nullptr, int(airings.size()))
                      .arg(itemName(first.rule), relativeText(first.at, now));
    }
    m_summary->setText(summary);
    emit announcementRequested(summary);
}

void ScheduleDialog::exportUpcomingCsv()
{
    const int rowCount = m_model->rowCount();
    if (rowCount == 0) {
        QMessageBox::information(this, tr("What Is Scheduled"),
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
        tr("XFB schedule"),
        tr("Period: %1 to %2").arg(m_from->dateTime().toString(kDisplayFormat),
                                   m_to->dateTime().toString(kDisplayFormat)),
        tr("Kind: %1").arg(m_kind->currentText()),
        tr("Exported: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)),
    };

    writeCsv(QStringLiteral("xfb-schedule-%1.csv")
                 .arg(QDate::currentDate().toString(Qt::ISODate)),
             preamble, header, rows);
}

// ----------------------------------------------- the bookings behind it all --

QWidget *ScheduleDialog::buildBookingsTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *intro = new QLabel(
        tr("Every booking in the station's diary, whatever period it falls in: "
           "one-offs, weekly slots and campaigns that run between two dates. "
           "Bookings are made and deleted in \"Add a publicity\" and \"Add a "
           "program\"; this only reads them."), page);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *actions = new QHBoxLayout;
    m_onlyProblems = new QCheckBox(tr("Only the ones that will &not air"), page);
    m_onlyProblems->setAccessibleName(tr("Show only bookings that will not air"));
    m_onlyProblems->setAccessibleDescription(
        tr("A booking whose file has been moved, or whose one-off time went by "
           "while XFB was closed, airs nothing and says nothing."));
    m_refreshBookings = new QPushButton(tr("&Refresh"), page);
    m_refreshBookings->setAccessibleName(tr("Read the bookings again"));
    actions->addWidget(m_onlyProblems);
    actions->addStretch();
    actions->addWidget(m_refreshBookings);
    layout->addLayout(actions);

    m_bookingModel = new QStandardItemModel(this);
    m_bookingModel->setHorizontalHeaderLabels({
        tr("Kind"), tr("Name"), tr("Booking"), tr("Will it air?"), tr("File")
    });
    m_bookingTable = new QTableView(page);
    m_bookingTable->setModel(m_bookingModel);
    prepareTable(m_bookingTable, tr("Bookings"),
                 tr("One row per booking. Arrow keys move between rows and "
                    "columns; the reason a booking will not air is written "
                    "underneath the table as you move onto it."));
    layout->addWidget(m_bookingTable, 1);

    m_bookingDetail = new QLabel(page);
    m_bookingDetail->setWordWrap(true);
    m_bookingDetail->setTextInteractionFlags(Qt::TextSelectableByMouse
                                             | Qt::TextSelectableByKeyboard);
    m_bookingDetail->setAccessibleName(tr("About the selected booking"));
    layout->addWidget(m_bookingDetail);

    m_bookingSummary = new QLabel(page);
    m_bookingSummary->setWordWrap(true);
    m_bookingSummary->setAccessibleName(tr("How many bookings there are"));
    layout->addWidget(m_bookingSummary);

    connect(m_refreshBookings, &QPushButton::clicked,
            this, &ScheduleDialog::refreshBookings);
    connect(m_onlyProblems, &QCheckBox::toggled,
            this, &ScheduleDialog::refreshBookings);
    // currentChanged rather than clicked: the detail line has to follow the
    // arrow keys too, or it is invisible to anyone not using a mouse.
    connect(m_bookingTable->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ScheduleDialog::showBookingDetail);

    return page;
}

void ScheduleDialog::refreshBookings()
{
    const QList<Rule> rules = ProgrammeSchedule::rules();
    const bool onlyProblems = m_onlyProblems->isChecked();

    m_bookingTable->setSortingEnabled(false);
    m_bookingModel->removeRows(0, m_bookingModel->rowCount());

    int problems = 0;
    int shown = 0;
    for (const Rule &rule : rules) {
        if (!rule.willAir())
            ++problems;
        if (onlyProblems && rule.willAir())
            continue;

        const QString name = itemName(rule);
        const QString status = rule.willAir()
                                   ? QString()
                                   : ProgrammeSchedule::problemText(rule.problem);

        QList<QStandardItem *> items;
        items << new QStandardItem(ProgrammeSchedule::kindLabel(rule.kind));
        items << new QStandardItem(name);
        items << new QStandardItem(ProgrammeSchedule::describe(rule));
        auto *statusItem = new QStandardItem(rule.willAir() ? tr("Yes") : tr("No"));
        if (!rule.willAir()) {
            // Bold rather than red: this window follows the operator's theme,
            // and a colour that reads as a warning on one is unreadable on the
            // other. The word says "No" either way.
            QFont bold = statusItem->font();
            bold.setBold(true);
            statusItem->setFont(bold);
        }
        // The whole reason travels with the row, for the detail line below the
        // table and for the CSV a station manager takes away.
        statusItem->setData(status, Qt::UserRole + 2);
        items << statusItem;
        items << new QStandardItem(rule.path);

        const QString spoken = rule.willAir()
            ? tr("%1, %2, %3. It will air.")
                  .arg(name, ProgrammeSchedule::kindLabel(rule.kind),
                       ProgrammeSchedule::describe(rule))
            : tr("%1, %2, %3. It will not air. %4")
                  .arg(name, ProgrammeSchedule::kindLabel(rule.kind),
                       ProgrammeSchedule::describe(rule), status);
        for (QStandardItem *item : std::as_const(items)) {
            item->setEditable(false);
            item->setData(spoken, Qt::AccessibleTextRole);
            item->setToolTip(spoken);
        }
        m_bookingModel->appendRow(items);
        ++shown;
    }

    m_bookingTable->setSortingEnabled(true);
    // By name, so the same booking is in the same place every time this is
    // opened rather than wherever the header's sort indicator last pointed.
    m_bookingTable->sortByColumn(1, Qt::AscendingOrder);
    m_bookingTable->resizeColumnsToContents();
    m_bookingDetail->clear();

    QString summary;
    if (rules.isEmpty())
        summary = tr("Nothing is booked. Bookings are made in the Database "
                     "menu, under \"Add a publicity\" and \"Add a program\".");
    else if (problems == 0)
        summary = tr("%n bookings, and every one of them will air.", nullptr,
                     int(rules.size()));
    else
        summary = tr("%n bookings.", nullptr, int(rules.size())) + QLatin1Char(' ')
                  + tr("%n of them will air nothing — select one to read why.",
                       nullptr, problems);
    if (onlyProblems && shown == 0 && !rules.isEmpty())
        summary = tr("Every booking will air.");

    m_bookingSummary->setText(summary);
    emit announcementRequested(summary);
}

void ScheduleDialog::showBookingDetail()
{
    const QModelIndex current = m_bookingTable->currentIndex();
    if (!current.isValid()) {
        m_bookingDetail->clear();
        return;
    }
    m_bookingDetail->setText(
        m_bookingModel->index(current.row(), 3).data(Qt::UserRole + 2).toString());
}

// --------------------------------------------------------------------- CSV --

bool ScheduleDialog::writeCsv(const QString &suggestedName,
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
    // A BOM: without it Excel opens accented names as mojibake, and this file
    // is going to a client.
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
