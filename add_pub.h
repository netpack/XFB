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
#ifndef ADD_PUB_H
#define ADD_PUB_H

#include <QDialog>
#include <QtSql>

namespace Ui {
class add_pub;
}

class add_pub : public QDialog
{
    Q_OBJECT

public:
    explicit add_pub(QWidget *parent = 0);
    ~add_pub();

private slots:
    void on_pushButton_3_clicked();
    void updateScheduleTable();
    void on_pushButton_4_clicked();
    void on_pushButton_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_5_clicked();

private:
    Ui::add_pub *ui;
    QString pub_id;

};

#endif // ADD_PUB_H
