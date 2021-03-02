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
#include "addjingle.h"
#include "ui_addjingle.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtSql>

addJingle::addJingle(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::addJingle)
{
    ui->setupUi(this);

}

addJingle::~addJingle()
{
    delete ui;
}

void addJingle::on_toolButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                     "",
                                                     tr("Files (*.*)"));
    if(fileName!="")
    ui->txt_file->setText(fileName);

   // QVariant jpath = fileName;

    /*colocar so o nome do ficheiro como nome do jingle*/
    QFileInfo file(fileName);
    QString baseName = file.baseName();
    ui->txt_name->setText(baseName);

   // QVariant jname = baseName;
}

void addJingle::on_pushButton_clicked()
{

    QString jpath = ui->txt_file->text();
    QString jname = ui->txt_name->text();
    QSqlQuery sql;

    sql.prepare("insert into jingles values(:jname,:jpath)");
    sql.bindValue(":jname",jname);
    sql.bindValue(":jpath",jpath);

    if(sql.exec())
    {
        qDebug() << "Sql for insert is: " << sql.lastQuery();
        QMessageBox::information(this,tr("Save"),tr("Jingle Added!"));
        adb.close();
        this->hide();

    } else {
        QMessageBox::critical(this,tr("Error"),sql.lastError().text());
        qDebug() << "Sql for insert is: " << sql.lastQuery();
    }

}


void addJingle::on_pushButton_2_clicked()
{
    this->hide();
}
