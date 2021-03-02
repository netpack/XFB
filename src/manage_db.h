#ifndef MANAGE_DB_H
#define MANAGE_DB_H

#include <QDialog>

namespace Ui {
class manage_db;
}

class manage_db : public QDialog
{
    Q_OBJECT

public:
    explicit manage_db(QWidget *parent = 0);
    ~manage_db();

private:
    Ui::manage_db *ui;
};

#endif // MANAGE_DB_H
