#include "UsersRolesDialog.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QCoreApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

/** The key a permission row carries, so nothing depends on its label. */
constexpr int kPermissionKeyRole = Qt::UserRole + 1;

/** A tree of every permission XFB has, grouped under its category. */
QTreeWidget *makePermissionTree(QWidget *parent)
{
    auto *tree = new QTreeWidget(parent);
    tree->setColumnCount(1);
    tree->setHeaderHidden(true);
    tree->setUniformRowHeights(false);
    tree->setAccessibleName(QObject::tr("Permissions"));
    return tree;
}

/**
 * Fill the tree.
 *
 * @param checked   what is currently allowed.
 * @param baseline  what the role behind this already allows — used only to
 *                  say so in the row, so an administrator editing one person
 *                  can see which ticks are the role's and which are theirs.
 */
void fillPermissionTree(QTreeWidget *tree, const QSet<QString> &checked, bool editable,
                        const QSet<QString> &baseline = QSet<QString>(),
                        bool showBaseline = false)
{
    tree->clear();

    // The reserved permissions are not shown at all unless this is the one
    // operator they answer to: not greyed, not empty-categoried, absent. The
    // heading is created with its first row, so a category whose every row is
    // skipped never appears either.
    const bool showReserved = AccessControl::instance().mayAdministerReserved();

    QHash<QString, QTreeWidgetItem *> headings;
    for (const AccessControl::Permission &permission : AccessControl::catalogue()) {
        if (!showReserved && AccessControl::isReserved(permission.key))
            continue;

        QTreeWidgetItem *heading = headings.value(permission.category);
        if (!heading) {
            heading = new QTreeWidgetItem(tree, {permission.category});
            QFont bold = heading->font(0);
            bold.setBold(true);
            heading->setFont(0, bold);
            heading->setFlags(Qt::ItemIsEnabled);
            heading->setExpanded(true);
            headings.insert(permission.category, heading);
        }

        QString label = permission.label;
        if (showBaseline && baseline.contains(permission.key))
            label += QObject::tr(" — allowed by the role");

        auto *item = new QTreeWidgetItem(heading, {label});
        item->setData(0, kPermissionKeyRole, permission.key);
        item->setToolTip(0, permission.hint);
        item->setCheckState(0, checked.contains(permission.key) ? Qt::Checked : Qt::Unchecked);
        item->setFlags(editable ? (Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable)
                                : (Qt::ItemIsEnabled | Qt::ItemIsSelectable));
    }

    tree->expandAll();
}

/**
 * What the operator has ticked.
 *
 * @param previous what the tree was filled from. A permission the tree does
 *                 not list — a reserved one, on every desk but one — keeps the
 *                 state it had: saving a role must never quietly withdraw
 *                 something the window was not allowed to show.
 */
QSet<QString> checkedPermissions(const QTreeWidget *tree, const QSet<QString> &previous)
{
    QSet<QString> keys;
    QSet<QString> listed;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        const QTreeWidgetItem *heading = tree->topLevelItem(i);
        for (int j = 0; j < heading->childCount(); ++j) {
            const QTreeWidgetItem *item = heading->child(j);
            const QString key = item->data(0, kPermissionKeyRole).toString();
            listed.insert(key);
            if (item->checkState(0) == Qt::Checked)
                keys.insert(key);
        }
    }
    for (const QString &key : previous) {
        if (!listed.contains(key))
            keys.insert(key);
    }
    return keys;
}

/**
 * One account, edited.
 *
 * Kept here rather than in a file of its own because it has no life outside
 * this window: it is the second half of the Users tab, and it is only a
 * separate window at all because a table cell is the wrong place to tick
 * thirty permissions.
 */
