#ifndef MIDILEARNDIALOG_H
#define MIDILEARNDIALOG_H

#include <QDialog>

class MidiController;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief Tying the controls of a MIDI desk to what XFB does.
 *
 * Every action XFB offers is listed, grouped (the station, each deck, the
 * pads...), with the control it is tied to. Pick one, press Learn, and move
 * the fader or press the button on the desk: that control now does it. The
 * monitor line under the list shows every message the desk sends, which is
 * how an operator finds out whether it is talking to XFB at all.
 *
 * Built in code rather than from a .ui file, like the other windows added
 * since the committed ui_*.h headers started shadowing generated ones.
 */
class MidiLearnDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MidiLearnDialog(MidiController *controller, QWidget *parent = nullptr);

signals:
    /** Routed to the player so screen readers hear what happened. */
    void announcementRequested(const QString &message);

protected:
    void reject() override;

private:
    void buildUi();
    void refreshPorts();
    void refreshBindings();
    void learnSelected();
    void clearSelected();
    void clearAll();
    void onLearned();
    void updateButtons();
    QString selectedAction() const;
    /** The encoding picker for jog rows, in the third column. */
    void installEncodingPicker(QTreeWidgetItem *item, const QString &actionId);

    MidiController *m_controller = nullptr;
    QCheckBox *m_enabled = nullptr;
    QComboBox *m_port = nullptr;
    QLabel *m_status = nullptr;
    QTreeWidget *m_tree = nullptr;
    QLabel *m_monitor = nullptr;
    QPushButton *m_learn = nullptr;
    QPushButton *m_clear = nullptr;
    QPushButton *m_clearAll = nullptr;
};

#endif // MIDILEARNDIALOG_H
