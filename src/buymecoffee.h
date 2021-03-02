#ifndef BUYMECOFFEE_H
#define BUYMECOFFEE_H

#include <QWidget>

namespace Ui {
class BuyMeCoffee;
}

class BuyMeCoffee : public QWidget
{
    Q_OBJECT

public:
    explicit BuyMeCoffee(QWidget *parent = 0);
    ~BuyMeCoffee();

private slots:
    void on_pushButton_clicked();

private:
    Ui::BuyMeCoffee *ui;
};

#endif // BUYMECOFFEE_H