class UserEditor : public QDialog
{
    // Its own translation context, without moc: the class lives in a .cpp and
    // has neither signals nor slots of its own.
    Q_DECLARE_TR_FUNCTIONS(UserEditor)

public:
    UserEditor(const QList<AccessControl::Role> &roles, const AccessControl::User &user,
               bool creating, QWidget *parent)
        : QDialog(parent)
        , m_roles(roles)
        , m_user(user)
        , m_creating(creating)
    {
        setWindowTitle(creating ? tr("New account") : tr("Edit account"));
        setModal(true);

        auto *layout = new QVBoxLayout(this);
        auto *form = new QFormLayout;

        m_username = new QLineEdit(user.username, this);
        m_username->setPlaceholderText(tr("what they type to sign in"));
        form->addRow(tr("&User name:"), m_username);

        m_displayName = new QLineEdit(user.displayName, this);
        m_displayName->setPlaceholderText(tr("as shown in the window title"));
        form->addRow(tr("&Name:"), m_displayName);

        m_role = new QComboBox(this);
        for (const AccessControl::Role &role : m_roles)
            m_role->addItem(role.name, role.id);
        const int index = m_role->findData(user.roleId);
        m_role->setCurrentIndex(index >= 0 ? index : 0);
        form->addRow(tr("&Role:"), m_role);

        if (creating) {
            m_password = new QLineEdit(this);
            m_password->setEchoMode(QLineEdit::Password);
            form->addRow(tr("&Password:"), m_password);

            m_repeat = new QLineEdit(this);
            m_repeat->setEchoMode(QLineEdit::Password);
            form->addRow(tr("Repeat pass&word:"), m_repeat);
        }

        m_enabled = new QCheckBox(tr("This account may sign in"), this);
        m_enabled->setChecked(user.enabled);
        m_enabled->setToolTip(tr("Switching an account off keeps it, its role and its "
                                 "exceptions — for the presenter who is away for a "
                                 "season and back afterwards."));
        form->addRow(QString(), m_enabled);

        layout->addLayout(form);

        auto *exceptions = new QGroupBox(tr("What this person may do"), this);
        auto *exceptionsLayout = new QVBoxLayout(exceptions);
        auto *note = new QLabel(
            tr("The ticks start as the role's. Change one here and it applies to "
               "this person only — and keeps applying even if the role changes "
               "later."), exceptions);
        note->setWordWrap(true);
        exceptionsLayout->addWidget(note);

        m_permissions = makePermissionTree(exceptions);
        exceptionsLayout->addWidget(m_permissions);
        layout->addWidget(exceptions, 1);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &UserEditor::validateAndAccept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        connect(m_role, &QComboBox::currentIndexChanged, this, [this]() { reloadPermissions(); });
        reloadPermissions();

        resize(560, 620);
    }

    AccessControl::User result() const { return m_result; }

private:
    AccessControl::Role currentRole() const
    {
        const QString id = m_role->currentData().toString();
        for (const AccessControl::Role &role : m_roles) {
            if (role.id == id)
                return role;
        }
        return AccessControl::Role();
    }

    QSet<QString> roleBaseline() const
    {
        const AccessControl::Role role = currentRole();
        if (!role.everything)
            return role.permissions;
        QSet<QString> everything;
        for (const AccessControl::Permission &permission : AccessControl::catalogue()) {
            // Same rule as AccessControl::effectivePermissions(): having
            // everything is not having the reserved permissions.
            if (AccessControl::isReserved(permission.key)
                && !AccessControl::isReservedOperator(m_user))
                continue;
            everything.insert(permission.key);
        }
        return everything;
    }

    void reloadPermissions()
    {
        const QSet<QString> baseline = roleBaseline();
        QSet<QString> effective = baseline;
        effective.unite(m_user.granted);
        effective.subtract(m_user.revoked);

        // An administrator's ticks are all of them and cannot be taken away
        // one at a time: an administrator can put them back in seconds, so a
        // half-administrator is a false comfort rather than a safeguard.
        const bool editable = !currentRole().everything;
        fillPermissionTree(m_permissions, effective, editable, baseline, true);

        if (!editable)
            m_permissions->setToolTip(tr("An administrator has every permission, "
                                         "and cannot be given fewer."));
        else
            m_permissions->setToolTip(QString());
    }

