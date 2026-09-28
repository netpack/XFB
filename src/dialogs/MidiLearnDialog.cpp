#include "MidiLearnDialog.h"

#include "../services/MidiController.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
constexpr int kActionRole = Qt::UserRole + 1;
} // namespace

MidiLearnDialog::MidiLearnDialog(MidiController *controller, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
{
    setWindowTitle(tr("MIDI Controller"));
    buildUi();
    refreshPorts();
    refreshBindings();

    connect(m_controller, &MidiController::portsChanged, this, &MidiLearnDialog::refreshPorts);
    connect(m_controller, &MidiController::statusChanged, m_status, &QLabel::setText);
    connect(m_controller, &MidiController::messageSeen, this, [this](const QString &d) {
        m_monitor->setText(tr("Last message: %1").arg(d));
    });
    connect(m_controller, &MidiController::learned, this, &MidiLearnDialog::onLearned);
    connect(m_controller, &MidiController::bindingsChanged, this,
            &MidiLearnDialog::refreshBindings);
}

void MidiLearnDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *intro = new QLabel(
        tr("Tie the faders, knobs, buttons and jog wheels of a MIDI controller to XFB. "
           "Choose an action, press Learn, then move or press the control on the "
           "controller that should do it."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *top = new QHBoxLayout;
    m_enabled = new QCheckBox(tr("&Use a MIDI controller"), this);
    m_enabled->setChecked(m_controller->isEnabled());
    top->addWidget(m_enabled);
    top->addStretch(1);
    auto *portLabel = new QLabel(tr("&Input:"), this);
    m_port = new QComboBox(this);
    m_port->setMinimumWidth(240);
    portLabel->setBuddy(m_port);
    top->addWidget(portLabel);
    top->addWidget(m_port);
    layout->addLayout(top);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({ tr("Action"), tr("Control"), tr("Turns as") });
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setAccessibleName(tr("Actions and the controls tied to them"));
    m_tree->setMinimumSize(560, 360);
    layout->addWidget(m_tree, 1);

    m_monitor = new QLabel(tr("Last message: none yet"), this);
    m_monitor->setAccessibleName(tr("Last MIDI message received"));
    layout->addWidget(m_monitor);

    auto *buttons = new QHBoxLayout;
    m_learn = new QPushButton(tr("&Learn"), this);
    m_clear = new QPushButton(tr("C&lear"), this);
    m_clearAll = new QPushButton(tr("Clear &all"), this);
    m_learn->setToolTip(tr("Tie the chosen action to the next control you move"));
    buttons->addWidget(m_learn);
    buttons->addWidget(m_clear);
    buttons->addWidget(m_clearAll);
    buttons->addStretch(1);
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    if (!MidiController::available()) {
        m_enabled->setChecked(false);
        m_enabled->setEnabled(false);
        m_port->setEnabled(false);
        m_learn->setEnabled(false);
        m_status->setText(tr("This build of XFB has no MIDI support. On Linux it needs "
                             "the ALSA libraries at build time."));
    } else {
        m_status->setText(m_controller->status());
    }

    connect(m_enabled, &QCheckBox::toggled, this, [this](bool on) {
        m_controller->setEnabled(on);
        updateButtons();
    });
    connect(m_port, &QComboBox::activated, this, [this](int index) {
        m_controller->setPort(m_port->itemData(index).toString());
    });
    connect(m_learn, &QPushButton::clicked, this, &MidiLearnDialog::learnSelected);
    connect(m_clear, &QPushButton::clicked, this, &MidiLearnDialog::clearSelected);
    connect(m_clearAll, &QPushButton::clicked, this, &MidiLearnDialog::clearAll);
    connect(m_tree, &QTreeWidget::currentItemChanged, this, &MidiLearnDialog::updateButtons);
    connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
        if (item && !item->data(0, kActionRole).toString().isEmpty())
            learnSelected();
    });
    connect(close, &QDialogButtonBox::rejected, this, &MidiLearnDialog::reject);
}

void MidiLearnDialog::refreshPorts()
{
    const QString chosen = m_controller->port();
    m_port->clear();
    m_port->addItem(tr("Every connected controller"), QString());
    for (const QString &name : m_controller->availablePorts())
        m_port->addItem(name, name);
    // A chosen controller that is unplugged stays chosen: it is listed so
    // the choice is visible, and it is used again when it comes back.
    if (!chosen.isEmpty() && m_port->findData(chosen) < 0)
        m_port->addItem(tr("%1 (not connected)").arg(chosen), chosen);
    m_port->setCurrentIndex(qMax(0, m_port->findData(chosen)));
}

