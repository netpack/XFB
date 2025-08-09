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
#include "addgenre.h"
#include "ui_addgenre.h"
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QList>
#include <QDebug>
#include <QDateTime>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QtSql>

addgenre::addgenre(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::addgenre)
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

QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQueryModel * model=new QSqlQueryModel();

    QSqlQuery* qry=new QSqlQuery(db);

    qry->prepare("select name from genres1 union select name from genres2 order by name");
    qry->exec();
    model->setQuery(*qry);
    ui->listGenres->setModel(model);


}

addgenre::~addgenre()
{
    delete ui;
}

void addgenre::on_btShowGenre_clicked()
{
    QSqlQueryModel * model=new QSqlQueryModel();
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery* qry=new QSqlQuery(db);

    qry->prepare("select name from genres1 union select name from genres2 order by name");
    qry->exec();
    model->setQuery(*qry);
    ui->listGenres->setModel(model);
}

void addgenre::on_btDelGenre_clicked()
{

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm file delete", "Do you really want to delete the selecte Genre?", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        //delete the row
        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        QSqlQuery* qry=new QSqlQuery(db);
        qry->prepare("delete from genres1 where name = :thname");
        qry->bindValue(":thname",valorselecionado);

       if(qry->exec()){
            qDebug() << "Genre deleted from genres1";
       } else {
           qDebug() << "Genre *NOT* deleted from genres1"<< qry->lastError() << qry->lastQuery();
       }

        qDebug() << "Genre Delete Op completed!";


    } else {
      qDebug() << "Genre *not* deleted..";
    }


}
void addgenre::on_listGenres_clicked(const QModelIndex &index)
{
    QVariant valor = ui->listGenres->model()->data(index,0);
    valorselecionado = valor.toString();
    qDebug() << "Pressed row is: " << index.row() << " and QVariant is: " << valor.toString();
}

void addgenre::on_btAddNewGenre_clicked()
{
    QString genreName = ui->txtNewGenre->text();
    //QString genreType = ui->comboBox->currentText();
    qDebug() << genreName ;
QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery* qry=new QSqlQuery(db);

        qry->prepare("insert into genres1 values (NULL,:thgName)");
        qry->bindValue(":thgName",genreName);
        qry->exec();

    QMessageBox::information(this,"Add genre","Genre Added!");
}

void addgenre::on_close_clicked()
{
    this->hide();
}