    void validateAndAccept()
    {
        const QString username = m_username->text().trimmed();
        if (username.isEmpty()) {
            QMessageBox::warning(this, tr("Nothing to sign in with"),
                                 tr("An account needs a user name."));
            return;
        }
        if (m_creating) {
            if (m_password->text().isEmpty()) {
                QMessageBox::warning(this, tr("Nothing to sign in with"),
                                     tr("An account needs a password."));
                return;
            }
            if (m_password->text() != m_repeat->text()) {
                QMessageBox::warning(this, tr("The passwords differ"),
                                     tr("The two passwords are not the same. Type "
                                        "them again."));
                return;
            }
        }

        m_result = m_user;
        m_result.username = username;
        m_result.displayName = m_displayName->text().trimmed().isEmpty()
                                   ? username : m_displayName->text().trimmed();
        m_result.roleId = m_role->currentData().toString();
        m_result.enabled = m_enabled->isChecked();
        if (m_creating)
            m_result.secret = AccessControl::hashPassword(m_password->text());

        // Store only what differs from the role, so an exception disappears on
        // its own the day the role is changed to agree with it.
        const QSet<QString> baseline = roleBaseline();
        QSet<QString> effective = roleBaseline();
        effective.unite(m_user.granted);
        effective.subtract(m_user.revoked);
        const QSet<QString> wanted = checkedPermissions(m_permissions, effective);
        m_result.granted = wanted;
        m_result.granted.subtract(baseline);
        m_result.revoked = baseline;
        m_result.revoked.subtract(wanted);

        accept();
    }

    QList<AccessControl::Role> m_roles;
    AccessControl::User m_user;
    AccessControl::User m_result;
    bool m_creating;

    QLineEdit *m_username = nullptr;
    QLineEdit *m_displayName = nullptr;
    QComboBox *m_role = nullptr;
    QLineEdit *m_password = nullptr;
    QLineEdit *m_repeat = nullptr;
    QCheckBox *m_enabled = nullptr;
    QTreeWidget *m_permissions = nullptr;
};

} // namespace

// ---------------------------------------------------------------------------

UsersRolesDialog::UsersRolesDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUi();
    showAppropriatePage();
}

void UsersRolesDialog::buildUi()
{
    setWindowTitle(tr("Users and roles"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/flat/Security Checked-48.png")));

    auto *layout = new QVBoxLayout(this);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(buildInvitation());
    m_pages->addWidget(buildTabs());
    layout->addWidget(m_pages, 1);

    auto *buttons = new QDialogButtonBox(this);
    QPushButton *save = buttons->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    QPushButton *close = buttons->addButton(tr("Close"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);

    connect(save, &QPushButton::clicked, this, &UsersRolesDialog::apply);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);

    // The invitation page has its own button; Save has nothing to write until
    // there is something to write it to.
    connect(m_pages, &QStackedWidget::currentChanged, save, [save](int page) {
        save->setVisible(page == 1);
    });
    save->setVisible(false);

    resize(760, 640);
}

QWidget *UsersRolesDialog::buildInvitation()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *heading = new QLabel(tr("<h3>Nobody has to sign in to this XFB</h3>"), page);
    layout->addWidget(heading);

    auto *explanation = new QLabel(
        tr("Anybody who can open XFB on this machine can do everything it does: "
           "empty the library, re-encode the catalogue overnight, change the "
           "stream credentials, hand out a pairing token.\n\n"
           "Creating the first account turns that off. From then on XFB asks who "
           "is at the desk, and what each person may do is decided by the role "
           "you give them — Producer, Presenter, Guest, or one of your own.\n\n"
           "The first account is an administrator, because somebody has to be "
           "able to create the second one."), page);
    explanation->setWordWrap(true);
    layout->addWidget(explanation);

    auto *form = new QFormLayout;
    m_firstUser = new QLineEdit(page);
    m_firstUser->setPlaceholderText(tr("what you will type to sign in"));
    form->addRow(tr("&User name:"), m_firstUser);

    m_firstName = new QLineEdit(page);
    m_firstName->setPlaceholderText(tr("your name, as the window title shows it"));
    form->addRow(tr("&Name:"), m_firstName);

    m_firstPassword = new QLineEdit(page);
    m_firstPassword->setEchoMode(QLineEdit::Password);
    form->addRow(tr("&Password:"), m_firstPassword);

    m_firstRepeat = new QLineEdit(page);
    m_firstRepeat->setEchoMode(QLineEdit::Password);
    form->addRow(tr("&Repeat password:"), m_firstRepeat);
    layout->addLayout(form);

    auto *warning = new QLabel(
        tr("Write this password down somewhere that is not this computer. XFB "
           "cannot recover it: the only way back into a station whose "
           "administrator password is lost is to delete the accounts file, "
           "which needs the operator's login on this machine."), page);
    warning->setWordWrap(true);
    warning->setStyleSheet(QStringLiteral("color: #b9770e;"));
    layout->addWidget(warning);

    auto *protectButton = new QPushButton(tr("Protect this installation"), page);
    connect(protectButton, &QPushButton::clicked, this, &UsersRolesDialog::protectInstallation);
    layout->addWidget(protectButton, 0, Qt::AlignLeft);

    layout->addStretch(1);
    return page;
}

QWidget *UsersRolesDialog::buildTabs()
{
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildUsersTab(), tr("Users"));
    m_tabs->addTab(buildRolesTab(), tr("Roles"));
    m_tabs->addTab(buildProtectionTab(), tr("Protection"));
    return m_tabs;
}

