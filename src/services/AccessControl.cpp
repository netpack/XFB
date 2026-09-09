#include "AccessControl.h"

#include "../secretstore.h"

#include <QAction>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QMessageBox>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>

namespace {

/** Where the accounts file lives — beside xfb.conf, never inside the library
 *  database: the database is copied to the backup station and to the phone,
 *  and the passwords of this station's staff have no business travelling with
 *  the music. */
QString accountsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb-users.conf");
}

/** Chosen so signing in costs a fraction of a second on the oldest machine a
 *  station is likely to have in the rack, and a very great deal on somebody
 *  else's. Stored per digest, so raising it later leaves old accounts working. */
constexpr int kPbkdf2Iterations = 120000;

/**
 * The one operator the reserved permissions answer to.
 *
 * The external downloader and the torrent tab are not features a station is
 * simply allowed to use: they are switched on for one desk, deliberately, by
 * the person whose desk it is. Every other installation — including one with
 * no accounts at all, which is the ordinary case — does not show them, cannot
 * grant them, and does not list them among the permissions an administrator
 * ticks through. Matched against the user name case-insensitively, the same
 * way signing in matches it.
 */
constexpr char kReservedOperator[] = "f";

QString setToString(const QSet<QString> &keys)
{
    QStringList sorted(keys.cbegin(), keys.cend());
    sorted.sort();
    return sorted.join(QLatin1Char(','));
}

QSet<QString> setFromString(const QString &value)
{
    QSet<QString> keys;
    const QStringList parts = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts)
        keys.insert(part.trimmed());
    return keys;
}

} // namespace

// ---------------------------------------------------------------------------
// The catalogue
// ---------------------------------------------------------------------------

