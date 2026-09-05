#include <QtTest/QtTest>

#include <QAction>
#include <QFile>
#include <QStandardPaths>

#include "services/AccessControl.h"

/**
 * @brief Operator accounts, roles, and the permission behind every menu entry.
 *
 * Four things here are worth a test rather than a read-through, because
 * getting any of them wrong is either a station locked out of its own playout
 * or a permission that quietly grants everything:
 *
 *  - an installation with no accounts grants everything, so a station that
 *    updates into this version notices nothing;
 *  - a password is never recoverable from what is stored, and two digests of
 *    one password differ;
 *  - guard() keeps holding — a lot of XFB enables and disables its actions
 *    from the state of the playlist, and a permission has to win every one of
 *    those arguments;
 *  - the last administrator cannot be removed.
 *
 * A QApplication rather than a QCoreApplication because QAction is a GUI
 * class, and the offscreen platform because there is no display in CI.
 */
class TestAccessControl : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void anUnprotectedInstallationGrantsEverything();
    void passwordsAreSaltedAndNotRecoverable();
    void theFirstAccountIsAnAdministrator();
    void theShippedRolesSayWhatTheyMean();
    void aRoleIsWhatTheAccountMayDo();
    void exceptionsOverrideTheRole();
    void guardBeatsALaterSetEnabled();
    void guardFollowsTheSession();
    void theLastAdministratorCannotBeRemoved();
    void nothingSurvivesInPlaintext();

private:
    /** Back to an installation nobody has protected. */
    void reset();
    AccessControl &access() { return AccessControl::instance(); }
};

void TestAccessControl::initTestCase()
{
    // Its own settings scope, so a developer's real accounts are never touched.
    QCoreApplication::setOrganizationName(QStringLiteral("XFB-TestAccessControl"));
    QCoreApplication::setApplicationName(QStringLiteral("XFB-TestAccessControl"));
}

void TestAccessControl::reset()
{
    QString reason;
    if (access().isProtected()) {
        // Only an administrator may turn the protection off, and a test may
        // have left the session as somebody else — or as nobody, having just
        // deleted the account it was signed in as. Try the accounts these
        // tests create until one of them is an administrator.
        // Every credential pair these tests create. Keep it complete: an
        // account left behind by an earlier *run* of this binary is loaded
        // from disk before the first test, and one whose password is not
        // listed here would leave the suite unable to reset itself.
        const QVector<QPair<QString, QString>> known{
            {QStringLiteral("admin"), QStringLiteral("first pass")},
            {QStringLiteral("admin"), QStringLiteral("hunter2 was here")},
            {QStringLiteral("anna"), QStringLiteral("mic check")},
        };
        for (const auto &credentials : known) {
            if (access().isAdministrator())
                break;
            access().signIn(credentials.first, credentials.second, &reason);
        }
        access().removeProtection(&reason);
    }
    QFile::remove(AccessControl::storagePath());
    QVERIFY(!access().isProtected());
}

void TestAccessControl::init()
{
    reset();
}

void TestAccessControl::cleanupTestCase()
{
    // Leave nothing on disk: the accounts file outlives the process, and the
    // next run reads it before the first test can clear it.
    reset();
}

void TestAccessControl::anUnprotectedInstallationGrantsEverything()
{
    QVERIFY(!access().isProtected());
    QVERIFY(!access().signInRequired());
    QVERIFY(access().isAdministrator());
    for (const AccessControl::Permission &permission : AccessControl::catalogue())
        QVERIFY2(access().allows(permission.key), qPrintable(permission.key));
}

void TestAccessControl::passwordsAreSaltedAndNotRecoverable()
{
    const QString digest = AccessControl::hashPassword(QStringLiteral("correct horse"));
    QVERIFY(digest.startsWith(QStringLiteral("pbkdf2-sha256$")));
    QVERIFY(!digest.contains(QStringLiteral("correct horse")));
    QVERIFY(AccessControl::verifyPassword(QStringLiteral("correct horse"), digest));
    QVERIFY(!AccessControl::verifyPassword(QStringLiteral("Correct horse"), digest));
    QVERIFY(!AccessControl::verifyPassword(QString(), digest));

    // Salted: the same password twice is two different digests.
    QVERIFY(AccessControl::hashPassword(QStringLiteral("x"))
            != AccessControl::hashPassword(QStringLiteral("x")));

    // Nothing that is not a digest verifies as one.
    QVERIFY(!AccessControl::verifyPassword(QStringLiteral("x"), QStringLiteral("x")));
    QVERIFY(!AccessControl::verifyPassword(QStringLiteral("x"), QString()));
}