QWidget *UsersRolesDialog::buildUsersTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_userTable = new QTableWidget(0, 5, page);
    m_userTable->setHorizontalHeaderLabels({tr("User name"), tr("Name"), tr("Role"),
                                            tr("Signs in"), tr("Last signed in")});
    m_userTable->horizontalHeader()->setStretchLastSection(true);
    m_userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_userTable->verticalHeader()->setVisible(false);
    m_userTable->setAccessibleName(tr("Accounts"));
    layout->addWidget(m_userTable, 1);

    auto *row = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add..."), page);
    m_editUserButton = new QPushButton(tr("Edit..."), page);
    m_passwordButton = new QPushButton(tr("Change password..."), page);
    m_removeUserButton = new QPushButton(tr("Remove"), page);
    row->addWidget(add);
    row->addWidget(m_editUserButton);
    row->addWidget(m_passwordButton);
    row->addWidget(m_removeUserButton);
    row->addStretch(1);
    layout->addLayout(row);

    connect(add, &QPushButton::clicked, this, &UsersRolesDialog::addUser);
    connect(m_editUserButton, &QPushButton::clicked, this, &UsersRolesDialog::editUser);
    connect(m_passwordButton, &QPushButton::clicked, this, &UsersRolesDialog::changePassword);
    connect(m_removeUserButton, &QPushButton::clicked, this, &UsersRolesDialog::removeUser);
    connect(m_userTable, &QTableWidget::itemDoubleClicked, this, &UsersRolesDialog::editUser);
    connect(m_userTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        const bool chosen = selectedUserIndex() >= 0;
        m_editUserButton->setEnabled(chosen);
        m_passwordButton->setEnabled(chosen);
        m_removeUserButton->setEnabled(chosen);
    });

    m_editUserButton->setEnabled(false);
    m_passwordButton->setEnabled(false);
    m_removeUserButton->setEnabled(false);
    return page;
}

QWidget *UsersRolesDialog::buildRolesTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);

    auto *left = new QVBoxLayout;
    m_roleList = new QListWidget(page);
    m_roleList->setAccessibleName(tr("Roles"));
    left->addWidget(m_roleList, 1);

    auto *buttons = new QHBoxLayout;
    auto *add = new QPushButton(tr("New"), page);
    auto *copy = new QPushButton(tr("Duplicate"), page);
    m_removeRoleButton = new QPushButton(tr("Delete"), page);
    buttons->addWidget(add);
    buttons->addWidget(copy);
    buttons->addWidget(m_removeRoleButton);
    left->addLayout(buttons);
    layout->addLayout(left, 1);

    auto *right = new QVBoxLayout;
    auto *form = new QFormLayout;
    m_roleName = new QLineEdit(page);
    form->addRow(tr("&Name:"), m_roleName);
    m_roleDescription = new QPlainTextEdit(page);
    m_roleDescription->setMaximumHeight(70);
    form->addRow(tr("&Description:"), m_roleDescription);
    right->addLayout(form);

    m_roleNotice = new QLabel(page);
    m_roleNotice->setWordWrap(true);
    m_roleNotice->setStyleSheet(QStringLiteral("color: #b9770e;"));
    right->addWidget(m_roleNotice);

    m_permissions = makePermissionTree(page);
    right->addWidget(m_permissions, 1);
    layout->addLayout(right, 2);

    connect(add, &QPushButton::clicked, this, &UsersRolesDialog::addRole);
    connect(copy, &QPushButton::clicked, this, &UsersRolesDialog::duplicateRole);
    connect(m_removeRoleButton, &QPushButton::clicked, this, &UsersRolesDialog::removeRole);
    connect(m_roleList, &QListWidget::currentRowChanged, this, &UsersRolesDialog::roleSelected);
    connect(m_permissions, &QTreeWidget::itemChanged, this, &UsersRolesDialog::permissionToggled);

    // Typing in the name or the description edits the selected role straight
    // away, so there is no second Apply hiding inside the tab.
    connect(m_roleName, &QLineEdit::textEdited, this, [this](const QString &text) {
        const int index = selectedRoleIndex();
        if (index < 0 || m_loading)
            return;
        m_roles[index].name = text;
        if (auto *item = m_roleList->item(index))
            item->setText(text);
    });
    connect(m_roleDescription, &QPlainTextEdit::textChanged, this, [this]() {
        const int index = selectedRoleIndex();
        if (index < 0 || m_loading)
            return;
        m_roles[index].description = m_roleDescription->toPlainText();
    });

    return page;
}