const QVector<AccessControl::Permission> &AccessControl::catalogue()
{
    // Built once, on first use, so the strings are translated with whatever
    // language was installed by the time anything asks — which is always
    // after main() has loaded the translator.
    static const QVector<Permission> permissions = [] {
        const QString library     = tr("Library");
        const QString playback    = tr("Playlist and playback");
        const QString programming = tr("Programming");
        const QString station     = tr("Station");
        const QString downloads   = tr("Downloads");
        const QString admin       = tr("Administration");

        return QVector<Permission>{
            // -- Library ---------------------------------------------------
            {QStringLiteral("library.add.single"), library,
             tr("Add a single song"),
             tr("Import one music file into the library.")},
            {QStringLiteral("library.add.folder"), library,
             tr("Add all songs in a folder"),
             tr("Import a whole folder, subfolders and all.")},
            {QStringLiteral("library.add.jingle"), library,
             tr("Add a jingle"), tr("Import a jingle.")},
            {QStringLiteral("library.add.publicity"), library,
             tr("Add a publicity"), tr("Import an advert.")},
            {QStringLiteral("library.add.program"), library,
             tr("Add a program"), tr("Import a recorded programme.")},
            {QStringLiteral("library.genres"), library,
             tr("Manage genres"),
             tr("Add, rename and remove the genres tracks are filed under.")},
            {QStringLiteral("library.edit"), library,
             tr("Edit track details"),
             tr("Change the artist, title, genre or country of tracks already "
                "in the library, one at a time or in a batch.")},
            {QStringLiteral("library.delete"), library,
             tr("Delete tracks from the database"),
             tr("Remove a track, jingle, advert or programme from the library.")},
            {QStringLiteral("library.check"), library,
             tr("Check and update the records"),
             tr("Walk the library looking for records whose file has moved or "
                "whose duration is missing, and repair them.")},
            {QStringLiteral("library.purge"), library,
             tr("Delete every invalid record without confirmation"),
             tr("The sweep that removes anything it cannot find on disk, in one "
                "pass and with nothing to confirm. An unmounted share makes it "
                "empty the library.")},
            {QStringLiteral("library.duplicates"), library,
             tr("Remove duplicate songs"),
             tr("Keep one record per artist and title, deleting the rest.")},
            {QStringLiteral("library.autotrim"), library,
             tr("AutoTrim the silence from every track"),
             tr("Rewrite every music file in the library with its leading and "
                "trailing silence removed.")},
            {QStringLiteral("library.convert"), library,
             tr("Convert the whole library to another format"),
             tr("Re-encode every music file to mp3, ogg or opus. Hours of work "
                "and the original encoding is not coming back.")},
            {QStringLiteral("library.retune"), library,
             tr("Retune the whole library to 432 Hz"),
             tr("Re-encode every music file at the other tuning.")},
            {QStringLiteral("library.analyse"), library,
             tr("Analyse tempo, intros and loudness"),
             tr("The background sweeps that fill in BPM, intro times and "
                "replay gain. They read the files and never rewrite them.")},
            {QStringLiteral("library.coverart"), library,
             tr("Find the missing cover art"),
             tr("Look up covers for tracks that have none and write them into "
                "the files.")},
            {QStringLiteral("library.watchedfolders"), library,
             tr("Set the watched folders"),
             tr("Choose which folders file themselves into the library.")},
            {QStringLiteral("library.audacity"), library,
             tr("Open a track in Audacity"),
             tr("Hand a library file to the external editor.")},

            // -- Playlist and playback -------------------------------------
            {QStringLiteral("playback.transport"), playback,
             tr("Play, pause, stop and skip"),
             tr("Drive what is on air. Withdrawing this leaves somebody who can "
                "watch the station run but not touch it.")},
            {QStringLiteral("playlist.open"), playback,
             tr("Open files straight into the player"),
             tr("Play a file from the disk without it being in the library.")},
            {QStringLiteral("playlist.edit"), playback,
             tr("Change what is in the playlist"),
             tr("Add, remove and reorder the running order.")},
            {QStringLiteral("playlist.load"), playback,
             tr("Load a playlist"), tr("Replace the running order with a saved one.")},
            {QStringLiteral("playlist.save"), playback,
             tr("Save the playlist"), tr("Keep the running order for later.")},
            {QStringLiteral("playlist.clear"), playback,
             tr("Clear the playlist"), tr("Empty the running order in one go.")},
            {QStringLiteral("playback.cue"), playback,
             tr("Cue tracks on the headphones"),
             tr("Listen to a track on the cue output before it goes to air.")},
            {QStringLiteral("playback.voicetrack"), playback,
             tr("Record voice tracks"),
             tr("Record the link over a join and write the ducking.")},
            {QStringLiteral("playback.automix"), playback,
             tr("Turn Auto Mode and Auto-mix on and off"),
             tr("Hand the running order to XFB, or take it back.")},
            {QStringLiteral("playback.fx"), playback,
             tr("Change the audio effects"),
             tr("The equaliser, the compressor and the deck effects.")},

            // -- Programming -----------------------------------------------
            {QStringLiteral("programming.rotation"), programming,
             tr("Edit the rotation rules"),
             tr("Separation, categories, dayparts and seasonal windows.")},
            {QStringLiteral("programming.hourclock"), programming,
             tr("Edit the hour clock"),
             tr("The shape of the hour Auto Mode fills in.")},
            {QStringLiteral("programming.timesignal"), programming,
             tr("Edit the time signals"),
             tr("The pips and the hour ident: what plays on the hour, in "
                "which hours, and whether it interrupts.")},
            {QStringLiteral("programming.airlog"), programming,
             tr("Read the as-run log"),
             tr("What actually went to air, and the advertiser report.")},
            {QStringLiteral("programming.schedule"), programming,
             tr("Read the schedule"),
             tr("What is booked to go on air and when, and which bookings XFB "
                "cannot honour. It reads the diary; it does not change it.")},
            {QStringLiteral("programming.quota"), programming,
             tr("Mark and report the music quota"),
             tr("Say which tracks count as national music, and measure what "
                "went to air against the station's obligation.")},
            {QStringLiteral("programming.requests"), programming,
             tr("Run the listener request line"),
             tr("The public page and the requests that arrive on it.")},
            {QStringLiteral("programming.deadair"), programming,
             tr("Change the dead-air watchdog"),
             tr("What XFB does when the station falls silent.")},
            {QStringLiteral("programming.record"), programming,
             tr("Record a new programme"),
             tr("Record from the microphone into the programmes library.")},
            {QStringLiteral("programming.makeprogram"), programming,
             tr("Make a programme from this playlist"),
             tr("Render the running order into one programme file.")},

            // -- Station ---------------------------------------------------
            {QStringLiteral("station.options"), station,
             tr("Open the Options window"),
             tr("Every station-wide setting: audio devices, paths, the server "
                "credentials, the language.")},
            {QStringLiteral("station.fullscreen"), station,
             tr("Switch full screen on and off"), tr("Cosmetic, and harmless.")},
            {QStringLiteral("station.stream"), station,
             tr("Stream to Icecast"),
             tr("Put this machine on the internet, or take it off.")},
            {QStringLiteral("station.sync.mobile"), station,
             tr("Pair a phone"),
             tr("Hand out a token that lets a phone read the library.")},
            {QStringLiteral("station.sync.station"), station,
             tr("Set up broadcast redundancy"),
             tr("Pair the machine standing by as this one's backup.")},
            {QStringLiteral("station.sync.production"), station,
             tr("Set up a production computer"),
             tr("Pair a machine that is allowed to write to this library.")},
            {QStringLiteral("station.server.ftp"), station,
             tr("Force an FTP check"), tr("Run the server exchange now.")},
            {QStringLiteral("station.server.monitor"), station,
             tr("Force monitoring"), tr("Run the monitoring script now.")},
            {QStringLiteral("station.server.ip"), station,
             tr("Update the dynamic server's IP"),
             tr("Tell the dynamic DNS provider where the station is.")},
            {QStringLiteral("station.update"), station,
             tr("Check for updates"),
             tr("Look for a newer XFB and install it.")},
            {QStringLiteral("station.dependencies"), station,
             tr("Install the external tools"),
             tr("Fetch FFmpeg, yt-dlp and the rest onto this machine.")},

            // -- Downloads -------------------------------------------------
            // Both of these are off for every role until an administrator
            // ticks them, and what they gate is *hidden* rather than greyed
            // out — a station that does not download from the internet should
            // not have to look at the entries that do.
            {QStringLiteral("downloads.external"), downloads,
             tr("Allow adding sources from external sources"),
             tr("Show \"Add a song from an external source\" in the menu, and the "
                "downloader it opens: YouTube, Bandcamp, a Spotify or Apple Music "
                "listing, and the rest. Without it the entry is not there.")},
            {QStringLiteral("downloads.torrents"), downloads,
             tr("Support .torrent files"),
             tr("Show the XFB Torrents switch in the Options window, and — once it "
                "is switched on — the Torrents tab with the searches and downloads "
                "made from it. Without it neither is there.")},

            // -- Administration --------------------------------------------
            {QStringLiteral("admin.users"), admin,
             tr("Manage users and roles"),
             tr("Create accounts, change what each role may do, and turn the "
                "protection off. Give this only to somebody who would be "
                "allowed to reinstall XFB.")},
        };
    }();

    return permissions;
}