void MidiLearnDialog::refreshBindings()
{
    const QString current = selectedAction();
    m_tree->clear();
    QHash<QString, QTreeWidgetItem *> groups;
    QTreeWidgetItem *select = nullptr;

    for (const Midi::Action &action : m_controller->actions()) {
        QTreeWidgetItem *group = groups.value(action.group);
        if (!group) {
            group = new QTreeWidgetItem(m_tree, { action.group });
            group->setFlags(Qt::ItemIsEnabled);
            group->setExpanded(true);
            groups.insert(action.group, group);
        }
        auto *item = new QTreeWidgetItem(group);
        item->setText(0, action.label);
        item->setData(0, kActionRole, action.id);
        const Midi::Binding *b = m_controller->bindingFor(action.id);
        if (m_controller->learningAction() == action.id)
            item->setText(1, tr("Waiting for a control…"));
        else
            item->setText(1, b ? Midi::describeControl(b->kind, b->channel, b->number)
                               : tr("—"));
        if (action.type == Midi::ActionType::Jog && b && b->kind == Midi::Kind::Control)
            installEncodingPicker(item, action.id);
        if (action.id == current)
            select = item;
    }
    if (select)
        m_tree->setCurrentItem(select);
    updateButtons();
}

void MidiLearnDialog::installEncodingPicker(QTreeWidgetItem *item, const QString &actionId)
{
    auto *box = new QComboBox(m_tree);
    box->addItem(tr("Wheel (1 / 127)"), int(Midi::Encoding::TwosComplement));
    box->addItem(tr("Wheel (65 / 63)"), int(Midi::Encoding::Offset64));
    box->addItem(tr("Knob (0 to 127)"), int(Midi::Encoding::Absolute));
    box->setToolTip(tr("How this control reports being turned. Learn guesses it; "
                       "if the wheel goes the wrong way or jumps, try another."));
    box->setAccessibleName(tr("How %1 reports turning").arg(item->text(0)));
    const Midi::Binding *b = m_controller->bindingFor(actionId);
    box->setCurrentIndex(qMax(0, box->findData(int(b ? b->encoding : Midi::Encoding::Absolute))));
    connect(box, &QComboBox::activated, this, [this, box, actionId](int index) {
        const Midi::Binding *current = m_controller->bindingFor(actionId);
        if (!current)
            return;
        Midi::Binding changed = *current;
        changed.encoding = Midi::Encoding(box->itemData(index).toInt());
        m_controller->setBinding(changed);
    });
    m_tree->setItemWidget(item, 2, box);
}

QString MidiLearnDialog::selectedAction() const
{
    const QTreeWidgetItem *item = m_tree ? m_tree->currentItem() : nullptr;
    return item ? item->data(0, kActionRole).toString() : QString();
}

void MidiLearnDialog::updateButtons()
{
    const bool usable = MidiController::available() && m_controller->isEnabled();
    const QString action = selectedAction();
    const bool learning = !m_controller->learningAction().isEmpty();
    m_learn->setText(learning ? tr("&Cancel learning") : tr("&Learn"));
    m_learn->setEnabled(usable && (learning || !action.isEmpty()));
    m_clear->setEnabled(!action.isEmpty() && m_controller->bindingFor(action));
    m_clearAll->setEnabled(!m_controller->bindings().isEmpty());
    m_port->setEnabled(usable);
}

void MidiLearnDialog::learnSelected()
{
    if (!m_controller->learningAction().isEmpty()) {
        m_controller->cancelLearning();
        refreshBindings();
        emit announcementRequested(tr("Learning cancelled."));
        return;
    }
    const QString action = selectedAction();
    if (action.isEmpty() || !m_controller->isEnabled())
        return;
    m_controller->startLearning(action);
    refreshBindings();
    const QTreeWidgetItem *item = m_tree->currentItem();
    const QString message = tr("Move or press the control for %1.")
                                .arg(item ? item->text(0) : action);
    m_status->setText(message);
    emit announcementRequested(message);
}

void MidiLearnDialog::onLearned()
{
    refreshBindings();
    const QTreeWidgetItem *item = m_tree->currentItem();
    const QString message = item ? tr("%1 is now on %2.").arg(item->text(0), item->text(1))
                                 : tr("Control learnt.");
    m_status->setText(message);
    emit announcementRequested(message);
}

void MidiLearnDialog::clearSelected()
{
    const QString action = selectedAction();
    if (!action.isEmpty())
        m_controller->clearBinding(action);
}

void MidiLearnDialog::clearAll()
{
    if (QMessageBox::question(this, tr("Clear all"),
                              tr("Untie every control from its action?"))
            == QMessageBox::Yes)
        m_controller->clearAllBindings();
}

void MidiLearnDialog::reject()
{
    // Escape while learning cancels the learning, not the window.
    if (!m_controller->learningAction().isEmpty()) {
        learnSelected();
        return;
    }
    QDialog::reject();
}