QWidget *UsersRolesDialog::buildProtectionTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_askAtStartup = new QCheckBox(tr("Ask who is at the desk when XFB starts"), page);
    m_askAtStartup->setToolTip(tr("Off means XFB comes up on its own after a power "
                                  "cut — as the account chosen below, with that "
                                  "account's permissions."));
    layout->addWidget(m_askAtStartup);

    auto *form = new QFormLayout;
    m_autoUser = new QComboBox(page);
    form->addRow(tr("&Start as:"), m_autoUser);

    m_autoLock = new QSpinBox(page);
    m_autoLock->setRange(0, 240);
    m_autoLock->setSuffix(tr(" minutes"));
    m_autoLock->setSpecialValueText(tr("never"));
    m_autoLock->setToolTip(tr("Lock the screen when nobody has touched the desk for "
                              "this long. The station stays on air behind the lock. "
                              "Off by default: a machine that locks itself mid-show "
                              "is the worse fault."));
    form->addRow(tr("&Lock the desk after:"), m_autoLock);
    layout->addLayout(form);

    connect(m_askAtStartup, &QCheckBox::toggled, m_autoUser, &QWidget::setDisabled);

    auto *honesty = new QGroupBox(tr("What this protects, and what it does not"), page);
    auto *honestyLayout = new QVBoxLayout(honesty);
    auto *text = new QLabel(
        tr("This keeps the wrong menu from being opened on a live station. It is "
           "not a defence against somebody who owns the computer: the accounts "
           "live in a file readable only by the operator XFB runs as, and "
           "deleting that file returns this installation to having no accounts "
           "at all. Passwords themselves are not in it — only digests, so a copy "
           "of the file does not hand over a password that may be in use "
           "somewhere else."), honesty);
    text->setWordWrap(true);
    honestyLayout->addWidget(text);

    m_storageNote = new QLabel(honesty);
    m_storageNote->setWordWrap(true);
    m_storageNote->setTextInteractionFlags(Qt::TextSelectableByMouse);
    honestyLayout->addWidget(m_storageNote);

    auto *off = new QPushButton(tr("Turn the protection off..."), honesty);
    connect(off, &QPushButton::clicked, this, &UsersRolesDialog::turnProtectionOff);
    honestyLayout->addWidget(off, 0, Qt::AlignLeft);

    layout->addWidget(honesty);
    layout->addStretch(1);
    return page;
}

// ---------------------------------------------------------------------------
// Filling the windows in
// ---------------------------------------------------------------------------

void UsersRolesDialog::showAppropriatePage()
{
    AccessControl &control = AccessControl::instance();
    if (!control.isProtected()) {
        m_pages->setCurrentIndex(0);
        m_firstUser->setFocus();
        return;
    }

    m_roles = control.roles();
    m_users = control.users();
    m_pages->setCurrentIndex(1);
    reloadUsers();
    reloadRoles();
    reloadProtection();
}