QStringList AccessControl::categories()
{
    QStringList names;
    for (const Permission &permission : catalogue()) {
        if (!names.contains(permission.category))
            names.append(permission.category);
    }
    return names;
}

bool AccessControl::isReserved(const QString &key)
{
    // The Downloads category, whole. Both entries in it are the same kind of
    // thing — a way of taking music off the internet — and a station that has
    // one has the other.
    return key.startsWith(QLatin1String("downloads."));
}

bool AccessControl::isReservedOperator(const User &user)
{
    return !user.username.isEmpty()
           && user.username.compare(QLatin1String(kReservedOperator), Qt::CaseInsensitive) == 0;
}

bool AccessControl::mayAdministerReserved() const
{
    // Protected and signed in as that operator: an installation with no
    // accounts has nobody to be him, so it never shows these.
    return isProtected() && m_signedIn && isReservedOperator(m_current);
}

QString AccessControl::labelFor(const QString &key)
{
    for (const Permission &permission : catalogue()) {
        if (permission.key == key)
            return permission.label;
    }
    // A key written by a newer XFB than this one. Show it rather than drop it:
    // the administrator can still see it is there, and save() keeps it.
    return key;
}

// ---------------------------------------------------------------------------
// The roles a fresh installation starts with
// ---------------------------------------------------------------------------

