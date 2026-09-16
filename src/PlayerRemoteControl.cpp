// The player's half of the network remote control: what each command of the
// API actually does to the station, and what the station looks like from
// outside. The HTTP, the keys and the routing live in RemoteControlServer; this
// file never sees a request, and the server never sees a widget.
//
// Kept out of player.cpp on purpose. Every command here is a thin, checked
// wrapper around a slot the desk already uses — the Play button, the Record
// button, the playlist context menu — so a remote press behaves exactly like
// one at the desk, including the as-run log and the dead-air watchdog. What is
// added is the checking: a button the operator can see is never pressed in a
// state where it would do something surprising, whereas a request arrives
// blind, so every command first says why it cannot run rather than doing half
// of it.

#include "player.h"
#include "ui_player.h"

#include "PlaylistWaveView.h"
#include "audio/ProgramRecorder.h"
#include "dialogs/RemoteControlDialog.h"
#include "permission_utils.h"
#include "services/RemoteControlServer.h"
#include "services/StreamService.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStatusBar>

using Reply = RemoteControlServer::Reply;

namespace {

/** The library tables a remote client may search and add from, as the API names them. */
struct LibrarySource {
    const char *name;   ///< "music", as in the ref "music:12"
    const char *table;
    const char *titleColumn;
    const char *artistColumn;  ///< nullptr where the table has none
    const char *durationColumn;
};

// Adverts are left out, as they are at the desk: the ad list has no "add to
// playlist" of its own, because ads are placed by the scheduler.
constexpr LibrarySource kSources[] = {
    {"music",    "musics",   "song", "artist", "time"},
    {"jingles",  "jingles",  "name", nullptr,  nullptr},
    {"programs", "programs", "name", nullptr,  nullptr},
};

const LibrarySource *sourceNamed(const QString &name)
{
    for (const LibrarySource &source : kSources) {
        if (name == QLatin1String(source.name))
            return &source;
    }
    return nullptr;
}

QSqlDatabase libraryDatabase()
{
    return QSqlDatabase::database(QStringLiteral("xfb_connection"));
}

/**
 * An integer argument, whether it came as a JSON number or — from a query
 * string, or a client that quotes everything — as text. A fraction is not an
 * index, so 1.5 is refused rather than rounded.
 */
bool intArg(const QJsonObject &args, const QString &name, qint64 *out)
{
    const QJsonValue value = args.value(name);
    if (value.isDouble()) {
        const double d = value.toDouble();
        if (d != double(qint64(d)))
            return false;
        *out = qint64(d);
        return true;
    }
    if (value.isString()) {
        bool ok = false;
        *out = value.toString().trimmed().toLongLong(&ok);
        return ok;
    }
    return false;
}

/** Artist and title for a file, from the library when it is there. */
void describeTrack(const QString &path, QString *artist, QString *title, QString *duration)
{
    artist->clear();
    title->clear();
    if (duration)
        duration->clear();

    QSqlDatabase db = libraryDatabase();
    if (db.isOpen() && !path.isEmpty()) {
        QSqlQuery lookup(db);
        lookup.prepare(QStringLiteral("SELECT artist, song, time FROM musics WHERE path = :path"));
        lookup.bindValue(QStringLiteral(":path"), path);
        if (lookup.exec() && lookup.next()) {
            *artist = lookup.value(0).toString().trimmed();
            *title = lookup.value(1).toString().trimmed();
            if (duration)
                *duration = lookup.value(2).toString();
        }
    }
    if (title->isEmpty()) {
        // "Artist - Title.mp3" is how most libraries on disk are named.
        const QString base = QFileInfo(path).completeBaseName();
        const int dash = base.indexOf(QStringLiteral(" - "));
        if (artist->isEmpty() && dash > 0) {
            *artist = base.left(dash).trimmed();
            *title = base.mid(dash + 3).trimmed();
        } else {
            *title = base;
        }
    }
}

QJsonObject changed(bool didChange)
{
    QJsonObject body;
    body.insert(QStringLiteral("changed"), didChange);
    return body;
}

} // namespace