void UsersRolesDialog::reloadUsers()
{
    m_userTable->setRowCount(m_users.size());
    for (int row = 0; row < m_users.size(); ++row) {
        const AccessControl::User &user = m_users.at(row);

        QString roleName = tr("(no role)");
        for (const AccessControl::Role &role : m_roles) {
            if (role.id == user.roleId) {
                roleName = role.name;
                break;
            }
        }

        const int exceptions = user.granted.size() + user.revoked.size();
        if (exceptions > 0)
            roleName += tr(" (%n exception(s))", "", exceptions);

        const QString lastSeen = user.lastSignIn.isValid()
                                     ? user.lastSignIn.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                     : tr("never");

        const QStringList cells{user.username, user.displayName, roleName,
                                user.enabled ? tr("yes") : tr("no"), lastSeen};
        for (int column = 0; column < cells.size(); ++column) {
            auto *item = new QTableWidgetItem(cells.at(column));
            if (!user.enabled)
                item->setForeground(QBrush(QColor(0x80, 0x80, 0x80)));
            m_userTable->setItem(row, column, item);
        }
    }
    m_userTable->resizeColumnsToContents();
    refreshAutoSignInChoices();
}

void UsersRolesDialog::reloadRoles()
{
    m_loading = true;
    const int previous = m_roleList->currentRow();
    m_roleList->clear();
    for (const AccessControl::Role &role : m_roles)
        m_roleList->addItem(role.name);
    m_loading = false;

    if (!m_roles.isEmpty())
        m_roleList->setCurrentRow(qBound(0, previous < 0 ? 0 : previous, m_roles.size() - 1));
    else
        roleSelected();
}

void UsersRolesDialog::reloadProtection()
{
    AccessControl &control = AccessControl::instance();
    m_askAtStartup->setChecked(control.signInRequired());
    m_autoLock->setValue(control.autoLockMinutes());
    m_autoUser->setDisabled(m_askAtStartup->isChecked());
    m_storageNote->setText(tr("The accounts are kept in %1").arg(AccessControl::storagePath()));
    refreshAutoSignInChoices();
}

void UsersRolesDialog::refreshAutoSignInChoices()
{
    if (!m_autoUser)
        return;
    const QString wanted = m_autoUser->count() > 0
                               ? m_autoUser->currentData().toString()
                               : AccessControl::instance().autoSignInUser();
    m_autoUser->clear();
    m_autoUser->addItem(tr("the account that can do least"), QString());
    for (const AccessControl::User &user : m_users) {
        if (user.enabled)
            m_autoUser->addItem(user.displayName.isEmpty() ? user.username : user.displayName,
                                user.username);
    }
    const int index = m_autoUser->findData(wanted);
    m_autoUser->setCurrentIndex(index >= 0 ? index : 0);
}

int UsersRolesDialog::selectedRoleIndex() const
{
    const int row = m_roleList->currentRow();
    return (row >= 0 && row < m_roles.size()) ? row : -1;
}

int UsersRolesDialog::selectedUserIndex() const
{
    const int row = m_userTable->currentRow();
    if (row < 0 || row >= m_users.size())
        return -1;
    return m_userTable->selectionModel() && m_userTable->selectionModel()->hasSelection() ? row : -1;
}

bool UsersRolesDialog::isProtectedRole(const AccessControl::Role &role)
{
    // Administrator is the one role whose meaning XFB relies on.
    return role.everything;
}

// ---------------------------------------------------------------------------
// Roles
// ---------------------------------------------------------------------------

