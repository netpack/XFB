#ifndef EXTERNALDOWNLOADER_H
#define EXTERNALDOWNLOADER_H

#include <QWidget>
#include <QtSql>
#include <QtDebug>
#include <QFileInfo>



namespace Ui {
class externaldownloader;
}

class externaldownloader : public QWidget
{
    Q_OBJECT

public:
    explicit externaldownloader(QWidget *parent = 0);
    ~externaldownloader();
     QSqlDatabase adb;

signals:
    void musicAdded(); // Signal emitted when music is successfully added to database

private slots:
    void on_bt_youtube_getIt_clicked();
    void on_pushButton_clicked();
    void getFile();
    void showLoadingFrame();
    void on_bt_close_clicked();
    void on_bt_clear_clicked();

private:
    Ui::externaldownloader *ui;
};

#endif // EXTERNALDOWNLOADER_H
