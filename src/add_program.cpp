/*
 * This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General programslic License as programslished by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General programslic License for more details.

    You should have received a copy of the GNU General programslic License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    For more info contact: info@netpack.pt

    */
#include <QFileDialog>
#include <QSqlTableModel>
#include <QDebug>
#include <QMessageBox>
#include <QListWidgetItem>
#include "add_program.h"
#include "audioformats.h"
#include "ui_add_program.h"


add_program::add_program(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::add_program)
{
    ui->setupUi(this);
    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = writableConfigPath + "/" + configFileName;
    QSettings settingsnew(configFilePath, QSettings::IniFormat);
    bool darkMode = settingsnew.value("DarkMode", false).toBool();
    qDebug() << "[StyleFix] OptionsDialog checking dark mode:" << darkMode;


    // 3. Apply DIRECT stylesheet for background color
    // This overrides the default Fusion background drawing
    if (darkMode) {
        this->setStyleSheet("QDialog { background-color: #353535; color: #bbbbbb; }");
        // Optional: force tab page background again if needed, though attributes should work
        // if (theTabWidget) theTabWidget->setStyleSheet("QWidget { background-color: #353535; color: #bbbbbb; }");
    } else {
        this->setStyleSheet("QDialog { background-color: #ffffff; color: #333333; }");
        // Optional: force tab page background again if needed
        // if (theTabWidget) theTabWidget->setStyleSheet("QWidget { background-color: #ffffff; color: #333333; }");
    }
    // --- END C++ BACKGROUND FIX & DIRECT STYLING ---

    qDebug()<<"Adding a new program...";
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");

    QSqlQuery qry(db);
    qry.prepare("insert into programs (name,path) values('Default','Default')");
    qry.exec();

    QDate now = QDate::currentDate();
    qDebug()<<"Current day: "<<now.day() << " ; Current month: "<<now.month()<< " ; Current year: "<<now.year();
    ui->dateTimeEdit->setDate(now);
    // The date interval starts today and runs a week, so "Add new" has
    // something sensible under it rather than the widget's own epoch.
    ui->dateEdit->setDate(now);
    ui->dateEdit_2->setDate(now.addDays(7));


}

add_program::~add_program()
{
    qDebug()<<"Exit add_program.cpp";

    //detele null data
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery qr(db);
    qr.prepare("delete from programs where path = 'Default'");
    qr.exec();
    qDebug()<<"Deleting temp tables";

    delete ui;
}

void add_program::on_pushButton_3_clicked()
{
    //browse for file

    QString file = QFileDialog::getOpenFileName(
        this, tr("Select file"), QString(),
        AudioFormats::fileDialogFilterString());
    ui->txt_selected_file->setText(file);

}

void add_program::on_pushButton_4_clicked()
{
    // add new date and time //

        //get the id of the current programs row
            QString programs_id;
             QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            QSqlQuery qry(db);
            qry.prepare("select id from programs order by id desc limit 0,1");
            qry.exec();
            while(qry.next()){
               programs_id = qry.value(0).toString();

            }
            qDebug()<<"This programs id is: "<<programs_id;

        //get the date/time selected
            QString dateandtime1 = ui->dateTimeEdit->text();
            qDebug()<<"selected date and time: " << dateandtime1;

            QStringList array_dateandtime1 = dateandtime1.split(" ");

            QStringList array_date1 = array_dateandtime1[0].split("/");
            QString dia1 = array_date1[0];
            QString mes1 = array_date1[1];
            QString ano1 = array_date1[2];

            qDebug () << "dia1: " << dia1 << " mes1: " << mes1 << " ano1: "<<ano1;

            QStringList array_time1 = array_dateandtime1[1].split(":");
            QString hora1 = array_time1[0];
            QString min1 = array_time1[1];

            qDebug () << "hora1: " << hora1 << "min1: "<< min1;



        //prepare the query

            QSqlQuery qry_add(db);
            qry_add.prepare("INSERT INTO scheduler VALUES (?, ?, ?, ?, ?, ?, '1', NULL, NULL, NULL, NULL, NULL, NULL, NULL, '1')");
            qry_add.addBindValue(programs_id);
            qry_add.addBindValue(ano1);
            qry_add.addBindValue(mes1);
            qry_add.addBindValue(dia1);
            qry_add.addBindValue(hora1);
            qry_add.addBindValue(min1);


        //add to scheduler
            qry_add.exec();

            qDebug () << qry_add.lastQuery();

            QString thisdateline = dia1+"/"+mes1+"/"+ano1+" at "+hora1+":"+min1;
            addScheduleLine(thisdateline, qry_add.lastInsertId());

}


void add_program::updateScheduleTable()
{







}

void add_program::on_pushButton_clicked()
{
    //save
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
//get the id of the current programs row
    QString programs_id;
QSqlQuery qry(db);
    qry.prepare("select id from programs order by id desc limit 0,1");
    qry.exec();
    while(qry.next()){
       programs_id = qry.value(0).toString();

    }
//get the path
    QString thisPath = ui->txt_selected_file->text();
//get the name
    QString thisName = ui->txt_name->text();

    //QSqlQuery qry;
    qry.prepare("UPDATE programs SET name=?, path=? WHERE id=?");
    qry.addBindValue(thisName);
    qry.addBindValue(thisPath);
    qry.addBindValue(programs_id);
    if(qry.exec()){
        qDebug()<<"add program query was ok: "<<qry.lastQuery();
    } else {
        qDebug()<<"There was an error executing the following sql: "<<qry.lastQuery();
        qDebug()<<"The returned info was: "<<qry.lastError();
    }

   QMessageBox::information(this,"Add program","program saved!");
   this->hide();

}

