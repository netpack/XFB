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
#ifndef ADDGENRE_H
#define ADDGENRE_H

#include <QDialog>
#include <QMainWindow>
#include <QtSql>
#include <QtDebug>
#include <QList>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>
namespace Ui {
class addgenre;
}

class addgenre : public QDialog
{
    Q_OBJECT

public:
    explicit addgenre(QWidget *parent = 0);
    ~addgenre();

private slots:
    void on_btShowGenre_clicked();

    void on_btDelGenre_clicked();

    void on_listGenres_clicked(const QModelIndex &index);

    void on_btAddNewGenre_clicked();

    void on_close_clicked();

private:
    Ui::addgenre *ui;
    QString valorselecionado;
    QMessageBox reply;

};

#endif // ADDGENRE_H