QList<AccessControl::Role> AccessControl::defaultRoles()
{
    QList<Role> roles;

    Role administrator;
    administrator.id = QStringLiteral("admin");
    administrator.name = tr("Administrator");
    administrator.description = tr("Everything, including who else may sign in. "
                                   "There is always at least one.");
    administrator.everything = true;
    administrator.builtin = true;
    roles.append(administrator);

    // The producer prepares what goes out: the library is theirs, the
    // programming is theirs, the machine is not. The three library sweeps that
    // rewrite or empty everything are left out — those are an owner's decision,
    // not a working one.
    Role producer;
    producer.id = QStringLiteral("producer");
    producer.name = tr("Producer");
    producer.description = tr("Prepares the programming: the library, the "
                              "rotation, the clock. Cannot change the station's "
                              "own settings or who signs in.");
    producer.builtin = true;
    for (const Permission &permission : catalogue()) {
        if (permission.key.startsWith(QLatin1String("admin."))
            || permission.key.startsWith(QLatin1String("station.sync."))
            || permission.key.startsWith(QLatin1String("station.server."))
            || permission.key == QLatin1String("station.options")
            || permission.key == QLatin1String("station.stream")
            || permission.key == QLatin1String("station.update")
            || permission.key == QLatin1String("station.dependencies")
            || permission.key == QLatin1String("library.purge")
            || permission.key == QLatin1String("library.convert")
            || permission.key == QLatin1String("library.retune")
            || permission.key == QLatin1String("library.autotrim")
            // The two switched-on-by-the-station features: an administrator
            // hands these out deliberately, per role, or nobody has them.
            || permission.key.startsWith(QLatin1String("downloads.")))
            continue;
        producer.permissions.insert(permission.key);
    }
    roles.append(producer);

    // The presenter drives the show and nothing else. Everything they need to
    // be on air, nothing that outlives their shift.
    Role presenter;
    presenter.id = QStringLiteral("presenter");
    presenter.name = tr("Presenter");
    presenter.description = tr("Drives the show: the playlist, the transport, "
                               "the cue, voice tracks and the requests. Leaves "
                               "the library as they found it.");
    presenter.builtin = true;
    presenter.permissions = {
        QStringLiteral("playback.transport"), QStringLiteral("playlist.open"),
        QStringLiteral("playlist.edit"),      QStringLiteral("playlist.load"),
        QStringLiteral("playlist.save"),      QStringLiteral("playlist.clear"),
        QStringLiteral("playback.cue"),       QStringLiteral("playback.voicetrack"),
        QStringLiteral("playback.automix"),   QStringLiteral("playback.fx"),
        QStringLiteral("programming.airlog"), QStringLiteral("programming.schedule"),
        QStringLiteral("programming.requests"),
        QStringLiteral("station.fullscreen"),
    };
    roles.append(presenter);

    // Somebody covering a shift who has not been shown the system yet, or a
    // machine in reception: the station plays, and that is all.
    Role guest;
    guest.id = QStringLiteral("guest");
    guest.name = tr("Guest");
    guest.description = tr("Watches the station run and can start and stop it. "
                           "Changes nothing.");
    guest.builtin = true;
    guest.permissions = {
        QStringLiteral("playback.transport"),
        QStringLiteral("station.fullscreen"),
    };
    roles.append(guest);

    return roles;
}

// ---------------------------------------------------------------------------
// Passwords
// ---------------------------------------------------------------------------

QString AccessControl::hashPassword(const QString &password)
{
    QByteArray salt(16, Qt::Uninitialized);
    QRandomGenerator::system()->generate(salt.begin(), salt.end());

    const QByteArray digest = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password.toUtf8(), salt, kPbkdf2Iterations, 32);

    return QStringLiteral("pbkdf2-sha256$%1$%2$%3")
        .arg(kPbkdf2Iterations)
        .arg(QString::fromLatin1(salt.toBase64()),
             QString::fromLatin1(digest.toBase64()));
}

bool AccessControl::verifyPassword(const QString &password, const QString &stored)
{
    const QStringList parts = stored.split(QLatin1Char('$'));
    if (parts.size() != 4 || parts.at(0) != QLatin1String("pbkdf2-sha256"))
        return false;

    bool ok = false;
    const int iterations = parts.at(1).toInt(&ok);
    if (!ok || iterations <= 0)
        return false;

    const QByteArray salt = QByteArray::fromBase64(parts.at(2).toLatin1());
    const QByteArray expected = QByteArray::fromBase64(parts.at(3).toLatin1());
    if (salt.isEmpty() || expected.isEmpty())
        return false;

    const QByteArray digest = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password.toUtf8(), salt, iterations, expected.size());

    // Compared in constant time. The threat here is small, but a timing
    // comparison is not harder to write than a wrong one.
    if (digest.size() != expected.size())
        return false;
    quint8 difference = 0;
    for (int i = 0; i < digest.size(); ++i)
        difference |= static_cast<quint8>(digest.at(i) ^ expected.at(i));
    return difference == 0;
}

