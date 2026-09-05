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

    // Cancel. It had no slot at all, so the button did nothing: the only way
    // out of this window was Save or the title bar.
    void on_pushButton_2_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_5_clicked();

    // "Date interval and time": every day at a given time, between two
    // dates. Writes a type-3 scheduler row (see run_scheduler()).
    void on_pushButton_7_clicked();

private:
    // Adds one line to the schedule list, tagged with the rowid of the
    // scheduler row it stands for so it can be deleted again exactly.
    void addScheduleLine(const QString &text, const QVariant &schedulerRowId);

    Ui::add_program *ui;
    QString pub_id;
};

#endif // ADD_PROGRAM_H
