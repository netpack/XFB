#ifndef ACCESSCONTROL_H
#define ACCESSCONTROL_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

class QAction;
class QWidget;

/**
 * @brief Who is at the desk, and what that person is allowed to do with XFB.
 *
 * A station is not one person. The owner installs XFB, a producer prepares
 * the week, a presenter drives the four hours they are on air, and somebody
 * covering a sick shift sits down at the same machine having never seen it
 * before. Until now they were all the same operator: every one of them could
 * empty the library, re-encode the whole catalogue, hand out a pairing token
 * or change the stream credentials, because the application had no idea who
 * was typing.
 *
 * This is the answer, and it is deliberately a small one:
 *
 *  - **Roles** are named sets of permissions — Administrator, Producer,
 *    Presenter, Guest to begin with, any number of others afterwards.
 *  - **Users** are people, each with a password and exactly one role, plus
 *    the odd exception granted or withdrawn just for them.
 *  - **Permissions** are one per thing XFB can be asked to do: each menu
 *    entry, each destructive sweep, each station-wide window. The catalogue
 *    below *is* the list an administrator ticks through.
 *
 * ### Nothing changes until somebody asks for it
 *
 * An installation with no accounts is unprotected, and unprotected means
 * exactly what XFB has always done: no sign-in, every permission granted. An
 * existing station that updates into this version notices nothing. Protection
 * starts the moment an administrator is created (createInitialAdministrator()),
 * which is what "the first installation has an admin" means in practice — the
 * first account made is always an Administrator, because there would otherwise
 * be nobody able to make the second one.
 *
 * ### What this is, and what it is not
 *
 * It is a way to keep an eleven o'clock presenter from converting the whole
 * library to Opus by opening the wrong menu — the kind of accident that ends
 * a station's evening. It is not protection against the person who owns the
 * computer: the accounts live in a file next to xfb.conf, and anybody with a
 * shell and the operator's login can delete it and be an administrator again.
 * That is stated plainly in the administration window rather than hidden,
 * because a claim of security that the storage cannot keep is worse than no
 * claim at all. Passwords themselves are never recoverable from the file —
 * they are stored as PBKDF2-SHA256 digests with a per-user salt, so a copy of
 * the file does not hand over the passwords, which are the thing an operator
 * is most likely to have reused elsewhere.
 *
 * ### Refusing an action
 *
 * A blocked action is *disabled and explained*, not hidden. A presenter who
 * cannot find "Add a jingle" at all will phone somebody; a presenter who sees
 * it greyed out with "not available to the Presenter role" knows both that it
 * exists and why it will not open. Disabled entries also keep their place in
 * the menus, so an operator who learned where things are does not have to
 * learn again on a machine where they have fewer rights. guard() is what
 * makes that happen, and it keeps holding: an action bound to a permission is
 * pushed back to disabled even if some other part of the application enables
 * it later.
 *
 * The exception is the handful of features a station *switches on* rather than
 * merely allows — the external downloader and the torrent tab. Those are off
 * for every role until they are ticked, so on the great majority of desks a
 * greyed-out entry would be advertising something the station has decided not
 * to have. Those are guarded with WhenDenied::Hide and are simply not there.
 *
 * ### Reserved permissions
 *
 * Those same two are further *reserved*: the rows for them appear in the
 * administration window only for the operator named in kReservedOperator, and
 * an administrator does not inherit them from having everything. An
 * installation nobody has protected — which is every station that has not
 * created an account — does not have them at all, which is the one place the
 * unprotected-grants-everything rule does not hold. The effect is that these
 * two features exist only where that one operator has deliberately switched
 * them on, and are invisible everywhere else rather than merely refused.
 * isReserved() names them; mayAdministerReserved() is who may see them.
 */
class AccessControl : public QObject
{
    Q_OBJECT

public:
    /** The single instance every part of the application talks to. */
    static AccessControl &instance();

    // -------------------------------------------------------- the catalogue --

    /** One thing XFB can be asked to do, as an administrator sees it. */
    struct Permission
    {
        QString key;       ///< stable identifier, written to the accounts file
        QString category;  ///< the heading it is listed under
        QString label;     ///< what the menu entry itself says
        QString hint;      ///< what allowing it lets somebody do
    };

