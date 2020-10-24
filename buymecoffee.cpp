#include "buymecoffee.h"
#include "ui_buymecoffee.h"

BuyMeCoffee::BuyMeCoffee(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BuyMeCoffee)
{
    ui->setupUi(this);
}

BuyMeCoffee::~BuyMeCoffee()
{
    delete ui;
}

void BuyMeCoffee::on_pushButton_clicked()
{
    //close widget
}
