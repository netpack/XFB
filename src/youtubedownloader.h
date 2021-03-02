#ifndef YOUTUBEDOWNLOADER_H
#define YOUTUBEDOWNLOADER_H

#include <QWidget>
#include <QtSql>
#include <QtDebug>
#include <QFileInfo>



namespace Ui {
class youtubedownloader;
}

class youtubedownloader : public QWidget
{
    Q_OBJECT

public:
    explicit youtubedownloader(QWidget *parent = 0);
    ~youtubedownloader();
     QSqlDatabase adb;

private slots:
    void on_bt_youtube_getIt_clicked();

    void on_pushButton_clicked();
    void getFile();
    void showLoadingFrame();
    void on_bt_close_clicked();

    void on_bt_clear_clicked();

private:
    Ui::youtubedownloader *ui;
};

#endif // YOUTUBEDOWNLOADER_H
