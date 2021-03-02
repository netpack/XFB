#ifndef ABOUTUS_H
#define ABOUTUS_H

#include <QDialog>

namespace Ui {
class aboutUs;
}

class aboutUs : public QDialog
{
    Q_OBJECT

public:
    explicit aboutUs(QWidget *parent = 0);
    ~aboutUs();

private:
    Ui::aboutUs *ui;
};

#endif // ABOUTUS_H