// Created on demand, like the phone sync server, and silent until started:
// an operator who never opens the window has no port open.
RemoteControlServer *player::remoteControlServer()
{
    if (m_remoteControl)
        return m_remoteControl;

    m_remoteControl = new RemoteControlServer(this);
    m_remoteControl->setCommandHandler([this](const QString &command, const QJsonObject &args) {
        return handleRemoteCommand(command, args);
    });

    // Somebody at the desk should never be left wondering why the station
    // just stopped. Every command that changes something says so, and names
    // the key that did it, which is also how an operator finds a key to
    // revoke.
    connect(m_remoteControl, &RemoteControlServer::commandExecuted, this,
            [this](const QString &keyName, const QString &command, int status) {
        qInfo() << "Remote control:" << keyName << command << status;
        if (status != 200 || !ui || !ui->statusBar)
            return;
        ui->statusBar->showMessage(tr("Remote control (%1): %2").arg(keyName, command), 6000);
    });
    connect(m_remoteControl, &RemoteControlServer::errorOccurred, this,
            [this](const QString &message) {
        qWarning() << "Remote control:" << message;
        if (ui && ui->statusBar)
            ui->statusBar->showMessage(tr("Remote control: %1").arg(message), 10000);
    });

    return m_remoteControl;
}

void player::openRemoteControlDialog()
{
    if (!m_remoteControlDialog) {
        m_remoteControlDialog = new RemoteControlDialog(remoteControlServer(), this);
        m_remoteControlDialog->setAttribute(Qt::WA_DeleteOnClose, false);
        connect(m_remoteControlDialog, &RemoteControlDialog::announcementRequested,
                this, &player::announceAccessible);
    }
    m_remoteControlDialog->show();
    m_remoteControlDialog->raise();
    m_remoteControlDialog->activateWindow();
}

// What a client polling /api/v1/status, or subscribed to /api/v1/events, sees.
// Runs every second per subscriber set, so it reads state and never scans.
QJsonObject player::remoteStatus()
{
    QJsonObject status;
    status.insert(QStringLiteral("api"), RemoteControlServer::apiVersion());
    status.insert(QStringLiteral("version"), QCoreApplication::applicationVersion());

    QString state = QStringLiteral("stopped");
    if (Xplayer) {
        switch (Xplayer->playbackState()) {
        case QMediaPlayer::PlayingState: state = QStringLiteral("playing"); break;
        case QMediaPlayer::PausedState:  state = QStringLiteral("paused");  break;
        default: break;
        }
    }
    QString mode = QStringLiteral("stopped");
    if (PlayMode == QLatin1String("Playing_Segue"))
        mode = QStringLiteral("segue");
    else if (PlayMode == QLatin1String("Playing_StopAtNextOne"))
        mode = QStringLiteral("stop-after");

    QJsonObject transport;
    transport.insert(QStringLiteral("state"), state);
    transport.insert(QStringLiteral("mode"), mode);
    status.insert(QStringLiteral("transport"), transport);

    if (state != QLatin1String("stopped") && !lastPlayedSong.isEmpty()) {
        // One lookup per track, not per poll.
        if (m_remoteStatusPath != lastPlayedSong) {
            m_remoteStatusPath = lastPlayedSong;
            describeTrack(lastPlayedSong, &m_remoteStatusArtist, &m_remoteStatusTitle, nullptr);
        }
        const qint64 position = Xplayer->position();
        const qint64 duration = Xplayer->duration();
        QJsonObject now;
        now.insert(QStringLiteral("path"), lastPlayedSong);
        now.insert(QStringLiteral("artist"), m_remoteStatusArtist);
        now.insert(QStringLiteral("title"), m_remoteStatusTitle);
        now.insert(QStringLiteral("positionMs"), position);
        now.insert(QStringLiteral("durationMs"), duration);
        now.insert(QStringLiteral("remainingMs"), duration > 0 ? qMax(qint64(0), duration - position) : qint64(-1));
        // Only the key travels here; the cover itself is a request of its own,
        // because this object is polled every second and a JPEG is not. The
        // refresh has to happen here rather than being left to whoever else
        // might ask: without it the key stays empty and no client ever learns
        // there is a cover to fetch.
        refreshPublicArtwork();
        now.insert(QStringLiteral("artworkKey"), m_publicArtKey);
        status.insert(QStringLiteral("nowPlaying"), now);
    } else {
        status.insert(QStringLiteral("nowPlaying"), QJsonValue::Null);
    }

    QJsonObject playlist;
    playlist.insert(QStringLiteral("count"), ui->playlist->count());
    status.insert(QStringLiteral("playlist"), playlist);

    status.insert(QStringLiteral("autoMode"), autoMode == 1);
    status.insert(QStringLiteral("volume"), ui->sliderVolume->value());
    status.insert(QStringLiteral("volumeLocked"), !ui->sliderVolume->isEnabled());

    QJsonObject recording;
    recording.insert(QStringLiteral("state"),
                     recMode == 0 ? QStringLiteral("idle")
                     : ui->bt_rec->isHidden() ? QStringLiteral("starting")
                                              : QStringLiteral("recording"));
    recording.insert(QStringLiteral("paused"), recMode == 1 && recPause);
    status.insert(QStringLiteral("recording"), recording);

    QJsonObject stream;
    // Asked of an existing service only: a status poll must not create one.
    stream.insert(QStringLiteral("active"), m_streamService && m_streamService->isActive());
    status.insert(QStringLiteral("stream"), stream);

    QJsonObject desk;
    desk.insert(QStringLiteral("locked"), m_deskLocked);
    status.insert(QStringLiteral("desk"), desk);

    return status;
}