void TestAccessControl::theFirstAccountIsAnAdministrator()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QStringLiteral("The Owner"),
                                                QStringLiteral("first pass"), &reason));
    QVERIFY(access().isProtected());
    QVERIFY(access().signInRequired());
    QVERIFY(access().isAdministrator());
    QVERIFY(access().allows(QStringLiteral("admin.users")));

    // There is only ever one first account.
    QVERIFY(!access().createInitialAdministrator(QStringLiteral("other"), QString(),
                                                 QStringLiteral("x"), &reason));
    QVERIFY(!reason.isEmpty());

    // And it needs something to sign in with.
    reset();
    QVERIFY(!access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                 QString(), &reason));
    QVERIFY(!access().createInitialAdministrator(QString(), QString(),
                                                 QStringLiteral("x"), &reason));
    QVERIFY(!access().isProtected());
}

void TestAccessControl::theShippedRolesSayWhatTheyMean()
{
    const QList<AccessControl::Role> roles = AccessControl::defaultRoles();
    QCOMPARE(roles.size(), 4);

    for (const AccessControl::Role &role : roles) {
        QVERIFY(role.builtin);
        QVERIFY(!role.id.isEmpty());
        // Only the administrator gets what a later version of XFB adds.
        QCOMPARE(role.everything, role.id == QLatin1String("admin"));
        // Everything a role names has to be a permission that exists.
        for (const QString &key : role.permissions)
            QVERIFY2(AccessControl::labelFor(key) != key, qPrintable(key));
    }

    AccessControl::Role presenter;
    AccessControl::Role guest;
    for (const AccessControl::Role &role : roles) {
        if (role.id == QLatin1String("presenter"))
            presenter = role;
        if (role.id == QLatin1String("guest"))
            guest = role;
    }
    QVERIFY(presenter.permissions.contains(QStringLiteral("playback.transport")));
    QVERIFY(!presenter.permissions.contains(QStringLiteral("library.purge")));
    QVERIFY(!presenter.permissions.contains(QStringLiteral("admin.users")));
    QVERIFY(guest.permissions.contains(QStringLiteral("playback.transport")));
    QVERIFY(!guest.permissions.contains(QStringLiteral("playlist.edit")));
}

void TestAccessControl::aRoleIsWhatTheAccountMayDo()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                QStringLiteral("first pass"), &reason));

    QList<AccessControl::User> users = access().users();
    AccessControl::User presenter;
    presenter.username = QStringLiteral("anna");
    presenter.roleId = QStringLiteral("presenter");
    presenter.secret = AccessControl::hashPassword(QStringLiteral("mic check"));
    users.append(presenter);
    QVERIFY2(access().save(access().roles(), users, &reason), qPrintable(reason));

    access().signOut();
    QVERIFY(!access().hasSession());
    QVERIFY(!access().allows(QStringLiteral("playback.transport")));

    QVERIFY(!access().signIn(QStringLiteral("anna"), QStringLiteral("wrong"), &reason));
    QVERIFY(!access().signIn(QStringLiteral("nobody"), QStringLiteral("mic check"), &reason));
    QVERIFY(access().signIn(QStringLiteral("anna"), QStringLiteral("mic check"), &reason));

    QVERIFY(access().allows(QStringLiteral("playback.transport")));
    QVERIFY(!access().allows(QStringLiteral("library.purge")));
    QVERIFY(!access().isAdministrator());

    // A switched-off account keeps everything and gets in nowhere.
    access().signIn(QStringLiteral("admin"), QStringLiteral("first pass"), &reason);
    users = access().users();
    for (AccessControl::User &user : users) {
        if (user.username == QLatin1String("anna"))
            user.enabled = false;
    }
    QVERIFY(access().save(access().roles(), users, &reason));
    QVERIFY(!access().signIn(QStringLiteral("anna"), QStringLiteral("mic check"), &reason));
}

void TestAccessControl::exceptionsOverrideTheRole()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                QStringLiteral("first pass"), &reason));

    QList<AccessControl::User> users = access().users();
    AccessControl::User anna;
    anna.username = QStringLiteral("anna");
    anna.roleId = QStringLiteral("presenter");
    anna.secret = AccessControl::hashPassword(QStringLiteral("mic check"));
    anna.granted = {QStringLiteral("library.add.jingle")};
    anna.revoked = {QStringLiteral("playlist.clear")};
    users.append(anna);
    QVERIFY(access().save(access().roles(), users, &reason));

    QVERIFY(access().signIn(QStringLiteral("anna"), QStringLiteral("mic check"), &reason));
    QVERIFY(access().allows(QStringLiteral("library.add.jingle")));  // granted to her alone
    QVERIFY(!access().allows(QStringLiteral("playlist.clear")));     // withdrawn from her alone
    QVERIFY(access().allows(QStringLiteral("playlist.save")));       // the role, untouched
}