void UsersRolesDialog::roleSelected()
{
    const int index = selectedRoleIndex();

    m_loading = true;
    if (index < 0) {
        m_roleName->clear();
        m_roleDescription->clear();
        m_permissions->clear();
        m_roleNotice->clear();
        m_roleName->setEnabled(false);
        m_roleDescription->setEnabled(false);
        m_removeRoleButton->setEnabled(false);
        m_loading = false;
        return;
    }

    const AccessControl::Role &role = m_roles.at(index);
    const bool locked = isProtectedRole(role);

    m_roleName->setText(role.name);
    m_roleDescription->setPlainText(role.description);
    m_roleName->setEnabled(!locked);
    m_roleDescription->setEnabled(!locked);
    m_removeRoleButton->setEnabled(!locked && !role.builtin);

    if (locked) {
        m_roleNotice->setText(tr("The Administrator role has every permission there "
                                 "is, including the ones a later version of XFB "
                                 "adds. It cannot be edited or deleted."));
    } else if (role.builtin) {
        m_roleNotice->setText(tr("One of the roles XFB ships with. You can change "
                                 "what it may do; it cannot be deleted, so there is "
                                 "always somewhere to put a new account."));
    } else {
        m_roleNotice->clear();
    }

    QSet<QString> permissions = role.permissions;
    if (role.everything) {
        for (const AccessControl::Permission &permission : AccessControl::catalogue()) {
            // Not part of what "everything" means — see AccessControl. The one
            // operator they answer to does have them through it, and is also
            // the only one this tree shows them to.
            if (AccessControl::isReserved(permission.key)
                && !AccessControl::instance().mayAdministerReserved())
                continue;
            permissions.insert(permission.key);
        }
    }
    fillPermissionTree(m_permissions, permissions, !locked);
    m_loading = false;
}

void UsersRolesDialog::permissionToggled()
{
    const int index = selectedRoleIndex();
    if (index < 0 || m_loading)
        return;
    m_roles[index].permissions = checkedPermissions(m_permissions,
                                                    m_roles[index].permissions);
}

void UsersRolesDialog::addRole()
{
    AccessControl::Role role;
    // The id is what users reference, so it never changes once it is written;
    // the name above it is free to.
    role.id = QStringLiteral("role%1").arg(QDateTime::currentMSecsSinceEpoch());
    role.name = tr("New role");
    m_roles.append(role);
    reloadRoles();
    m_roleList->setCurrentRow(m_roles.size() - 1);
    m_roleName->setFocus();
    m_roleName->selectAll();
}

void UsersRolesDialog::duplicateRole()
{
    const int index = selectedRoleIndex();
    if (index < 0)
        return;

    AccessControl::Role copy = m_roles.at(index);
    copy.id = QStringLiteral("role%1").arg(QDateTime::currentMSecsSinceEpoch());
    copy.name = tr("%1 (copy)").arg(copy.name);
    copy.builtin = false;
    if (copy.everything) {
        // A copy of Administrator is a role with everything ticked today,
        // rather than a second role that silently gains whatever XFB adds next.
        copy.everything = false;
        for (const AccessControl::Permission &permission : AccessControl::catalogue())
            copy.permissions.insert(permission.key);
    }
    m_roles.append(copy);
    reloadRoles();
    m_roleList->setCurrentRow(m_roles.size() - 1);
    m_roleName->setFocus();
    m_roleName->selectAll();
}

void UsersRolesDialog::removeRole()
{
    const int index = selectedRoleIndex();
    if (index < 0)
        return;

    const AccessControl::Role role = m_roles.at(index);
    if (isProtectedRole(role) || role.builtin)
        return;

    QStringList holders;
    for (const AccessControl::User &user : m_users) {
        if (user.roleId == role.id)
            holders.append(user.username);
    }
    if (!holders.isEmpty()) {
        QMessageBox::information(
            this, tr("The role is in use"),
            tr("%1 still holds the %2 role. Give them another role first.")
                .arg(holders.join(QStringLiteral(", ")), role.name));
        return;
    }

    m_roles.removeAt(index);
    reloadRoles();
}

// ---------------------------------------------------------------------------
// Users
// ---------------------------------------------------------------------------

void UsersRolesDialog::addUser()
{
    AccessControl::User fresh;
    // Whatever is least privileged is the safe thing to land on.
    fresh.roleId = m_roles.isEmpty() ? QString() : m_roles.last().id;

    UserEditor editor(m_roles, fresh, true, this);
    if (editor.exec() != QDialog::Accepted)
        return;

    const AccessControl::User created = editor.result();
    for (const AccessControl::User &existing : m_users) {
        if (existing.username.compare(created.username, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, tr("That name is taken"),
                                 tr("There is already an account called %1.")
                                     .arg(created.username));
            return;
        }
    }

    m_users.append(created);
    reloadUsers();
    emit announcementRequested(tr("Account %1 added.").arg(created.username));
}