QString AccessControl::storagePath()
{
    return accountsPath();
}

// ---------------------------------------------------------------------------
// Construction and storage
// ---------------------------------------------------------------------------

AccessControl &AccessControl::instance()
{
    static AccessControl control;
    return control;
}

AccessControl::AccessControl(QObject *parent)
    : QObject(parent)
{
    load();
}

void AccessControl::load()
{
    m_roles.clear();
    m_users.clear();

    QSettings settings(accountsPath(), QSettings::IniFormat);

    m_signInRequired = settings.value(QStringLiteral("AccessControl/SignInRequired"), true).toBool();
    m_autoSignInUser = settings.value(QStringLiteral("AccessControl/AutoSignInUser")).toString();
    m_autoLockMinutes = settings.value(QStringLiteral("AccessControl/AutoLockMinutes"), 0).toInt();
    m_lastUser = settings.value(QStringLiteral("AccessControl/LastUser")).toString();

    const int roleCount = settings.beginReadArray(QStringLiteral("Roles"));
    for (int i = 0; i < roleCount; ++i) {
        settings.setArrayIndex(i);
        Role role;
        role.id = settings.value(QStringLiteral("id")).toString();
        role.name = settings.value(QStringLiteral("name")).toString();
        role.description = settings.value(QStringLiteral("description")).toString();
        role.everything = settings.value(QStringLiteral("everything"), false).toBool();
        role.builtin = settings.value(QStringLiteral("builtin"), false).toBool();
        role.permissions = setFromString(settings.value(QStringLiteral("permissions")).toString());
        if (!role.id.isEmpty())
            m_roles.append(role);
    }
    settings.endArray();

    const int userCount = settings.beginReadArray(QStringLiteral("Users"));
    for (int i = 0; i < userCount; ++i) {
        settings.setArrayIndex(i);
        User user;
        user.username = settings.value(QStringLiteral("username")).toString();
        user.displayName = settings.value(QStringLiteral("displayName")).toString();
        user.roleId = settings.value(QStringLiteral("role")).toString();
        user.secret = settings.value(QStringLiteral("secret")).toString();
        user.enabled = settings.value(QStringLiteral("enabled"), true).toBool();
        user.lastSignIn = settings.value(QStringLiteral("lastSignIn")).toDateTime();
        user.granted = setFromString(settings.value(QStringLiteral("granted")).toString());
        user.revoked = setFromString(settings.value(QStringLiteral("revoked")).toString());
        if (!user.username.isEmpty())
            m_users.append(user);
    }
    settings.endArray();
}

void AccessControl::persist()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

    QSettings settings(accountsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("AccessControl/SignInRequired"), m_signInRequired);
    settings.setValue(QStringLiteral("AccessControl/AutoSignInUser"), m_autoSignInUser);
    settings.setValue(QStringLiteral("AccessControl/AutoLockMinutes"), m_autoLockMinutes);
    settings.setValue(QStringLiteral("AccessControl/LastUser"), m_lastUser);

    settings.remove(QStringLiteral("Roles"));
    settings.beginWriteArray(QStringLiteral("Roles"), m_roles.size());
    for (int i = 0; i < m_roles.size(); ++i) {
        settings.setArrayIndex(i);
        const Role &role = m_roles.at(i);
        settings.setValue(QStringLiteral("id"), role.id);
        settings.setValue(QStringLiteral("name"), role.name);
        settings.setValue(QStringLiteral("description"), role.description);
        settings.setValue(QStringLiteral("everything"), role.everything);
        settings.setValue(QStringLiteral("builtin"), role.builtin);
        settings.setValue(QStringLiteral("permissions"), setToString(role.permissions));
    }
    settings.endArray();

    settings.remove(QStringLiteral("Users"));
    settings.beginWriteArray(QStringLiteral("Users"), m_users.size());
    for (int i = 0; i < m_users.size(); ++i) {
        settings.setArrayIndex(i);
        const User &user = m_users.at(i);
        settings.setValue(QStringLiteral("username"), user.username);
        settings.setValue(QStringLiteral("displayName"), user.displayName);
        settings.setValue(QStringLiteral("role"), user.roleId);
        settings.setValue(QStringLiteral("secret"), user.secret);
        settings.setValue(QStringLiteral("enabled"), user.enabled);
        settings.setValue(QStringLiteral("lastSignIn"), user.lastSignIn);
        settings.setValue(QStringLiteral("granted"), setToString(user.granted));
        settings.setValue(QStringLiteral("revoked"), setToString(user.revoked));
    }
    settings.endArray();

    settings.sync();

    // Owner-only, the same treatment the credentials in xfb.conf get. It is
    // not what makes this safe — see the class comment — but leaving a file of
    // password digests world-readable would be careless.
    SecretStore::restrictFile(accountsPath());
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