void add_program::on_pushButton_2_clicked()
{
    // reject() rather than hide(), so Cancel and Escape leave this window the
    // same way. Dates already added with "Add new" are written to the
    // scheduler as they are added and are not taken back by this.
    reject();
}

void add_program::on_pushButton_6_clicked()
{
    /*add new date and time with type 2 (dayOfTheWeek+houre+min)*/

    QString dayOfTheWeek = ui->cbox_dayOfTheWeek->currentText();
    qDebug()<< "Value of dayOfTheWeek is "<<dayOfTheWeek;

    QString hourMinute = ui->hourMinute->text();
    qDebug()<<"Value of hourMinute is "<<hourMinute;

    QString thisprogramsId;
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery Q(db);
    Q.prepare("select id from programs order by id desc limit 0,1");
    Q.exec();
    while(Q.next()){
        thisprogramsId = Q.value(0).toString();
    }

    QStringList array_time = hourMinute.split(":");

    QString hora = array_time[0];
    QString min = array_time[1];

    qDebug()<<"Array splitted hours: "<<hora<<" and minutes: "<<min;

    QSqlQuery Qr_add(db);
    Qr_add.prepare("INSERT INTO scheduler VALUES (?, NULL, NULL, NULL, ?, ?, '2', ?, NULL, NULL, NULL, NULL, NULL, NULL, '1')");
    Qr_add.addBindValue(thisprogramsId);
    Qr_add.addBindValue(hora);
    Qr_add.addBindValue(min);
    Qr_add.addBindValue(dayOfTheWeek);
    Qr_add.exec();
    qDebug()<<"Last Query: "<<Qr_add.lastQuery();
    QString str = dayOfTheWeek + " at " + hourMinute;
    addScheduleLine(str, Qr_add.lastInsertId());

}

void add_program::on_pushButton_5_clicked()
{
    /*delete this selected row*/

    QListWidgetItem *item = ui->listWidget->currentItem();
    if (!item)
        return;

    // Delete by the rowid carried on the line. The old code parsed the line
    // back into a date, which only ever worked for a one-off: a weekly line
    // ("Monday at 21:30") disappeared from the list and stayed in the
    // database, and a date interval cannot be matched that way at all.
    const QVariant rowId = item->data(Qt::UserRole);
    if (rowId.isValid()) {
        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        QSqlQuery qrydel(db);
        qrydel.prepare("DELETE FROM scheduler WHERE rowid = ?");
        qrydel.addBindValue(rowId);
        if (qrydel.exec())
            qDebug() << "Deleted scheduler row" << rowId.toString();
        else
            qWarning() << "Could not delete the schedule:" << qrydel.lastError().text();
    }

    delete ui->listWidget->takeItem(ui->listWidget->row(item));
}

void add_program::addScheduleLine(const QString &text, const QVariant &schedulerRowId)
{
    // The line the operator reads, and — invisibly on it — the rowid of the
    // scheduler row it stands for. Deleting a schedule then means deleting
    // that row, rather than parsing the line back into a date and hoping.
    QListWidgetItem *item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, schedulerRowId);
    ui->listWidget->addItem(item);
}

void add_program::on_pushButton_7_clicked()
{
    /* add a date interval and a time: type 3 — every day at hh:mm, from one
     * date to the other, both included. The scheduler table has carried the
     * six start_/end_ columns for this since it was written; nothing ever
     * filled them, and the button was not connected to anything at all. */

    const QDate from = ui->dateEdit->date();
    const QDate to   = ui->dateEdit_2->date();
    const QTime at   = ui->timeEdit_2->time();

    if (to < from) {
        QMessageBox::warning(this, tr("Date interval"),
                             tr("The end date is before the start date."));
        return;
    }

    QSqlDatabase db = QSqlDatabase::database("xfb_connection");

    QString thisId;
    QSqlQuery qry(db);
    qry.prepare("select id from programs order by id desc limit 0,1");
    qry.exec();
    while (qry.next())
        thisId = qry.value(0).toString();

    QSqlQuery qry_add(db);
    qry_add.prepare("INSERT INTO scheduler VALUES (?, NULL, NULL, NULL, ?, ?,"
                    " '3', NULL, ?, ?, ?, ?, ?, ?, '1')");
    qry_add.addBindValue(thisId);
    qry_add.addBindValue(at.hour());
    qry_add.addBindValue(at.minute());
    qry_add.addBindValue(from.year());
    qry_add.addBindValue(from.month());
    qry_add.addBindValue(from.day());
    qry_add.addBindValue(to.year());
    qry_add.addBindValue(to.month());
    qry_add.addBindValue(to.day());

    if (!qry_add.exec()) {
        qWarning() << "Could not add the date interval:" << qry_add.lastError().text();
        QMessageBox::warning(this, tr("Date interval"),
                             tr("The schedule could not be saved."));
        return;
    }
    qDebug() << "Added a type 3 schedule:" << from << "to" << to << "at" << at;

    addScheduleLine(tr("From %1 To %2 at %3")
                        .arg(from.toString("dd/MM/yyyy"),
                             to.toString("dd/MM/yyyy"),
                             at.toString("hh:mm")),
                    qry_add.lastInsertId());
}
