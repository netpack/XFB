#ifndef ADD_FULL_DIR_H
#define ADD_FULL_DIR_H

#include <QDialog>

namespace Ui {
class add_full_dir;
}

class add_full_dir : public QDialog
{
    Q_OBJECT

public:
    explicit add_full_dir(QWidget *parent = 0);
    ~add_full_dir();

private slots:
    void on_f_bt_browse_clicked();
    void on_f_bt_add_clicked();
    void on_f_bt_manageGenres_clicked();
    void updateGenres();

private:
    Ui::add_full_dir *ui;
};

#endif // ADD_FULL_DIR_H