bool AccessControl::isProtected() const
{
    for (const User &user : m_users) {
        if (user.enabled)
            return true;
    }
    return false;
}

bool AccessControl::signInRequired() const
{
    return isProtected() && m_signInRequired;
}

void AccessControl::setSignInRequired(bool required)
{
    if (m_signInRequired == required)
        return;
    m_signInRequired = required;
    persist();
}

QString AccessControl::autoSignInUser() const
{
    return m_autoSignInUser;
}

void AccessControl::setAutoSignInUser(const QString &username)
{
    if (m_autoSignInUser == username)
        return;
    m_autoSignInUser = username;
    persist();
}

int AccessControl::autoLockMinutes() const
{
    return isProtected() ? m_autoLockMinutes : 0;
}

void AccessControl::setAutoLockMinutes(int minutes)
{
    if (m_autoLockMinutes == minutes)
        return;
    m_autoLockMinutes = qMax(0, minutes);
    persist();
    emit sessionChanged();
}

// ---------------------------------------------------------------------------
// Roles and users
// ---------------------------------------------------------------------------

AccessControl::Role AccessControl::role(const QString &id) const
{
    for (const Role &role : m_roles) {
        if (role.id == id)
            return role;
    }
    return Role();
}

AccessControl::Role AccessControl::roleOf(const User &user) const
{
    return role(user.roleId);
}

AccessControl::User AccessControl::user(const QString &username) const
{
    for (const User &candidate : m_users) {
        if (candidate.username.compare(username, Qt::CaseInsensitive) == 0)
            return candidate;
    }
    return User();
}

QSet<QString> AccessControl::effectivePermissions(const User &user) const
{
    const Role userRole = roleOf(user);

    QSet<QString> keys;
    if (userRole.everything) {
        for (const Permission &permission : catalogue()) {
            // "Everything" is everything this XFB and every later one can do —
            // except the reserved permissions, which are not part of the set an
            // administrator is administrator of. Only the one operator they
            // answer to has them without being handed them.
            if (isReserved(permission.key) && !isReservedOperator(user))
                continue;
            keys.insert(permission.key);
        }
    } else {
        keys = userRole.permissions;
    }

    keys.unite(user.granted);
    keys.subtract(user.revoked);
    return keys;
}

bool AccessControl::save(const QList<Role> &roles, const QList<User> &users, QString *reason)
{
    // The one thing this refuses: an installation nobody can administer. Every
    // other shape — no presenters, a role with nothing ticked, a disabled
    // account — is somebody's deliberate decision.
    bool reachable = false;
    for (const User &user : users) {
        if (!user.enabled)
            continue;
        for (const Role &role : roles) {
            if (role.id != user.roleId)
                continue;
            if (role.everything || role.permissions.contains(QStringLiteral("admin.users"))
                || user.granted.contains(QStringLiteral("admin.users")))
                reachable = !user.revoked.contains(QStringLiteral("admin.users"));
            break;
        }
        if (reachable)
            break;
    }

    // An empty set is how protection is removed, and that is allowed: it is
    // removeProtection() calling in.
    if (!users.isEmpty() && !reachable) {
        if (reason)
            *reason = tr("Somebody has to be able to manage the accounts. Leave at "
                         "least one enabled account with a role that may manage "
                         "users and roles.");
        return false;
    }

    m_roles = roles;
    m_users = users;
    persist();
    refreshCurrentFromStore();
    reapplyGuards();
    emit sessionChanged();
    return true;
}

