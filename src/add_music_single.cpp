/*
 * This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    For more info contact: info@netpack.pt

    */
#include "add_music_single.h"
#include "audioformats.h"
#include "mediaduration.h"
#include "ui_add_music_single.h"
#include "addgenre.h"
#include <QFileDialog>
//#include "connect.h"
#include <QMessageBox>
#include <QtSql>
add_music_single::add_music_single(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::add_music_single)
{
    ui->setupUi(this);
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

    updateGenres();


 QList<QLocale> allLocales = QLocale::matchingLocales(
             QLocale::AnyLanguage,
             QLocale::AnyScript,
             QLocale::AnyTerritory);

 for(const QLocale &locale : allLocales) {
     ui->cbox_country->addItem(QLocale::territoryToString(locale.territory()));
 }




}


void add_music_single::updateGenres()
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
    ui->cbox_g1->setModel(model);

    QSqlQuery qry2(db);
    qry2.prepare("select name from genres1");
    qry2.exec();
    model2->setQuery(std::move(qry2));
    ui->cbox_g2->setModel(model2);

}


add_music_single::~add_music_single()
{
    delete ui;
}

void add_music_single::on_toolButton_clicked()
{

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open File"), QString(),
        AudioFormats::fileDialogFilterString());
    if(fileName!="")
    ui->txt_file->setText(fileName);

/*
    QProcess cmd;
    cmd.start("sh", QStringList() << "-c" << "mediainfo \""+fileName+"\"");
    cmd.waitForFinished();
    QString cout = cmd.readAll();
    qDebug()<<"running mediainfo: "<<cout;

    QStringList pieces = cout.split( "\n" );

    QString format = pieces.value( 2 );
    QString size = pieces.value( 3 );
    QString duration = pieces.value( 4 );
    QString bitrate = pieces.value( 6 );
    QString album = pieces.value( 7 );


    QString art = pieces.value( 8 );
    QString sng = pieces.value( 9 );
    QString gnr = pieces.value( 14 );

    QStringList artist_arr = art.split(":");
    QString artist_name;
    if(!artist_arr.isEmpty())
        artist_name = artist_arr[1].trimmed();

    QString song_name;
    QStringList song_arr = sng.split(":");
    if(!song_arr.isEmpty())
        song_name = song_arr[1].trimmed();


    QStringList format_arr = format.split(":");
    if(!format_arr.isEmpty())
        QString format_name = format_arr[1].trimmed();

    QStringList album_arr = album.split(":");
    if(!album_arr.isEmpty())
        QString album_name = album_arr[1].trimmed();

    QStringList bitrate_arr = bitrate.split(":");
    if(!bitrate_arr.isEmpty())
        QString bitrate_name = bitrate_arr[1].trimmed();

    QStringList duration_arr = duration.split(":");
    if(!duration_arr.isEmpty())
        QString duration_name = duration_arr[1].trimmed();

    QStringList size_arr = size.split(":");
    if(!size_arr.isEmpty())
        QString size_name = size_arr[1].trimmed();

    QString genre_name;
    QStringList genre_arr = gnr.split(":");
    if(!genre_arr.isEmpty())
        genre_name = genre_arr[1].trimmed();


    QString artistN = ui->txt_artist->text();
    if(artistN.isEmpty()){
     if(!artist_name.isEmpty()){
         qDebug()<<"artist_name from meta info is: "<<artist_name;
         ui->txt_artist->setText(artist_name);
     }

    }


    QString song = ui->txt_song->text();
    if(song.isEmpty()){
            if(!song_name.isEmpty())
            ui->txt_song->setText(song_name);
        }
    QString g1 = ui->cbox_g1->currentText();
    if(g1.isEmpty()){
        if(!genre_name.isEmpty())
            ui->cbox_g1->setCurrentText(genre_name);

    }
    QString g2= ui->cbox_g2->currentText();
    if(g2.isEmpty()){
        if(!genre_name.isEmpty())
            ui->cbox_g2->setCurrentText(genre_name);
    }

*/
}

void add_music_single::on_pushButton_2_clicked()
{
    this->hide();

}

void add_music_single::on_pushButton_clicked()
{
    QString file, artist, song, g1, g2, country, data_mes, data_dia;
    file = ui->txt_file->text();
    artist = ui->txt_artist->text();
    song = ui->txt_song->text();
    g1 = ui->cbox_g1->currentText();
    g2= ui->cbox_g2->currentText();
    country = ui->cbox_country->currentText();
    QString data_ano = ui->f_date->date().toString("yyyy/MM/dd");
    //data_mes = ui->f_date->MonthSection;
    //data_dia = ui->f_date->DaySection;

  //  QString pub_date = data_ano+"-"+data_mes+"-"+data_dia;
    qDebug()<<"data_ano value: "<<data_ano;


    qDebug()<<"g1 is "<< g1 << " g2 is "<< g2<<" contry is "<<country;

    QString time = MediaDuration::forFile(file);
    qDebug()<<"Total track time is: "<<time;
    QSqlDatabase db = QSqlDatabase::database("xfb_connection");
    QSqlQuery sql(db);


        sql.prepare("INSERT INTO musics (artist,song,genre1,genre2,country,published_date,"
                    "path,time,played_times,last_played) VALUES(?, ?, ?, ?, ?, ?, ?, ?, 0, '-')");
        sql.addBindValue(artist);
        sql.addBindValue(song);
        sql.addBindValue(g1);
        sql.addBindValue(g2);
        sql.addBindValue(country);
        sql.addBindValue(data_ano);
        sql.addBindValue(file);
        sql.addBindValue(time);


        if(sql.exec())
        {
            qDebug() << "last sql: " << sql.lastQuery();
            QMessageBox::information(this,tr("Save"),tr("Music Added!"));
            connClose();
            this->hide();
        } else {
            QMessageBox::critical(this,tr("Error"),sql.lastError().text());
            qDebug() << "last sql: " << sql.lastQuery();
        }




/*
        QSqlQuery sql;
        sql.prepare("insert into musics (artist,song,genre1,genre2,country,published_date,path,time) "
                    "values(:artist,:song,:g1,:g2,:country,:pub_date,:file,:time)");
        sql.bindValue(":artist",artist);
        sql.bindValue(":song",song);
        sql.bindValue(":g1",g1);
        sql.bindValue(":g2",g2);
        sql.bindValue(":country",country);
        sql.bindValue(":pub_date",data_ano);
        sql.bindValue(":file",file);
        sql.bindValue(":time",time);

        if(sql.exec())
        {
            qDebug() << "last sql: " << sql.lastQuery();


        } else {
            QMessageBox::critical(this,tr("Error"),sql.lastError().text());
            qDebug() << "last sql: " << sql.lastQuery();
        }
*/


}

void add_music_single::on_bt_manageGenres_clicked()
{
    addgenre addgenre;
    addgenre.setModal(true);
    addgenre.exec();
    updateGenres();
}