void TestAccessControl::guardBeatsALaterSetEnabled()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                QStringLiteral("first pass"), &reason));
    QList<AccessControl::User> users = access().users();
    AccessControl::User anna;
    anna.username = QStringLiteral("anna");
    anna.roleId = QStringLiteral("guest");
    anna.secret = AccessControl::hashPassword(QStringLiteral("mic check"));
    users.append(anna);
    QVERIFY(access().save(access().roles(), users, &reason));
    QVERIFY(access().signIn(QStringLiteral("anna"), QStringLiteral("mic check"), &reason));

    QAction blocked;
    blocked.setToolTip(QStringLiteral("Empty the library"));
    access().guard(&blocked, QStringLiteral("library.purge"));
    QVERIFY(!blocked.isEnabled());
    QVERIFY(blocked.toolTip() != QLatin1String("Empty the library"));

    // What the rest of the application does all day, from the state of the
    // playlist and the transport. The permission has to win.
    blocked.setEnabled(true);
    QVERIFY(!blocked.isEnabled());

    QAction allowed;
    access().guard(&allowed, QStringLiteral("playback.transport"));
    QVERIFY(allowed.isEnabled());
}

void TestAccessControl::guardFollowsTheSession()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                QStringLiteral("first pass"), &reason));
    QList<AccessControl::User> users = access().users();
    AccessControl::User anna;
    anna.username = QStringLiteral("anna");
    anna.roleId = QStringLiteral("guest");
    anna.secret = AccessControl::hashPassword(QStringLiteral("mic check"));
    users.append(anna);
    QVERIFY(access().save(access().roles(), users, &reason));

    QAction action;
    action.setToolTip(QStringLiteral("Empty the library"));
    access().guard(&action, QStringLiteral("library.purge"));
    QVERIFY(action.isEnabled());   // the administrator is still signed in

    QVERIFY(access().signIn(QStringLiteral("anna"), QStringLiteral("mic check"), &reason));
    QVERIFY(!action.isEnabled());

    QVERIFY(access().signIn(QStringLiteral("admin"), QStringLiteral("first pass"), &reason));
    QVERIFY(action.isEnabled());
    QCOMPARE(action.toolTip(), QStringLiteral("Empty the library"));
}

void TestAccessControl::theLastAdministratorCannotBeRemoved()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                QStringLiteral("first pass"), &reason));

    QList<AccessControl::User> users = access().users();
    AccessControl::User anna;
    anna.username = QStringLiteral("anna");
    anna.roleId = QStringLiteral("presenter");
    anna.secret = AccessControl::hashPassword(QStringLiteral("mic check"));
    users.append(anna);
    QVERIFY(access().save(access().roles(), users, &reason));

    // Deleting the only administrator: refused, and nothing is written.
    QList<AccessControl::User> withoutAdmin;
    for (const AccessControl::User &user : access().users()) {
        if (user.username == QLatin1String("anna"))
            withoutAdmin.append(user);
    }
    QVERIFY(!access().save(access().roles(), withoutAdmin, &reason));
    QVERIFY(!reason.isEmpty());
    QCOMPARE(access().users().size(), 2);

    // Switching the only administrator off is the same mistake wearing a hat.
    QList<AccessControl::User> disabledAdmin = access().users();
    for (AccessControl::User &user : disabledAdmin) {
        if (user.username == QLatin1String("admin"))
            user.enabled = false;
    }
    QVERIFY(!access().save(access().roles(), disabledAdmin, &reason));

    // Promoting somebody else first makes it allowed.
    QList<AccessControl::User> promoted = access().users();
    for (AccessControl::User &user : promoted) {
        if (user.username == QLatin1String("anna"))
            user.roleId = QStringLiteral("admin");
    }
    QVERIFY(access().save(access().roles(), promoted, &reason));
    QList<AccessControl::User> annaAlone;
    for (const AccessControl::User &user : access().users()) {
        if (user.username == QLatin1String("anna"))
            annaAlone.append(user);
    }
    QVERIFY2(access().save(access().roles(), annaAlone, &reason), qPrintable(reason));
    QCOMPARE(access().users().size(), 1);
}

void TestAccessControl::nothingSurvivesInPlaintext()
{
    QString reason;
    QVERIFY(access().createInitialAdministrator(QStringLiteral("admin"), QString(),
                                                QStringLiteral("hunter2 was here"), &reason));

    QFile file(AccessControl::storagePath());
    QVERIFY(file.exists());

    // Owner-only. Not what makes this safe, but leaving a file of digests
    // world-readable would be careless.
    const QFile::Permissions mode = file.permissions();
    QVERIFY(!mode.testFlag(QFile::ReadGroup));
    QVERIFY(!mode.testFlag(QFile::ReadOther));

    QVERIFY(file.open(QIODevice::ReadOnly));
    const QString written = QString::fromUtf8(file.readAll());
    QVERIFY(!written.contains(QStringLiteral("hunter2")));
    QVERIFY(written.contains(QStringLiteral("pbkdf2-sha256")));
}

QTEST_MAIN(TestAccessControl)
#include "TestAccessControl.moc"
