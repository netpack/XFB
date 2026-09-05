#ifndef USERSROLESDIALOG_H
#define USERSROLESDIALOG_H

#include <QDialog>
#include <QList>

#include "../services/AccessControl.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTabWidget;
class QTableWidget;
class QTreeWidget;

/**
 * @brief Where an administrator decides who may do what.
 *
 * Three tabs and one door in front of them.
 *
 * The door is what an unprotected installation sees: a page explaining what
 * turning protection on means and asking for the first administrator. Until
 * somebody walks through it XFB behaves exactly as it always has, which is
 * the point — an existing station that updates into this version is not
 * suddenly asking a presenter for a password nobody has been given.
 *
 *  - **Users** — the people. One role each, switched on or off without being
 *    deleted (the cover presenter who is back next month), and exceptions for
 *    the one person who is a presenter *and* runs the advert import.
 *  - **Roles** — the sets. Every permission XFB has, grouped the way the
 *    menus are, ticked or not. Administrator cannot be edited: a role that
 *    can be talked out of its own rights is not a safeguard.
 *  - **Protection** — whether XFB asks at startup, which account it comes up
 *    as when it does not, and how long the desk may sit idle before it locks.
 *
 * Nothing here claims to be more than it is, and the tab says so: the accounts
 * live in a file beside xfb.conf and anybody with the operator's login and a
 * shell can delete it. This keeps the wrong menu from being opened on a live
 * station. It does not keep out somebody who owns the computer.
 *
 * Built in code rather than from a .ui file: the committed ui_*.h headers in
 * src/ shadow the generated ones, and a stale one is a silent failure.
 */
class UsersRolesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UsersRolesDialog(QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear what happened. */
    void announcementRequested(const QString &message);

private slots:
    void protectInstallation();
    void addUser();
    void editUser();
    void removeUser();
    void changePassword();
    void addRole();
    void duplicateRole();
    void removeRole();
    void roleSelected();
    void permissionToggled();
    void apply();
    void turnProtectionOff();

private:
    void buildUi();
    QWidget *buildInvitation();
    QWidget *buildTabs();
    QWidget *buildUsersTab();
    QWidget *buildRolesTab();
    QWidget *buildProtectionTab();

    /** Swap between the invitation and the tabs, and fill whichever shows. */
    void showAppropriatePage();
    void reloadUsers();
    void reloadRoles();
    void reloadProtection();
    void refreshAutoSignInChoices();
    /** The role currently selected on the Roles tab, or -1. */
    int selectedRoleIndex() const;
    int selectedUserIndex() const;
    /** True when this role may not be edited or deleted. */
    static bool isProtectedRole(const AccessControl::Role &role);

    // The edit is done on copies and written back in one go by apply(), so
    // Close leaves an installation exactly as it was found.
    QList<AccessControl::Role> m_roles;
    QList<AccessControl::User> m_users;

    QStackedWidget *m_pages = nullptr;
    QTabWidget *m_tabs = nullptr;

    // Invitation
    QLineEdit *m_firstUser = nullptr;
    QLineEdit *m_firstName = nullptr;
    QLineEdit *m_firstPassword = nullptr;
    QLineEdit *m_firstRepeat = nullptr;

    // Users
    QTableWidget *m_userTable = nullptr;
    QPushButton *m_editUserButton = nullptr;
    QPushButton *m_removeUserButton = nullptr;
    QPushButton *m_passwordButton = nullptr;

    // Roles
    QListWidget *m_roleList = nullptr;
    QLineEdit *m_roleName = nullptr;
    QPlainTextEdit *m_roleDescription = nullptr;
    QTreeWidget *m_permissions = nullptr;
    QLabel *m_roleNotice = nullptr;
    QPushButton *m_removeRoleButton = nullptr;

    // Protection
    QCheckBox *m_askAtStartup = nullptr;
    QComboBox *m_autoUser = nullptr;
    QSpinBox *m_autoLock = nullptr;
    QLabel *m_storageNote = nullptr;

    bool m_loading = false;
};

#endif // USERSROLESDIALOG_H
