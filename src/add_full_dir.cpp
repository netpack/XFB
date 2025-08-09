#include "add_full_dir.h"
#include "ui_add_full_dir.h"
#include "addgenre.h"
#include <QDirIterator>
#include <QFileDialog>
#include <QDebug>
#include <QSql>
#include <QSqlQuery>
#include <QMessageBox>


add_full_dir::add_full_dir(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::add_full_dir)
{
    ui->setupUi(this);
    updateGenres();
    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = writableConfigPath + "/" + configFileName;
    QSettings settingsnew(configFilePath, QSettings::IniFormat);
    bool darkMode = settingsnew.value("DarkMode", false).toBool();
    qDebug() << "[StyleFix] OptionsDialog checking dark mode:" << darkMode;


    // 3. Apply DIRECT stylesheet for background color
    // This overrides the default Fusion background drawing
    if (darkMode) {
        this->setStyleSheet("QDialog { background-color: #353535; color: #bbbbbb; }");
        // Optional: force tab page background again if needed, though attributes should work
        // if (theTabWidget) theTabWidget->setStyleSheet("QWidget { background-color: #353535; color: #bbbbbb; }");
    } else {
        this->setStyleSheet("QDialog { background-color: #ffffff; color: #333333; }");
        // Optional: force tab page background again if needed
        // if (theTabWidget) theTabWidget->setStyleSheet("QWidget { background-color: #ffffff; color: #333333; }");
    }
    // --- END C++ BACKGROUND FIX & DIRECT STYLING ---


    QList<QLocale> allLocales = QLocale::matchingLocales(
                QLocale::AnyLanguage,
                QLocale::AnyScript,
                QLocale::AnyCountry);

    for(const QLocale &locale : allLocales) {
        ui->f_cbox_country->addItem(QLocale::countryToString(locale.country()));
    }




}

add_full_dir::~add_full_dir()
{
    delete ui;
}

void add_full_dir::on_f_bt_browse_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory to import"));
    qDebug()<<"Selected a dir to import: "<<dir;
    ui->txt_path->setText(dir);
}


void add_full_dir::on_f_bt_add_clicked()
{

    QString dir = ui->txt_path->text();

    if(dir.isEmpty())
    {
        QMessageBox::information(this,tr("Path?"),tr("Please select a folder to add."));
    }

    QDirIterator it(dir, QStringList() << "*.mp3" << "*.wav" << "*.ogg" << "*.flac" << "*.aac" << "*.m4a" << "*.wma" << "*.opus", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {

        QString filewpath = it.next();
        qDebug() << "Adding track: " << filewpath;

        QString nomeficheiro = QDir(dir).relativeFilePath(filewpath);

        //nomeficheiro.replace("_"," ");

        QString txtArtist = ui->txt_artistName->text();
        QString artist;
        QStringList divide_nome = nomeficheiro.split( "-" );
        if(!txtArtist.isEmpty()){
            artist = txtArtist;
        } else {
            artist = divide_nome.value(0).replace("_"," ").replace(".mp3","").replace(".mp4","").replace(".ogg","").replace(".wav","").replace(".flac","").trimmed();

        }

        QString song = divide_nome.value(1).replace("_"," ").replace(".mp3","").replace(".mp4","").replace(".ogg","").replace(".wav","").replace(".flac","").trimmed();
        if(song.isEmpty()){
            song="-";
        }

         QString g1 = ui->f_cbox_genre1->currentText();
         QString g2 = ui->f_cbox_genre2->currentText();

         QString country = "Other country / language";

         QString pub_date = ui->f_date->date().toString("yyyy/MM/dd");

         qDebug() << "Got! artist: "<<artist<<" and song: "<<song<<" from file: "<<nomeficheiro;



      //check if it's already in db

     int dbhasmusic=0;
         QSqlDatabase db = QSqlDatabase::database("xfb_connection");
         QSqlQuery query(db);
     query.prepare("SELECT path FROM musics WHERE path=:path");
     query.bindValue(":path",filewpath);
     query.exec();
     while (query.next()){
         QString thispath = query.value(0).toString();
         qDebug() << "Skipping: "<<thispath;
         dbhasmusic=1;
     }
     qDebug()<<"dbhasmusic value is: "<<dbhasmusic;

     if(dbhasmusic==0){
         //add to db

         QProcess cmd;
         QString time;
         QString cmdtmpstr = "exiftool \""+filewpath+"\" | grep Duration";
         cmd.start("sh",QStringList()<<"-c"<<cmdtmpstr);
         cmd.waitForFinished();
         QString cmdOut = cmd.readAll();
         qDebug()<<"Output of exiftool: "<<cmdOut;
         cmd.close();

         QStringList arraycmd = cmdOut.split(" ");
         if(arraycmd.count()>25){
             QStringList splitarray = arraycmd[25].split("\n");
             qDebug()<<"Total track time is: "<<splitarray[0];
             time = splitarray[0];

         } else {
             qDebug()<<"-------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>     !!!!!!!!    An exception happend !? ... outputing details of this track: ";
             for(int i=0;i<arraycmd.count();i++){
                 qDebug()<< "The array position "<<i<<" has: "<<arraycmd[i];
                }

            }
         int played = 0;
         QString last = "";
         QSqlQuery sql(db);
         sql.prepare("insert into musics values(NULL,:artist,:song,:g1,:g2,:country,:pub_date,:file,:time,:played,:last)");
         sql.bindValue(":artist",artist);
         sql.bindValue(":song",song);
         sql.bindValue(":g1",g1);
         sql.bindValue(":g2",g2);
         sql.bindValue(":country",country);
         sql.bindValue(":pub_date",pub_date);
         sql.bindValue(":file",filewpath);
         sql.bindValue(":time",time);
         sql.bindValue(":played",played);
         sql.bindValue(":last",last);

         if(sql.exec())
         {
             qDebug() << "last sql: " << sql.lastQuery();
         } else {
           //  QMessageBox::critical(this,tr("Error"),sql.lastError().text());
             qDebug() << "last sql: " << sql.lastQuery();
         }


     }

}


   QMessageBox::information(this,tr("Add directory"),tr("All done! Have a nice day!"));
   this->hide();

}

void add_full_dir::on_f_bt_manageGenres_clicked()
{
    addgenre addgenre;
    addgenre.setModal(true);
    addgenre.exec();
    updateGenres();
}
void add_full_dir::updateGenres()
{
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQueryModel * model=new QSqlQueryModel();
    QSqlQueryModel * model2=new QSqlQueryModel();

    QSqlQuery* qry=new QSqlQuery(db);

    qry->prepare("select name from genres1");
    qry->exec();
    model->setQuery(*qry);
    ui->f_cbox_genre1->setModel(model);

    qry->prepare("select name from genres1");
    qry->exec();
    model2->setQuery(*qry);
    ui->f_cbox_genre2->setModel(model2);

}
