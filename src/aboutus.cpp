#include "aboutus.h"
#include "ui_aboutus.h"

aboutUs::aboutUs(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutUs)
{
    ui->setupUi(this);
}

aboutUs::~aboutUs()
{
    delete ui;
}
