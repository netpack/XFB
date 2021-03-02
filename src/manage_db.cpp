#include "manage_db.h"
#include "ui_manage_db.h"

manage_db::manage_db(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::manage_db)
{
    ui->setupUi(this);
}

manage_db::~manage_db()
{
    delete ui;
}
