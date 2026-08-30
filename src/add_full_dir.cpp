#include "add_full_dir.h"
#include "audioformats.h"
#include "mediaduration.h"
#include "ui_add_full_dir.h"
#include "addgenre.h"
#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QSql>
#include <QSqlError>
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

    QString dir = ui->txt_path->text().trimmed();

    if(dir.isEmpty())
    {
        // Used to fall through and scan "" — a scan of nothing that still
        // ended with "All done! Have a nice day!".
        QMessageBox::information(this,tr("Path?"),tr("Please select a folder to add."));
        return;
    }

    if(!QFileInfo(dir).isDir())
    {
        QMessageBox::information(this,tr("Path?"),
                                 tr("That folder does not exist: %1").arg(dir));
        return;
    }

    // One list, one walk: every extension XFB accepts, matched case-insensitively,
    // descending into subfolders and through symlinked folders (loop-guarded).
    const bool recursive = ui->chk_recursive->isChecked();
    const QStringList found = AudioFormats::findAudioFiles(dir, recursive);
    qDebug() << "Scanning" << dir << (recursive ? "and its subfolders" : "only")
             << "found" << found.size() << "audio files";

    int added = 0;
    int skipped = 0;
    int failed = 0;
    QString firstError;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    for (const QString &filewpath : found) {

        qDebug() << "Adding track: " << filewpath;

        // The name only. This used to be the path relative to the folder being
        // imported, so anything in a subfolder was filed under an artist called
        // "Rock/Best of" — and the split below never saw a clean file name.
        QString nomeficheiro = QFileInfo(filewpath).fileName();

        QString fileArtist;
        QString fileSong;
        AudioFormats::splitArtistAndSong(nomeficheiro, &fileArtist, &fileSong);

        QString txtArtist = ui->txt_artistName->text();
        QString artist = txtArtist.isEmpty() ? fileArtist : txtArtist;

        QString song = fileSong;
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

     if(dbhasmusic!=0){
         ++skipped;
     }

     if(dbhasmusic==0){
         //add to db

         QString time = MediaDuration::forFile(filewpath);
         qDebug()<<"Total track time is: "<<time;
         int played = 0;
         QString last = "";
         QSqlQuery sql(db);
         // Columns named, never positional: the table has grown (bpm, loudness,
         // intro times) and a bare VALUES(...) has to fill every one of them or
         // SQLite refuses the whole insert.
         sql.prepare("insert into musics (artist,song,genre1,genre2,country,published_date,"
                     "path,time,played_times,last_played) "
                     "values(:artist,:song,:g1,:g2,:country,:pub_date,:file,:time,:played,:last)");
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
             ++added;
             qDebug() << "last sql: " << sql.lastQuery();
         } else {
             // Never swallow this again. A silent failure here is what made a
             // whole folder look like it had simply been ignored: the columns
             // of musics had grown and the insert had been refused every time,
             // with nothing on screen and nothing in the summary to say so.
             ++failed;
             if (firstError.isEmpty())
                 firstError = sql.lastError().text();
             qWarning() << "Could not add" << filewpath << ":" << sql.lastError().text();
         }


     }

}

   QApplication::restoreOverrideCursor();

   // Saying what happened beats "All done!": an operator who points this at a
   // folder of Opus files now sees whether they went in.
   if (found.isEmpty()) {
       QMessageBox::information(
           this, tr("Add directory"),
           tr("No audio files were found in %1.\n\n"
              "XFB imports: %2")
               .arg(dir, AudioFormats::suffixes().join(QStringLiteral(", "))));
   } else if (failed > 0) {
       // A refused insert is a fault in XFB, not something the operator did
       // wrong, so it says so plainly and hands over the reason SQLite gave.
       QMessageBox::warning(
           this, tr("Add directory"),
           tr("Found %1 audio file(s): added %2, already in the library %3, "
              "and %4 could not be added.\n\nThe database refused them: %5")
               .arg(found.size()).arg(added).arg(skipped).arg(failed).arg(firstError));
   } else {
       QMessageBox::information(
           this, tr("Add directory"),
           tr("All done! Have a nice day!\n\n"
              "Found %1 audio file(s), added %2, already in the library %3.")
               .arg(found.size()).arg(added).arg(skipped));
   }
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