    /** A named set of permissions. */
    struct Role
    {
        QString id;                  ///< stable identifier, referenced by users
        QString name;                ///< as shown
        QString description;
        QSet<QString> permissions;
        /** Everything, present and future. Only the Administrator role has it,
         *  so a role written before a feature existed cannot silently gain it. */
        bool everything = false;
        /** Shipped with XFB: it may be edited (except Administrator) but the
         *  four built-in roles cannot be deleted, so an installation always
         *  has somewhere to put a new user. */
        bool builtin = false;
    };

    /** One person who signs in. */
    struct User
    {
        QString username;      ///< what they type, matched case-insensitively
        QString displayName;
        QString roleId;
        QString secret;        ///< PBKDF2 digest; see hashPassword()
        bool enabled = true;   ///< false keeps the account without letting it in
        QDateTime lastSignIn;
        /** Exceptions on top of the role, for the one presenter who is also
         *  allowed to run the advert import. */
        QSet<QString> granted;
        QSet<QString> revoked;
    };

    /** Every permission XFB knows about, in the order the editor lists them. */
    static const QVector<Permission> &catalogue();
    /**
     * True for a permission that is not an ordinary administrator's to hand
     * out — see the note about reserved permissions above. It is still a
     * permission like any other once granted; what is reserved is the ability
     * to grant it and to see that it exists.
     */
    static bool isReserved(const QString &key);
    /** True when @p user is the one operator the reserved permissions answer
     *  to. Matched on the user name, case-insensitively, like signing in. */
    static bool isReservedOperator(const User &user);
    /** True when this session may see the reserved permissions in the
     *  administration window, and hand them out there. */
    bool mayAdministerReserved() const;
    /** The category headings, in catalogue order, without repeats. */
    static QStringList categories();
    /** The label for a key, or the key itself when it is from a newer XFB. */
    static QString labelFor(const QString &key);

    // ------------------------------------------------------------- the state --

    /** True once an administrator exists: XFB asks who is there and enforces
     *  what they may do. False on an installation nobody has protected, where
     *  every permission is granted to everyone. */
    bool isProtected() const;
    /** Whether sign-in is asked for at startup. Only meaningful when
     *  protected; an administrator can turn it off to keep the permissions
     *  while letting the machine come up on its own after a power cut, in
     *  which case XFB runs as the account named in autoSignInUser(). */
    bool signInRequired() const;
    void setSignInRequired(bool required);
    /** The account XFB comes up as when sign-in is not asked for. Empty means
     *  the least privileged enabled account. */
    QString autoSignInUser() const;
    void setAutoSignInUser(const QString &username);
    /** Lock the screen after this many idle minutes. 0 (the default) never
     *  locks: a studio machine that locks itself in the middle of a show is a
     *  worse fault than an unattended one. */
    int autoLockMinutes() const;
    void setAutoLockMinutes(int minutes);

    // ---------------------------------------------------------- the session --

    /** Who signed in last, so the sign-in question and the lock screen can
     *  fill the name in and leave the cursor in the password. Only a name —
     *  it grants nothing. */
    QString lastSignedInUser() const { return m_lastUser; }
    /** The signed-in account, empty on an unprotected installation. */
    User currentUser() const { return m_current; }
    /** True when somebody is signed in (or the installation is unprotected). */
    bool hasSession() const { return !isProtected() || m_signedIn; }
    /** The role of whoever is signed in, as shown in the window title. */
    QString currentRoleName() const;
    /** True when the current session may manage users and roles. */
    bool isAdministrator() const;

    /** Check a password and, when it is right, make that account the session.
     *  @param reason receives why it failed, ready to show. */
    bool signIn(const QString &username, const QString &password, QString *reason = nullptr);
    /** Drop the session without ending playback. */
    void signOut();
    /** Sign in as the account startup uses when nobody is asked. */
    bool signInAutomatically();

    // ------------------------------------------------------- the permissions --