bool AccessControl::createInitialAdministrator(const QString &username,
                                               const QString &displayName,
                                               const QString &password,
                                               QString *reason)
{
    if (isProtected()) {
        if (reason)
            *reason = tr("This installation already has accounts.");
        return false;
    }
    if (username.trimmed().isEmpty()) {
        if (reason)
            *reason = tr("An account needs a user name.");
        return false;
    }
    if (password.isEmpty()) {
        if (reason)
            *reason = tr("An account needs a password. Without one there is nothing "
                         "to protect.");
        return false;
    }

    m_roles = defaultRoles();

    User administrator;
    administrator.username = username.trimmed();
    administrator.displayName = displayName.trimmed().isEmpty() ? username.trimmed()
                                                                : displayName.trimmed();
    administrator.roleId = QStringLiteral("admin");
    administrator.secret = hashPassword(password);
    administrator.enabled = true;
    m_users = {administrator};

    m_signInRequired = true;
    persist();

    // Whoever just created the account is that account: asking them to type
    // the password they invented five seconds ago would be theatre.
    m_current = administrator;
    m_signedIn = true;
    m_lastUser = administrator.username;

    reapplyGuards();
    emit sessionChanged();
    return true;
}

bool AccessControl::removeProtection(QString *reason)
{
    if (!isAdministrator()) {
        if (reason)
            *reason = tr("Only an administrator can turn the protection off.");
        return false;
    }

    m_roles.clear();
    m_users.clear();
    m_current = User();
    m_signedIn = false;
    m_autoSignInUser.clear();
    m_autoLockMinutes = 0;
    m_lastUser.clear();
    persist();

    reapplyGuards();
    emit sessionChanged();
    return true;
}

// ---------------------------------------------------------------------------
// The session
// ---------------------------------------------------------------------------

QString AccessControl::currentRoleName() const
{
    if (!isProtected())
        return tr("Unprotected");
    if (!m_signedIn)
        return tr("Not signed in");
    const Role userRole = roleOf(m_current);
    return userRole.name.isEmpty() ? tr("No role") : userRole.name;
}

bool AccessControl::isAdministrator() const
{
    if (!isProtected())
        return true;
    if (!m_signedIn)
        return false;
    return effectivePermissions(m_current).contains(QStringLiteral("admin.users"));
}

bool AccessControl::signIn(const QString &username, const QString &password, QString *reason)
{
    const User candidate = user(username);

    // The same sentence whichever half was wrong, so a wrong guess does not
    // tell somebody standing at the desk which user names exist.
    const QString refusal = tr("That user name and password do not match an account.");

    if (candidate.username.isEmpty() || candidate.secret.isEmpty()) {
        if (reason)
            *reason = refusal;
        return false;
    }
    if (!verifyPassword(password, candidate.secret)) {
        if (reason)
            *reason = refusal;
        return false;
    }
    if (!candidate.enabled) {
        if (reason)
            *reason = tr("That account has been switched off. An administrator "
                         "can turn it back on.");
        return false;
    }

    m_current = candidate;
    m_signedIn = true;
    m_lastUser = candidate.username;

    for (User &stored : m_users) {
        if (stored.username.compare(candidate.username, Qt::CaseInsensitive) == 0) {
            stored.lastSignIn = QDateTime::currentDateTime();
            m_current.lastSignIn = stored.lastSignIn;
            break;
        }
    }
    persist();

    reapplyGuards();
    emit sessionChanged();
    return true;
}

void AccessControl::signOut()
{
    if (!m_signedIn)
        return;
    m_current = User();
    m_signedIn = false;
    reapplyGuards();
    emit sessionChanged();
}

