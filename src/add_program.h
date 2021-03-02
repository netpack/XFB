#ifndef ADD_PROGRAM_H
#define ADD_PROGRAM_H

#include <QDialog>
#include <QtSql>

namespace Ui {
class add_program;
}

class add_program : public QDialog
{
    Q_OBJECT

public:
    explicit add_program(QWidget *parent = 0);
    ~add_program();

private slots:
    void on_pushButton_3_clicked();
    void updateScheduleTable();
    void on_pushButton_4_clicked();
    void on_pushButton_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_5_clicked();

private:
    Ui::add_program *ui;
    QString pub_id;
};

#endif // ADD_PROGRAM_H