void UsersRolesDialog::editUser()
{
    const int index = selectedUserIndex();
    if (index < 0)
        return;

    UserEditor editor(m_roles, m_users.at(index), false, this);
    if (editor.exec() != QDialog::Accepted)
        return;

    m_users[index] = editor.result();
    reloadUsers();
}

void UsersRolesDialog::changePassword()
{
    const int index = selectedUserIndex();
    if (index < 0)
        return;

    // An administrator setting somebody else's password is not asked for the
    // old one — they are already trusted with deleting the account outright.
    QDialog ask(this);
    ask.setWindowTitle(tr("Password for %1").arg(m_users.at(index).username));
    auto *layout = new QVBoxLayout(&ask);
    auto *form = new QFormLayout;
    auto *first = new QLineEdit(&ask);
    first->setEchoMode(QLineEdit::Password);
    auto *again = new QLineEdit(&ask);
    again->setEchoMode(QLineEdit::Password);
    form->addRow(tr("New &password:"), first);
    form->addRow(tr("&Repeat it:"), again);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &ask);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &ask, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &ask, &QDialog::reject);

    if (ask.exec() != QDialog::Accepted)
        return;
    if (first->text().isEmpty()) {
        QMessageBox::warning(this, tr("Nothing to sign in with"),
                             tr("An account needs a password."));
        return;
    }
    if (first->text() != again->text()) {
        QMessageBox::warning(this, tr("The passwords differ"),
                             tr("The two passwords are not the same. Type them again."));
        return;
    }

    m_users[index].secret = AccessControl::hashPassword(first->text());
    emit announcementRequested(tr("Password changed. Save to keep it."));
}

void UsersRolesDialog::removeUser()
{
    const int index = selectedUserIndex();
    if (index < 0)
        return;

    const AccessControl::User user = m_users.at(index);
    const auto answer = QMessageBox::question(
        this, tr("Remove %1?").arg(user.username),
        tr("The account and its exceptions go. If this person is only away for a "
           "while, switch the account off in Edit instead — it keeps everything.\n\n"
           "Remove it?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    m_users.removeAt(index);
    reloadUsers();
}

// ---------------------------------------------------------------------------
// Writing it back
// ---------------------------------------------------------------------------

void UsersRolesDialog::protectInstallation()
{
    if (m_firstPassword->text() != m_firstRepeat->text()) {
        QMessageBox::warning(this, tr("The passwords differ"),
                             tr("The two passwords are not the same. Type them again."));
        return;
    }

    QString reason;
    if (!AccessControl::instance().createInitialAdministrator(
            m_firstUser->text(), m_firstName->text(), m_firstPassword->text(), &reason)) {
        QMessageBox::warning(this, tr("Nothing was changed"), reason);
        return;
    }

    m_firstPassword->clear();
    m_firstRepeat->clear();
    showAppropriatePage();
    emit announcementRequested(tr("This installation is protected. You are signed in "
                                  "as its administrator."));
    QMessageBox::information(
        this, tr("Protected"),
        tr("XFB will ask who is at the desk from now on, and you are signed in as "
           "its administrator.\n\nAdd the rest of the station on the Users tab."));
}

void UsersRolesDialog::apply()
{
    QString reason;
    if (!AccessControl::instance().save(m_roles, m_users, &reason)) {
        QMessageBox::warning(this, tr("Nothing was saved"), reason);
        return;
    }

    AccessControl::instance().setSignInRequired(m_askAtStartup->isChecked());
    AccessControl::instance().setAutoSignInUser(m_autoUser->currentData().toString());
    AccessControl::instance().setAutoLockMinutes(m_autoLock->value());

    emit announcementRequested(tr("Users and roles saved."));
    accept();
}

void UsersRolesDialog::turnProtectionOff()
{
    const auto answer = QMessageBox::question(
        this, tr("Turn the protection off?"),
        tr("Every account and every role goes, and XFB returns to what it was "
           "before: anybody who can open it can do everything it does.\n\n"
           "Turn it off?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    QString reason;
    if (!AccessControl::instance().removeProtection(&reason)) {
        QMessageBox::warning(this, tr("Nothing was changed"), reason);
        return;
    }

    m_roles.clear();
    m_users.clear();
    emit announcementRequested(tr("The protection is off. Nobody has to sign in."));
    showAppropriatePage();
}
