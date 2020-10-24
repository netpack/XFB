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
#ifndef ADD_MUSIC_SINGLE_H
#define ADD_MUSIC_SINGLE_H

#include <QDialog>
#include <QtSql>
#include <QtDebug>
#include <QFileInfo>
namespace Ui {
class add_music_single;
}

class add_music_single : public QDialog
{
    Q_OBJECT
    
public:
     QSqlDatabase adb;
     void connClose(){
         adb.close();
         adb.removeDatabase(QSqlDatabase::defaultConnection);
     }

    explicit add_music_single(QWidget *parent = 0);
    ~add_music_single();
    
private slots:
    void on_toolButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
    void on_bt_manageGenres_clicked();
    void updateGenres();

private:
    Ui::add_music_single *ui;
};

#endif // ADD_MUSIC_SINGLE_H
