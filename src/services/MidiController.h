#ifndef MIDICONTROLLER_H
#define MIDICONTROLLER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <memory>
#include <vector>

class QTimer;
class RtMidiIn;

/**
 * The parts of MIDI XFB listens to, kept free of any device so they can be
 * tested without one.
 */
namespace Midi
{
enum class Kind { Note, Control, PitchBend };

/** One channel message, as a controller sends it. */
struct Message
{
    Kind kind = Kind::Control;
    int channel = 0;  ///< 0..15 (shown to people as 1..16)
    int number = 0;   ///< note or controller number; 0 for pitch bend
    int value = 0;    ///< velocity or controller value 0..127; bend 0..16383
    bool noteOff = false; ///< a note-off, or a note-on at velocity 0
};

/**
 * Reads a raw MIDI message. Only note on/off, control change and pitch bend
 * are of use to a desk; everything else (clock, sysex, aftertouch...)
 * returns false.
 */
bool parse(const unsigned char *bytes, size_t size, Message *out);

/** "CC 7, channel 1" / "note C4 (60), channel 10" / "pitch bend, channel 2". */
QString describeControl(Kind kind, int channel, int number);

/** How a knob or jog wheel reports turning. Only jog actions ask. */
enum class Encoding
{
    Absolute,          ///< the position itself, 0..127 (faders, pots)
    TwosComplement,    ///< 1..63 forwards, 127..65 backwards (most jog wheels)
    Offset64,          ///< 65.. forwards, ..63 backwards, 64 = still
};

/** A control on the desk tied to one of XFB's actions. */
struct Binding
{
    QString action;
    Kind kind = Kind::Control;
    int channel = 0;
    int number = 0;
    Encoding encoding = Encoding::Absolute;

    bool matches(const Message &m) const
    {
        return m.kind == kind && m.channel == channel
               && (kind == Kind::PitchBend || m.number == number);
    }
};

/** What an action wants from a control. */
enum class ActionType
{
    Button, ///< fires once per press
    Hold,   ///< told 1 on press and 0 on release (nudge buttons)
    Fader,  ///< a position, 0..1
    Jog,    ///< how far it was turned since last time, in steps (signed)
};

struct Action
{
    QString id;     ///< stable, stored in xfb.conf: "deck1.tempo"
    QString group;  ///< "Deck 1", for the list in the window
    QString label;  ///< "Tempo"
    ActionType type = ActionType::Button;
};

/** Is this message a press (as opposed to a release)? */
bool isPress(const Message &m);
/** A fader position 0..1 from any kind of message. */
double faderValue(const Message &m);
/** Steps turned, for a relative control. Absolute controls are handled by
 *  the controller, which remembers the last position. */
int relativeSteps(Encoding encoding, int value);
/** A guess at a jog wheel's encoding from the first value it sent. */
Encoding guessEncoding(int value);
} // namespace Midi

/**
 * MIDI control of the station: which controls on a desk do what, learnt by
 * example ("MIDI learn") and kept in xfb.conf.
 *
 * Input comes from RtMidi — ALSA on Linux, Windows Multimedia, CoreMIDI —
 * inside this process, so a fader reaches the player in well under a
 * millisecond; nothing goes through the network remote control. What the
 * actions do is not this class's business: the player registers each one
 * with a handler, and messages are handed over on the GUI thread, where
 * everything they touch lives.
 *
 * Off until the operator switches it on. Controllers plugged in later are
 * picked up on their own (the port list is looked at every two seconds).
 */
class MidiController : public QObject
{
    Q_OBJECT

public:
    using Handler = std::function<void(double)>;

    explicit MidiController(QObject *parent = nullptr);
    ~MidiController() override;

    /** False when this build has no MIDI backend (Linux without ALSA). */
    static bool available();

    /** Adds an action and what it does. Button/Hold/Fader handlers get 0..1,
     *  Jog handlers the steps turned. */
    void registerAction(const Midi::Action &action, Handler handler);
    const QVector<Midi::Action> &actions() const { return m_actions; }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    /** Input port to listen to; empty means every one there is. */
    QString port() const { return m_port; }
    void setPort(const QString &port);
    /** Ports there are now, by stable name. */
    QStringList availablePorts() const { return m_portNames; }
    /** How many inputs are open. */
    int openInputCount() const { return int(m_inputs.size()); }
    /** What statusChanged() last said, for a window opened afterwards. */
    QString status() const { return m_status; }

    QVector<Midi::Binding> bindings() const { return m_bindings; }
    /** The control bound to @a action, if any. */
    const Midi::Binding *bindingFor(const QString &action) const;
    void setBinding(const Midi::Binding &binding);
    void clearBinding(const QString &action);
    void clearAllBindings();

    /** The next message that arrives is bound to @a action. */
    void startLearning(const QString &action);
    void cancelLearning();
    QString learningAction() const { return m_learning; }

    /** Handles one message as if it had come from a device. The tests use
     *  it; so does nothing else. */
    void handleMessage(const Midi::Message &message);

    /** A port name without the numbers ALSA hands out afresh each boot. */
    static QString stablePortName(const QString &name);

signals:
    /** Every message received, described, for the window's monitor line. */
    void messageSeen(const QString &description);
    void learned(const Midi::Binding &binding);
    void bindingsChanged();
    void portsChanged(const QStringList &ports);
    /** The inputs were opened or closed, or opening one failed. */
    void statusChanged(const QString &status);

private:
    void load();
    void save() const;
    void reopenInputs();
    void setStatus(const QString &status);
    void pollPorts();
    void openPortWatch();
    void closePortWatch();
    bool openedPortWentAway();
    void onRawMessage(const std::vector<unsigned char> &bytes);
    static void rtMidiCallback(double timeStamp, std::vector<unsigned char> *message,
                               void *userData);

    QVector<Midi::Action> m_actions;
    QHash<QString, Handler> m_handlers;
    QVector<Midi::Binding> m_bindings;
    /** Last position of absolute controls bound to jog actions. */
    QHash<QString, int> m_lastAbsolute;

    bool m_enabled = false;
    QString m_port;
    QString m_learning;
    QString m_status;

    std::unique_ptr<RtMidiIn> m_probe;           ///< lists the ports
    std::vector<std::unique_ptr<RtMidiIn>> m_inputs;
    QStringList m_portNames;
    /** The names as the system gives them, ALSA's "client:port" included:
     *  a controller plugged back in may come back under the same stable
     *  name but new numbers, and the input on the old ones hears nothing. */
    QStringList m_rawPortNames;
    /** ALSA "client:port" of every controller an input is open on. */
    QStringList m_openedAddresses;
    /** ALSA only: a sequencer client subscribed to the system's port
     *  announcements (snd_seq_t *). Catches a controller that went away and
     *  came back between two polls under the very same numbers, which no
     *  port list can tell apart from one that never left. */
    void *m_portWatch = nullptr;
    QTimer *m_pollTimer = nullptr;
};

#endif // MIDICONTROLLER_H
