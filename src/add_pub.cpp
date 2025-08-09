/*
 * This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    For more info contact: info@netpack.pt

    */
#include <QFileDialog>
#include <QSqlTableModel>
#include <QDebug>
#include <QMessageBox>
#include "add_pub.h"
#include "ui_add_pub.h"

add_pub::add_pub(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::add_pub)
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

    qDebug()<<"Adding a new pub...";

QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery qry(db);
    qry.prepare("insert into pub values(NULL,'Default','Default')");
    qry.exec();
/*
    QString pub_id;
    qry.prepare("select id from pub order by id desc limit 0,1");
    qry.exec();
    while(qry.next()){
       pub_id = qry.value(0).toString();

    }

    QSqlQueryModel * model = new QSqlQueryModel();
        model->setQuery("select * from scheduler where id='"+pub_id+"'");
        ui->tableView->setModel(model);
*/

    QDate now = QDate::currentDate();
    qDebug()<<"Current day: "<<now.day() << " ; Current month: "<<now.month()<< " ; Current year: "<<now.year();
    ui->dateTimeEdit->setDate(now);

    
}

add_pub::~add_pub()
{
    qDebug()<<"Exit add_pub.cpp";

    //detele null data
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery qr(db);
    qr.prepare("delete from pub where path = 'Default'");
    qr.exec();
    qDebug()<<"Deleting temp tables";

    delete ui;
}

void add_pub::on_pushButton_3_clicked()
{
    //browse for file

    QString file = QFileDialog::getOpenFileName(this,tr("Select file"));
    ui->txt_selected_file->setText(file);

}

void add_pub::on_pushButton_4_clicked()
{
    // add new date and time //

        //get the id of the current pub row
            QString pub_id;
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
            QSqlQuery qry(db);
            qry.prepare("select id from pub order by id desc limit 0,1");
            qry.exec();
            while(qry.next()){
               pub_id = qry.value(0).toString();

            }
            qDebug()<<"This pub id is: "<<pub_id;

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
            qry_add.prepare("insert into scheduler values ('"+pub_id+"','"+ano1+"','"+mes1+"','"+dia1+"','"+hora1+"','"+min1+"','1',NULL,NULL,NULL,NULL,NULL,NULL,NULL,'0')");


        //add to scheduler
            qry_add.exec();

            qDebug () << qry_add.lastQuery();

            QString thisdateline = dia1+"/"+mes1+"/"+ano1+" at "+hora1+":"+min1;
            ui->listWidget->addItem(thisdateline);
            /*
            ui->listWidget->clear();

            QSqlQuery qss;
            qss.prepare("select * from scheduler where id='"+pub_id+"'");
            qss.exec();
            while(qss.next()){

                //if type is 1 then show it like: 25/08/2014 at 21:30


                    QString thisyear = qss.value(1).toString();
                    QString thismonth = qss.value(2).toString();
                    QString thisday = qss.value(3).toString();

                    QString thishour = qss.value(4).toString();
                    QString thismin = qss.value(5).toString();

                    QString thisdateline = thisday+"/"+thismonth+"/"+thisyear+" at "+thishour+":"+thismin;
                    ui->listWidget->addItem(thisdateline);
                    qDebug()<<"Adding this line into QlistWidget: "<<thisdateline;




            }*/




}


void add_pub::updateScheduleTable()
{







}

void add_pub::on_pushButton_clicked()
{
    //save

//get the id of the current pub row
    QString pub_id;
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery qry(db);
    qry.prepare("select id from pub order by id desc limit 0,1");
    qry.exec();
    while(qry.next()){
       pub_id = qry.value(0).toString();

    }
//get the path
    QString thisPath = ui->txt_selected_file->text();
//get the name
    QString thisName = ui->txt_name->text();

    //QSqlQuery qry;
    qry.prepare("update pub set name='"+thisName+"', path='"+thisPath+"' where id="+pub_id+"");
    qry.exec();

   QMessageBox::information(this,"Add pub","Publicity saved!");
   this->hide();

}

void add_pub::on_pushButton_6_clicked()
{
    /*add new date and time with type 2 (dayOfTheWeek+houre+min)*/

    QString dayOfTheWeek = ui->cbox_dayOfTheWeek->currentText();
    qDebug()<< "Value of dayOfTheWeek is "<<dayOfTheWeek;

    QString hourMinute = ui->hourMinute->text();
    qDebug()<<"Value of hourMinute is "<<hourMinute;
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QString thisPubId;
QSqlQuery Q(db);
    Q.prepare("select id from pub order by id desc limit 0,1");
    Q.exec();
    while(Q.next()){
        thisPubId = Q.value(0).toString();
    }

    QStringList array_time = hourMinute.split(":");

    QString hora = array_time[0];
    QString min = array_time[1];

    qDebug()<<"Array splitted hours: "<<hora<<" and minutes: "<<min;

    QSqlQuery Qr_add(db);
    Qr_add.prepare("insert into scheduler values ('"+thisPubId+"',NULL,NULL,NULL,'"+hora+"','"+min+"','2','"+dayOfTheWeek+"',NULL,NULL,NULL,NULL,NULL,NULL,'0')");

    Qr_add.exec();
    qDebug()<<"Last Query: "<<Qr_add.lastQuery();
    QString str = dayOfTheWeek + " at " + hourMinute;
    ui->listWidget->addItem(str);
    /*
    while(Qr_add.next()){
        //if type is 2 then show it like: Mondays at 21:30

        QString thisday = Qr_add.value(7).toString();
        QString thishour = Qr_add.value(4).toString();
        QString thismin = Qr_add.value(5).toString();
        QString str = thisday+" at "+thishour+":"+thismin;
        ui->listWidget->addItem(str);





        //if type is 3 then show it like: From 25/08/2014 To 26/08/2014 at 21:30
    }*/

}

void add_pub::on_pushButton_5_clicked()
{
    /*delete this selected row*/


    QString estaData = ui->listWidget->currentItem()->text();
    qDebug()<<"estaData: "<<estaData;

    QStringList array_estaData = estaData.split("/");
    qDebug()<<"array_estaData: "<<array_estaData.count();

    if(array_estaData.count()==1){
        qDebug()<<"dealing with a tipe 2";
    }
    if(array_estaData.count()==3){
        qDebug()<<"dealing with a tipe 1";
        QString dia = array_estaData[0];
        QString mes = array_estaData[1];
        QString ano = array_estaData[2];

        QStringList hstr = estaData.split(" ");
        QStringList hstr1 = hstr[2].split(":");
        QString hora = hstr1[0];
        QString min = hstr1[1];

        QString qqq = "delete from scheduler where dia='"+dia+"' and mes='"+mes+"' and dia='"+dia+"' and ano='"+ano+"' and hora='"+hora+"' and min='"+min+"'";
        qDebug()<<qqq;
        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        QSqlQuery qrydel(db);
        qrydel.prepare(qqq);
        qrydel.exec();


    }
    qDebug()<<"qlistwidget index: "<<ui->listWidget->currentRow();

    int g = ui->listWidget->currentRow();
    delete ui->listWidget->item(g);

}
