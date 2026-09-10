#include "add_full_dir.h"
#include "audioformats.h"
#include "mediaduration.h"
#include "ui_add_full_dir.h"
#include "addgenre.h"
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QHash>
#include <QMap>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QSql>
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>

namespace {

// The category a track inherits from where it sits: the first folder below the
// one being imported. A library is usually filed "Rock/Nirvana/song.mp3", so
// the top level is the category and everything under it is the operator's own
// ordering. A track sitting loose in the chosen folder has no category and
// keeps whatever the dialog says.
QString folderCategory(const QDir &root, const QString &filePath)
{
    const QString relative = root.relativeFilePath(filePath);
    if (relative.startsWith(QLatin1String("..")))
        return QString(); // reached through a link that leaves the tree

    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return QString();

    return parts.first().trimmed();
}

} // namespace


add_full_dir::add_full_dir(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::add_full_dir)
{
    ui->setupUi(this);
    updateGenres();
    ui->chk_folderGenres->setEnabled(ui->chk_recursive->isChecked());
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
                QLocale::AnyTerritory);

    for(const QLocale &locale : allLocales) {
        ui->f_cbox_country->addItem(QLocale::territoryToString(locale.territory()));
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

    // Folders as categories. A library filed "Rock/", "Pop/", "Fado/" under the
    // chosen folder says what each track is far better than one genre picked
    // once for the whole import, so the subfolder wins over the combo box for
    // anything sitting inside one. Only meaningful when we descend at all.
    const bool useFolderGenres = recursive && ui->chk_folderGenres->isChecked();
    const QDir rootDir(dir);

    // The genres already known, keyed case-insensitively, so a "rock" folder is
    // filed under an existing "Rock" instead of creating a second spelling.
    QHash<QString, QString> knownGenres;
    {
        QSqlDatabase db = QSqlDatabase::database("xfb_connection");
        QSqlQuery genreQuery(db);
        genreQuery.prepare("select name from genres1");
        if (genreQuery.exec()) {
            while (genreQuery.next()) {
                const QString name = genreQuery.value(0).toString();
                knownGenres.insert(name.toLower(), name);
            }
        }
    }

    int added = 0;
    int skipped = 0;
    int failed = 0;
    QString firstError;
    QMap<QString, int> byCategory;   // category -> tracks added under it
    QStringList newGenres;           // categories that were not in genres1 yet

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

         // The folder the track lives in, if it lives in one of its own.
         QString category;
         if (useFolderGenres) {
             category = folderCategory(rootDir, filewpath);
             if (!category.isEmpty()) {
                 const QString key = category.toLower();
                 if (knownGenres.contains(key)) {
                     // Keep the spelling the genre list already uses.
                     category = knownGenres.value(key);
                 } else {
                     // A category nobody has typed in yet: file it, and put it
                     // in the genre list so it can be picked and filtered like
                     // any other. Guarded so a race cannot double it up.
                     QSqlDatabase db = QSqlDatabase::database("xfb_connection");
                     QSqlQuery insertGenre(db);
                     insertGenre.prepare("insert into genres1 (name) select :n where not exists "
                                         "(select 1 from genres1 where name = :n collate nocase)");
                     insertGenre.bindValue(":n", category);
                     if (insertGenre.exec()) {
                         newGenres << category;
                     } else {
                         // The track still gets the folder's name; only the
                         // genre list misses out, so say so in the log rather
                         // than claiming a genre was added.
                         qWarning() << "Could not add genre" << category << ":"
                                    << insertGenre.lastError().text();
                     }
                     knownGenres.insert(key, category);
                 }
                 g1 = category;
             }
         }

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
             if (!category.isEmpty())
                 byCategory[category] += 1;
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

   // The genre list grew, so the combo boxes behind this dialog are stale.
   if (!newGenres.isEmpty())
       updateGenres();

   // What went where. An operator who points this at a filed library wants to
   // read back the categories it found, not just a count.
   QString categoryReport;
   if (!byCategory.isEmpty()) {
       QStringList lines;
       for (auto it = byCategory.constBegin(); it != byCategory.constEnd(); ++it)
           lines << tr("%1: %2").arg(it.key()).arg(it.value());
       categoryReport = tr("\n\nFiled by subfolder:\n%1").arg(lines.join(QStringLiteral("\n")));
       if (!newGenres.isEmpty()) {
           newGenres.removeDuplicates();
           categoryReport += tr("\n\nNew genres added to the list: %1")
                                 .arg(newGenres.join(QStringLiteral(", ")));
       }
   }

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
               .arg(found.size()).arg(added).arg(skipped).arg(failed).arg(firstError)
           + categoryReport);
   } else {
       QMessageBox::information(
           this, tr("Add directory"),
           tr("All done! Have a nice day!\n\n"
              "Found %1 audio file(s), added %2, already in the library %3.")
               .arg(found.size()).arg(added).arg(skipped)
           + categoryReport);
   }
   this->hide();

}

void add_full_dir::on_chk_recursive_toggled(bool checked)
{
    // Without descending into subfolders there are no subfolder names to file
    // by, so the option says so rather than quietly doing nothing.
    ui->chk_folderGenres->setEnabled(checked);
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

    // A model takes the query by move and owns it from then on, so each combo
    // needs a query of its own: they used to share one, and re-preparing it for
    // the second model pulled the result out from under the first.
    QSqlQuery qry1(db);
    qry1.prepare("select name from genres1");
    qry1.exec();
    model->setQuery(std::move(qry1));
    ui->f_cbox_genre1->setModel(model);

    QSqlQuery qry2(db);
    qry2.prepare("select name from genres1");
    qry2.exec();
    model2->setQuery(std::move(qry2));
    ui->f_cbox_genre2->setModel(model2);

}