    /** True when the current session may do @p permission. Always true on an
     *  unprotected installation, and always true for an administrator. */
    bool allows(const QString &permission) const;
    /** allows(), but says no out loud. Use it at the top of a slot that has to
     *  refuse even though its menu entry was somehow reachable. */
    bool demand(const QString &permission, QWidget *parent);
    /** What an administrator has effectively given @p user. */
    QSet<QString> effectivePermissions(const User &user) const;

    /** What a guarded action does when the session may not use it. */
    enum class WhenDenied
    {
        /** Greyed out, with a tooltip saying which role it belongs to. The
         *  default, and what every ordinary menu entry wants: see the note
         *  about refusing an action above. */
        Disable,
        /** Taken out of the menu altogether. Reserved for the few features an
         *  administrator switches on for the station rather than merely
         *  allows — the external downloader, the torrent tab. Those are off
         *  for everybody until somebody ticks them, so a permanently greyed
         *  entry would be clutter on most desks rather than a clue. */
        Hide,
    };

    /**
     * Bind a menu entry (or any other action) to a permission.
     *
     * The action is disabled and its tooltip explains why whenever the session
     * may not use it, and is left alone otherwise. The binding survives: the
     * action is re-checked when somebody signs in or out, and pushed back to
     * disabled if other code enables it — a lot of XFB's actions are enabled
     * and disabled by the state of the playlist, and a permission has to win
     * every one of those arguments. WhenDenied::Hide keeps the entry out of
     * sight instead, and holds it there the same way.
     */
    void guard(QAction *action, const QString &permission,
               WhenDenied whenDenied = WhenDenied::Disable);
    /** Re-check every guarded action. Called on every session change. */
    void reapplyGuards();

    // ---------------------------------------------------- administration API --

    QList<Role> roles() const { return m_roles; }
    QList<User> users() const { return m_users; }
    Role role(const QString &id) const;
    /** The role a user has, or an empty role when it has been deleted. */
    Role roleOf(const User &user) const;
    User user(const QString &username) const;

    /** Replace the whole set. The caller (the administration window) has
     *  already decided the result is consistent; this refuses only the two
     *  things that would lock everybody out: no Administrator role, and no
     *  enabled account holding it. @param reason receives why. */
    bool save(const QList<Role> &roles, const QList<User> &users, QString *reason = nullptr);

    /** Create the first account. It is an Administrator by construction, and
     *  it is what turns protection on. */
    bool createInitialAdministrator(const QString &username, const QString &displayName,
                                    const QString &password, QString *reason = nullptr);
    /** Turn protection off and delete every account. Administrators only. */
    bool removeProtection(QString *reason = nullptr);

    /** The four roles a fresh installation starts with. */
    static QList<Role> defaultRoles();

    /** PBKDF2-SHA256 with a random salt, stored as
     *  "pbkdf2-sha256$<iterations>$<salt base64>$<digest base64>". */
    static QString hashPassword(const QString &password);
    static bool verifyPassword(const QString &password, const QString &stored);

    /** Where the accounts live, for the sentence the window shows. */
    static QString storagePath();

signals:
    /** Somebody signed in, signed out, or an administrator changed the rules. */
    void sessionChanged();

private:
    explicit AccessControl(QObject *parent = nullptr);

    /** One action bound to one permission, and what its tooltip said before
     *  the permission took it over. */
    struct GuardedAction
    {
        QPointer<QAction> action;
        QString permission;
        QString originalTip;     ///< restored when the permission is granted
        QString blockedTip;      ///< the sentence currently standing in for it
        WhenDenied whenDenied = WhenDenied::Disable;
        bool applying = false;   ///< re-entry guard for QAction::changed
    };

    void load();
    void persist();
    /** Point the session at the stored copy of the signed-in account, so a
     *  permission an administrator has just changed applies to them too. */
    void refreshCurrentFromStore();
    void applyGuard(GuardedAction *guarded);

    QList<Role> m_roles;
    QList<User> m_users;
    User m_current;
    bool m_signedIn = false;
    bool m_signInRequired = true;
    QString m_autoSignInUser;
    QString m_lastUser;
    int m_autoLockMinutes = 0;

    QList<GuardedAction *> m_guards;
};

#endif // ACCESSCONTROL_H