Reply player::handleRemoteCommand(const QString &command, const QJsonObject &args)
{
    if (!ui || !ui->playlist || !Xplayer)
        return Reply::error(503, tr("XFB is still starting."));

    // ---------------------------------------------------------------- read

    if (command == QLatin1String("status"))
        return Reply::ok(remoteStatus());

    if (command == QLatin1String("artwork")) {
        // The very JPEG the public page is given: already decoded, scaled and
        // re-encoded by publicNowPlaying(), and held until the track changes.
        // No route here ever opens a file, so a cover can only be bytes the
        // player deliberately handed over.
        // Not publicNowPlaying(): that reports nothing while the track is
        // paused, and a paused track still has a cover worth showing.
        refreshPublicArtwork();
        if (m_publicArtJpeg.isEmpty())
            return Reply::error(404, tr("There is no cover for what is playing."));
        return Reply::file(m_publicArtJpeg, QByteArrayLiteral("image/jpeg"));
    }

    if (command == QLatin1String("playlist")) {
        QJsonArray items;
        for (int row = 0; row < ui->playlist->count(); ++row) {
            const QListWidgetItem *item = ui->playlist->item(row);
            if (!item)
                continue;
            const QString path = item->text();
            QString artist, title, duration;
            describeTrack(path, &artist, &title, &duration);

            QJsonObject entry;
            entry.insert(QStringLiteral("index"), row);
            entry.insert(QStringLiteral("path"), path);
            entry.insert(QStringLiteral("artist"), artist);
            entry.insert(QStringLiteral("title"), title);
            entry.insert(QStringLiteral("duration"), duration);
            entry.insert(QStringLiteral("overlapMs"),
                         item->data(PlaylistWaveView::OverlapRole).toLongLong());
            entry.insert(QStringLiteral("voiceTrack"),
                         item->data(PlaylistWaveView::VoiceTrackRole).toBool());
            items.append(entry);
        }
        QJsonObject body;
        body.insert(QStringLiteral("items"), items);
        return Reply::ok(body);
    }

    if (command == QLatin1String("library.search")) {
        const QString sourceName = args.value(QStringLiteral("source")).toString(QStringLiteral("music"));
        const LibrarySource *source = sourceNamed(sourceName);
        if (!source)
            return Reply::error(422, tr("Unknown source \"%1\". Use music, jingles or programs.").arg(sourceName));

        qint64 limit = 25;
        if (args.contains(QStringLiteral("limit")) && !intArg(args, QStringLiteral("limit"), &limit))
            return Reply::error(422, tr("limit must be a whole number."));
        limit = qBound(qint64(1), limit, qint64(100));

        QSqlDatabase db = libraryDatabase();
        if (!db.isOpen())
            return Reply::error(503, tr("The library database is not open."));

        // LIKE's own wildcards in what was typed are matched literally.
        QString needle = args.value(QStringLiteral("q")).toString().trimmed();
        needle.replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
              .replace(QLatin1Char('%'), QStringLiteral("\\%"))
              .replace(QLatin1Char('_'), QStringLiteral("\\_"));
        needle = QLatin1Char('%') + needle + QLatin1Char('%');

        // Table and column names come from kSources, never from the request.
        const QString artist = source->artistColumn ? QString::fromLatin1(source->artistColumn) : QStringLiteral("''");
        const QString durationCol = source->durationColumn ? QString::fromLatin1(source->durationColumn) : QStringLiteral("''");
        QString where = QStringLiteral("%1 LIKE :q ESCAPE '\\'").arg(QLatin1String(source->titleColumn));
        if (source->artistColumn)
            where += QStringLiteral(" OR %1 LIKE :q2 ESCAPE '\\'").arg(QLatin1String(source->artistColumn));

        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT rowid, %1, %2, %3, path FROM %4 WHERE %5 ORDER BY %1, %2 LIMIT :limit")
                          .arg(artist, QLatin1String(source->titleColumn), durationCol,
                               QLatin1String(source->table), where));
        query.bindValue(QStringLiteral(":q"), needle);
        if (source->artistColumn)
            query.bindValue(QStringLiteral(":q2"), needle);
        query.bindValue(QStringLiteral(":limit"), limit);
        if (!query.exec())
            return Reply::error(500, tr("The library search failed."));

        QJsonArray results;
        while (query.next()) {
            QJsonObject entry;
            const qint64 id = query.value(0).toLongLong();
            entry.insert(QStringLiteral("ref"), QStringLiteral("%1:%2").arg(QLatin1String(source->name)).arg(id));
            entry.insert(QStringLiteral("source"), QLatin1String(source->name));
            entry.insert(QStringLiteral("id"), id);
            entry.insert(QStringLiteral("artist"), query.value(1).toString());
            entry.insert(QStringLiteral("title"), query.value(2).toString());
            entry.insert(QStringLiteral("duration"), query.value(3).toString());
            entry.insert(QStringLiteral("path"), query.value(4).toString());
            results.append(entry);
        }
        QJsonObject body;
        body.insert(QStringLiteral("results"), results);
        return Reply::ok(body);
    }

    // ----------------------------------------------------------- transport

    const bool onAir = Xplayer->playbackState() != QMediaPlayer::StoppedState;
    const bool haveSomethingToPlay = ui->playlist->count() > 0 || autoMode == 1;

    if (command == QLatin1String("transport.play")) {
        if (playPause) {
            on_bt_pause_play_clicked();  // resume
            if (PlayMode == QLatin1String("Playing_StopAtNextOne"))
                on_btPlay_clicked();     // and carry on past this track
            return Reply::ok(changed(true));
        }
        // "Play and stop" leaves the transport in its stop-after mode once the
        // track has ended, and so does a running order that ran dry. The Play
        // button then only flips the mode back to segue and plays nothing; a
        // remote play means play, so the transport is reset to stopped first.
        if (PlayMode != QLatin1String("stopped")
            && Xplayer->playbackState() == QMediaPlayer::StoppedState) {
            on_btStop_clicked();
        }
        if (PlayMode == QLatin1String("stopped")) {
            if (!haveSomethingToPlay)
                return Reply::error(409, tr("The playlist is empty and Auto Mode is off."));
            on_btPlay_clicked();
            if (PlayMode == QLatin1String("stopped"))
                return Reply::error(409, tr("Nothing could be played: the library has no track to offer."));
            return Reply::ok(changed(true));
        }
        if (PlayMode == QLatin1String("Playing_StopAtNextOne")) {
            on_btPlay_clicked();  // back to segue
            return Reply::ok(changed(true));
        }
        return Reply::ok(changed(false));  // already playing and segueing
    }

    if (command == QLatin1String("transport.stopAfter")) {
        if (PlayMode == QLatin1String("stopped"))
            return Reply::error(409, tr("Nothing is playing."));
        if (PlayMode == QLatin1String("Playing_Segue")) {
            on_btPlay_clicked();  // the button's second state: play and stop
            return Reply::ok(changed(true));
        }
        return Reply::ok(changed(false));
    }

    if (command == QLatin1String("transport.pause")) {
        if (playPause)
            return Reply::ok(changed(false));
        if (Xplayer->playbackState() != QMediaPlayer::PlayingState)
            return Reply::error(409, tr("Nothing is playing."));
        on_bt_pause_play_clicked();
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("transport.resume")) {
        if (!playPause)
            return Reply::ok(changed(false));
        on_bt_pause_play_clicked();
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("transport.stop")) {
        if (!onAir && PlayMode == QLatin1String("stopped"))
            return Reply::ok(changed(false));
        // The desk's Stop leaves a paused Pause button lit, and the next Play
        // would then read as a resume. Nobody remote can see that button.
        if (playPause) {
            playPause = false;
            ui->bt_pause_play->setStyleSheet(QString());
        }
        on_btStop_clicked();
        refreshTransportAccessibleState();
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("transport.next")) {
        if (PlayMode == QLatin1String("stopped"))
            return Reply::error(409, tr("Nothing is playing. Use play to start."));
        if (!haveSomethingToPlay)
            return Reply::error(409, tr("Nothing is queued after this track and Auto Mode is off."));
        on_btPlayNext_clicked();
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("transport.seek")) {
        qint64 position = 0;
        if (!intArg(args, QStringLiteral("positionMs"), &position))
            return Reply::error(422, tr("positionMs must be a whole number of milliseconds."));
        if (disableSeekBar)
            return Reply::error(409, tr("Seeking is switched off in Options."));
        if (!onAir)
            return Reply::error(409, tr("Nothing is playing."));
        const qint64 duration = Xplayer->duration();
        position = qBound(qint64(0), position, qMax(qint64(0), duration));
        Xplayer->setPosition(position);
        QJsonObject body;
        body.insert(QStringLiteral("positionMs"), position);
        return Reply::ok(body);
    }

    // ------------------------------------------------------ volume, Auto Mode

    if (command == QLatin1String("volume.set")) {
        qint64 volume = 0;
        if (!intArg(args, QStringLiteral("volume"), &volume) || volume < 0 || volume > 100)
            return Reply::error(422, tr("volume must be a whole number from 0 to 100."));
        if (!ui->sliderVolume->isEnabled())
            return Reply::error(409, tr("The volume control is locked in Options."));
        ui->sliderVolume->setValue(int(volume));
        on_sliderVolume_sliderMoved(int(volume));
        QJsonObject body;
        body.insert(QStringLiteral("volume"), int(volume));
        return Reply::ok(body);
    }

    if (command == QLatin1String("automode.set")) {
        const QJsonValue enabled = args.value(QStringLiteral("enabled"));
        if (!enabled.isBool())
            return Reply::error(422, tr("enabled must be true or false."));
        if ((autoMode == 1) == enabled.toBool())
            return Reply::ok(changed(false));
        on_bt_autoMode_clicked();
        return Reply::ok(changed(true));
    }

    // ------------------------------------------------------------ playlist

    if (command == QLatin1String("playlist.add")) {
        // A track is named by its library row, never by a path: a remote
        // client can queue what the station already holds, and nothing else.
        QString sourceName = args.value(QStringLiteral("source")).toString();
        qint64 id = -1;
        const QString ref = args.value(QStringLiteral("ref")).toString();
        if (!ref.isEmpty()) {
            const int colon = ref.indexOf(QLatin1Char(':'));
            bool ok = false;
            if (colon > 0)
                id = ref.mid(colon + 1).toLongLong(&ok);
            if (colon <= 0 || !ok)
                return Reply::error(422, tr("ref must look like \"music:123\"."));
            sourceName = ref.left(colon);
        } else if (!intArg(args, QStringLiteral("id"), &id)) {
            return Reply::error(422, tr("Give the track as ref (\"music:123\"), or as source and id."));
        }
        if (sourceName.isEmpty())
            sourceName = QStringLiteral("music");
        const LibrarySource *source = sourceNamed(sourceName);
        if (!source)
            return Reply::error(422, tr("Unknown source \"%1\". Use music, jingles or programs.").arg(sourceName));

        QSqlDatabase db = libraryDatabase();
        if (!db.isOpen())
            return Reply::error(503, tr("The library database is not open."));
        QSqlQuery lookup(db);
        lookup.prepare(QStringLiteral("SELECT path FROM %1 WHERE rowid = :id").arg(QLatin1String(source->table)));
        lookup.bindValue(QStringLiteral(":id"), id);
        if (!lookup.exec() || !lookup.next())
            return Reply::error(404, tr("There is no %1 entry %2 in the library.").arg(sourceName).arg(id));
        const QString path = lookup.value(0).toString();
        if (path.isEmpty() || !QFileInfo::exists(path))
            return Reply::error(409, tr("The audio file for that entry is missing."));

        const int count = ui->playlist->count();
        int index = count;
        const QJsonValue position = args.value(QStringLiteral("position"));
        if (position.isString() && position.toString() == QLatin1String("start")) {
            index = 0;
        } else if (position.isString() && position.toString() == QLatin1String("end")) {
            index = count;
        } else if (!position.isUndefined() && !position.isNull()) {
            qint64 wanted = 0;
            if (!intArg(args, QStringLiteral("position"), &wanted) || wanted < 0 || wanted > count)
                return Reply::error(422, tr("position must be \"start\", \"end\" or an index from 0 to %1.").arg(count));
            index = int(wanted);
        }

        ui->playlist->insertItem(index, path);
        calculate_playlist_total_time();

        QJsonObject body;
        body.insert(QStringLiteral("index"), index);
        body.insert(QStringLiteral("path"), path);
        return Reply::ok(body);
    }

    if (command == QLatin1String("playlist.remove")) {
        qint64 index = -1;
        if (!intArg(args, QStringLiteral("index"), &index))
            return Reply::error(422, tr("index must be a whole number."));
        if (index < 0 || index >= ui->playlist->count())
            return Reply::error(404, tr("There is no playlist entry %1.").arg(index));
        QListWidgetItem *item = ui->playlist->takeItem(int(index));
        const QString path = item ? item->text() : QString();
        delete item;
        calculate_playlist_total_time();
        QJsonObject body;
        body.insert(QStringLiteral("path"), path);
        return Reply::ok(body);
    }

    if (command == QLatin1String("playlist.move")) {
        qint64 from = -1, to = -1;
        if (!intArg(args, QStringLiteral("from"), &from) || !intArg(args, QStringLiteral("to"), &to))
            return Reply::error(422, tr("from and to must be whole numbers."));
        const int count = ui->playlist->count();
        if (from < 0 || from >= count || to < 0 || to >= count)
            return Reply::error(404, tr("from and to must be indexes from 0 to %1.").arg(count - 1));
        if (from == to)
            return Reply::ok(changed(false));
        // takeItem/insertItem move the same object, so the crossfade and the
        // volume line stored on the item go with it — as at the desk.
        QListWidgetItem *moved = ui->playlist->takeItem(int(from));
        ui->playlist->insertItem(int(to), moved);
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("playlist.clear")) {
        if (ui->playlist->count() == 0)
            return Reply::ok(changed(false));
        // No "are you sure" here: the desk asks because a mouse slips, and a
        // request that says clear was sent on purpose.
        ui->playlist->clear();
        calculate_playlist_total_time();
        return Reply::ok(changed(true));
    }

    // ------------------------------------------------- recording and stream

    if (command == QLatin1String("recording.start")) {
        if (recMode == 1)
            return Reply::ok(changed(false));
        // Both of these would otherwise put a dialog on the desk and leave the
        // request hanging until somebody there answered it.
        if (m_recSource != QLatin1String("onair")) {
            const MicrophonePermissionStatus mic = checkMicrophonePermission();
            if (mic != MicrophonePermissionStatus::Granted
                && mic != MicrophonePermissionStatus::NotApplicable)
                return Reply::error(409, tr("This computer has not allowed XFB to use the microphone. "
                                            "Start one recording at the desk first."));
        }
        if (m_recSource != QLatin1String("input") && !ProgramRecorder::available())
            return Reply::error(409, tr("Recording what goes on air needs ffmpeg, and it was not found."));
        on_bt_rec_clicked();
        if (recMode != 1)
            return Reply::error(409, tr("The recording did not start."));
        QJsonObject body = changed(true);
        body.insert(QStringLiteral("startsInMs"), 5000);
        return Reply::ok(body);
    }

    if (command == QLatin1String("recording.stop")) {
        if (recMode == 0)
            return Reply::ok(changed(false));
        // Stopping inside the countdown would leave its timer to start a
        // recording nobody is tracking; the desk key waits for the same reason.
        if (ui->bt_rec->isHidden())
            return Reply::error(409, tr("The recording is about to start. Stop it once the countdown ends."));
        on_bt_rec_clicked();
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("stream.start")) {
        StreamService *service = streamService();
        if (service->isActive())
            return Reply::ok(changed(false));
        service->start();
        if (!service->isActive())
            return Reply::error(409, tr("The stream did not start: no mount is set up."));
        return Reply::ok(changed(true));
    }

    if (command == QLatin1String("stream.stop")) {
        if (!m_streamService || !m_streamService->isActive())
            return Reply::ok(changed(false));
        m_streamService->stop();
        return Reply::ok(changed(true));
    }

    return Reply::error(404, tr("No such command."));
}