bool AccessControl::signInAutomatically()
{
    if (!isProtected())
        return true;

    User chosen;
    if (!m_autoSignInUser.isEmpty()) {
        const User named = user(m_autoSignInUser);
        if (named.enabled)
            chosen = named;
    }

    if (chosen.username.isEmpty()) {
        // Nothing named: come up as the account that can do least, so a
        // machine that boots on its own is never left wide open.
        int fewest = -1;
        for (const User &candidate : m_users) {
            if (!candidate.enabled)
                continue;
            const int count = effectivePermissions(candidate).size();
            if (fewest < 0 || count < fewest) {
                fewest = count;
                chosen = candidate;
            }
        }
    }

    if (chosen.username.isEmpty())
        return false;

    m_current = chosen;
    m_signedIn = true;
    reapplyGuards();
    emit sessionChanged();
    return true;
}

void AccessControl::refreshCurrentFromStore()
{
    if (!m_signedIn)
        return;
    const User stored = user(m_current.username);
    if (stored.username.isEmpty() || !stored.enabled) {
        // The account signing this session was deleted or switched off while
        // it was open. It keeps the window it has — throwing an operator off a
        // running station is not something an edit in another window should do
        // — but it keeps nothing else.
        m_current = User();
        m_signedIn = false;
        return;
    }
    m_current = stored;
}

// ---------------------------------------------------------------------------
// Permissions
// ---------------------------------------------------------------------------

bool AccessControl::allows(const QString &permission) const
{
    if (!isProtected()) {
        // The one exception to unprotected-grants-everything. A station that
        // has never created an account has nobody who could have switched
        // these on, so they are not there — which is the whole point of
        // reserving them.
        return !isReserved(permission);
    }
    if (!m_signedIn)
        return false;
    return effectivePermissions(m_current).contains(permission);
}

bool AccessControl::demand(const QString &permission, QWidget *parent)
{
    if (allows(permission))
        return true;

    QMessageBox::information(
        parent, tr("Not allowed"),
        tr("%1 is not available to the %2 role.\n\n"
           "An administrator can allow it in Options ▸ Users and roles.")
            .arg(labelFor(permission), currentRoleName()));
    return false;
}

void AccessControl::guard(QAction *action, const QString &permission, WhenDenied whenDenied)
{
    if (!action)
        return;

    auto *guarded = new GuardedAction;
    guarded->action = action;
    guarded->permission = permission;
    guarded->whenDenied = whenDenied;
    m_guards.append(guarded);

    // XFB enables and disables a great many of its actions from the state of
    // the playlist, the transport and the database. A permission has to beat
    // every one of those, so rather than setting the state once, watch for it
    // being changed back.
    connect(action, &QAction::changed, this, [this, guarded]() {
        if (guarded->applying || !guarded->action)
            return;
        if (allows(guarded->permission))
            return;
        const bool showing = guarded->whenDenied == WhenDenied::Hide
                             && guarded->action->isVisible();
        if (guarded->action->isEnabled() || showing)
            applyGuard(guarded);
    });

    applyGuard(guarded);
}

void AccessControl::applyGuard(GuardedAction *guarded)
{
    if (!guarded || !guarded->action)
        return;

    const bool allowed = allows(guarded->permission);

    guarded->applying = true;

    if (guarded->whenDenied == WhenDenied::Hide) {
        // Nothing to explain and nothing to restore: the entry is either part
        // of this station or it is not. Disabled as well as hidden, because an
        // action still answers its keyboard shortcut while it is out of sight.
        guarded->action->setVisible(allowed);
        guarded->action->setEnabled(allowed);
        guarded->applying = false;
        return;
    }

    if (allowed) {
        guarded->action->setEnabled(true);
        if (!guarded->blockedTip.isEmpty()
            && guarded->action->toolTip() == guarded->blockedTip) {
            guarded->action->setToolTip(guarded->originalTip);
        }
    } else {
        if (guarded->blockedTip.isEmpty()
            || guarded->action->toolTip() != guarded->blockedTip) {
            guarded->originalTip = guarded->action->toolTip();
        }
        guarded->blockedTip = tr("Not available to the %1 role.").arg(currentRoleName());
        guarded->action->setEnabled(false);
        guarded->action->setToolTip(guarded->blockedTip);
    }
    guarded->applying = false;
}

void AccessControl::reapplyGuards()
{
    for (auto it = m_guards.begin(); it != m_guards.end();) {
        if (!(*it)->action) {
            delete *it;
            it = m_guards.erase(it);
            continue;
        }
        applyGuard(*it);
        ++it;
    }
}
