#include "SignInDialog.h"

#include "../IconTheme.h"
#include "../services/AccessControl.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

SignInDialog::SignInDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , m_mode(mode)
{
    buildUi();
}

void SignInDialog::buildUi()
{
    setWindowTitle(m_mode == Mode::Startup ? tr("Sign in to XFB") : tr("XFB is locked"));
    setWindowIcon(IconTheme::icon(QStringLiteral(":/icons/flat/Security Checked-48.png")));
    setModal(true);

    // No close button: the window is answered or the application is quit.
    // Escape is handled the same way, in keyPressEvent().
    setWindowFlags((windowFlags() | Qt::CustomizeWindowHint)
                   & ~Qt::WindowCloseButtonHint
                   & ~Qt::WindowContextHelpButtonHint);

    auto *layout = new QVBoxLayout(this);

    auto *heading = new QLabel(
        m_mode == Mode::Startup
            ? tr("This station asks who is at the desk.")
            : tr("The station is still on air. Sign in to take the desk back — "
                 "or sign in as somebody else to hand it over."),
        this);
    heading->setWordWrap(true);
    layout->addWidget(heading);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_username = new QLineEdit(this);
    m_username->setAccessibleName(tr("User name"));
    form->addRow(tr("&User name:"), m_username);

    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setAccessibleName(tr("Password"));
    form->addRow(tr("&Password:"), m_password);

    layout->addLayout(form);

    m_message = new QLabel(this);
    m_message->setWordWrap(true);
    m_message->setStyleSheet(QStringLiteral("color: #c0392b;"));
    // A screen reader reads this the moment it changes, which is the only way
    // somebody who cannot see the red text learns the password was refused.
    m_message->setAccessibleName(tr("Sign-in status"));
    layout->addWidget(m_message);

    auto *buttons = new QDialogButtonBox(this);
    m_signIn = buttons->addButton(tr("Sign in"), QDialogButtonBox::AcceptRole);
    m_signIn->setDefault(true);
    QPushButton *quit = buttons->addButton(m_mode == Mode::Startup ? tr("Quit")
                                                                   : tr("Quit XFB"),
                                           QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);

    connect(m_signIn, &QPushButton::clicked, this, &SignInDialog::attempt);
    connect(m_password, &QLineEdit::returnPressed, this, &SignInDialog::attempt);
    connect(m_username, &QLineEdit::returnPressed, m_password,
            qOverload<>(&QWidget::setFocus));
    connect(quit, &QPushButton::clicked, this, [this]() {
        // Quitting from the lock screen takes the station off air, so it is
        // asked about rather than done. Startup has nothing to lose.
        if (m_mode == Mode::Lock) {
            const auto answer = QMessageBox::question(
                this, tr("Quit XFB?"),
                tr("XFB may still be on air. Quitting stops it.\n\nQuit anyway?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
        }
        QDialog::reject();
    });

    // The last person to sign in is nearly always the next one, so the name is
    // filled in and the cursor waits in the password.
    const QString last = AccessControl::instance().lastSignedInUser();
    if (!last.isEmpty()) {
        m_username->setText(last);
        m_password->setFocus();
    } else {
        m_username->setFocus();
    }

    m_cooldown = new QTimer(this);
    m_cooldown->setSingleShot(true);
    connect(m_cooldown, &QTimer::timeout, this, &SignInDialog::allowAnotherAttempt);

    resize(420, sizeHint().height());
}

void SignInDialog::attempt()
{
    if (!m_signIn->isEnabled())
        return;

    QString reason;
    if (AccessControl::instance().signIn(m_username->text().trimmed(),
                                         m_password->text(), &reason)) {
        accept();
        return;
    }

    ++m_failures;
    m_password->clear();
    m_password->setFocus();
    m_message->setText(reason);

    // Three tries free, then a wait that grows. Capped, because the operator
    // who genuinely forgot which of two passwords it was should not be shut
    // out of a station that is about to go silent.
    if (m_failures >= 3) {
        const int seconds = qMin(15, 3 * (m_failures - 2));
        m_signIn->setEnabled(false);
        m_message->setText(tr("%1\n\nWait %n second(s) before trying again.", "",
                              seconds).arg(reason));
        m_cooldown->start(seconds * 1000);
    }
}

void SignInDialog::allowAnotherAttempt()
{
    m_signIn->setEnabled(true);
    m_message->setText(tr("You can try again."));
    m_password->setFocus();
}

void SignInDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->accept(); // there is no "not now" answer to this question
        return;
    }
    QDialog::keyPressEvent(event);
}

bool SignInDialog::ask(Mode mode, QWidget *parent)
{
    SignInDialog dialog(mode, parent);
    return dialog.exec() == QDialog::Accepted;
}
