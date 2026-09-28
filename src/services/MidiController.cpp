#include "MidiController.h"

#include <QDebug>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include "RtMidi.h"

#ifdef __LINUX_ALSA__
#include <alsa/asoundlib.h>
#include <cerrno>
#endif

namespace
{
const QString kGroup = QStringLiteral("Midi");

QString configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

QString kindName(Midi::Kind kind)
{
    switch (kind) {
    case Midi::Kind::Note: return QStringLiteral("note");
    case Midi::Kind::Control: return QStringLiteral("cc");
    case Midi::Kind::PitchBend: return QStringLiteral("bend");
    }
    return QString();
}

Midi::Kind kindFromName(const QString &name, bool *ok)
{
    *ok = true;
    if (name == QLatin1String("note"))
        return Midi::Kind::Note;
    if (name == QLatin1String("bend"))
        return Midi::Kind::PitchBend;
    *ok = (name == QLatin1String("cc"));
    return Midi::Kind::Control;
}

QString encodingName(Midi::Encoding e)
{
    switch (e) {
    case Midi::Encoding::Absolute: return QStringLiteral("absolute");
    case Midi::Encoding::TwosComplement: return QStringLiteral("twos-complement");
    case Midi::Encoding::Offset64: return QStringLiteral("offset-64");
    }
    return QString();
}

Midi::Encoding encodingFromName(const QString &name)
{
    if (name == QLatin1String("twos-complement"))
        return Midi::Encoding::TwosComplement;
    if (name == QLatin1String("offset-64"))
        return Midi::Encoding::Offset64;
    return Midi::Encoding::Absolute;
}
} // namespace

// ---------------------------------------------------------------- Midi::

bool Midi::parse(const unsigned char *bytes, size_t size, Message *out)
{
    if (!bytes || size < 3 || !out)
        return false;
    const int status = bytes[0] & 0xF0;
    const int channel = bytes[0] & 0x0F;
    const int d1 = bytes[1] & 0x7F;
    const int d2 = bytes[2] & 0x7F;

    Message m;
    m.channel = channel;
    switch (status) {
    case 0x80: // note off
        m.kind = Kind::Note;
        m.number = d1;
        m.value = d2;
        m.noteOff = true;
        break;
    case 0x90: // note on; velocity 0 is a note off by another name
        m.kind = Kind::Note;
        m.number = d1;
        m.value = d2;
        m.noteOff = (d2 == 0);
        break;
    case 0xB0:
        m.kind = Kind::Control;
        m.number = d1;
        m.value = d2;
        break;
    case 0xE0: // 14-bit, least significant byte first
        m.kind = Kind::PitchBend;
        m.number = 0;
        m.value = d1 | (d2 << 7);
        break;
    default:
        return false;
    }
    *out = m;
    return true;
}

QString Midi::describeControl(Kind kind, int channel, int number)
{
    const int shownChannel = channel + 1;
    switch (kind) {
    case Kind::Note: {
        static const char *names[] = { "C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B" };
        return QObject::tr("note %1%2 (%3), channel %4")
            .arg(QLatin1String(names[number % 12]))
            .arg(number / 12 - 1)
            .arg(number)
            .arg(shownChannel);
    }
    case Kind::Control:
        return QObject::tr("CC %1, channel %2").arg(number).arg(shownChannel);
    case Kind::PitchBend:
        return QObject::tr("pitch bend, channel %1").arg(shownChannel);
    }
    return QString();
}

bool Midi::isPress(const Message &m)
{
    switch (m.kind) {
    case Kind::Note: return !m.noteOff;
    case Kind::Control: return m.value >= 64;
    case Kind::PitchBend: return m.value >= 8192;
    }
    return false;
}

double Midi::faderValue(const Message &m)
{
    if (m.kind == Kind::PitchBend)
        return m.value / 16383.0;
    if (m.kind == Kind::Note && m.noteOff)
        return 0.0;
    return m.value / 127.0;
}

int Midi::relativeSteps(Encoding encoding, int value)
{
    switch (encoding) {
    case Encoding::TwosComplement:
        return value < 64 ? value : value - 128;
    case Encoding::Offset64:
        return value - 64;
    case Encoding::Absolute:
        break;
    }
    return 0;
}

Midi::Encoding Midi::guessEncoding(int value)
{
    // A wheel's first message is a small movement: near 0 either way in
    // two's complement (1, 2 ... or 127, 126 ...), near 64 in offset form.
    // A value from the middle of either is most likely a plain knob.
    if ((value >= 1 && value <= 15) || (value >= 113 && value <= 127))
        return Encoding::TwosComplement;
    if (value >= 49 && value <= 79 && value != 64)
        return Encoding::Offset64;
    return Encoding::Absolute;
}

// --------------------------------------------------------- MidiController

MidiController::MidiController(QObject *parent)
    : QObject(parent)
{
    load();

    if (available()) {
        try {
            m_probe = std::make_unique<RtMidiIn>(RtMidi::UNSPECIFIED, "XFB");
        } catch (const RtMidiError &e) {
            qWarning() << "MIDI: cannot list inputs:" << e.what();
        }
    }

    // RtMidi has no hot-plug notice, and a controller is exactly the sort
    // of thing that gets plugged in after XFB has started. Often enough that
    // a cable pulled and pushed back in is seen as gone, not as never moved:
    // then its input is reopened. On Linux the port announcements catch even
    // a replug quicker than this (see openPortWatch).
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(500);
    connect(m_pollTimer, &QTimer::timeout, this, &MidiController::pollPorts);

    if (m_enabled)
        openPortWatch();
    pollPorts();
    if (m_enabled) {
        reopenInputs();
        m_pollTimer->start();
    } else {
        m_status = tr("MIDI control is off.");
    }
}

MidiController::~MidiController()
{
    // Closing a port stops its callback thread before anything goes away.
    m_inputs.clear();
    closePortWatch();
}

bool MidiController::available()
{
    std::vector<RtMidi::Api> apis;
    RtMidi::getCompiledApi(apis);
    for (const RtMidi::Api api : apis) {
        if (api != RtMidi::RTMIDI_DUMMY && api != RtMidi::UNSPECIFIED)
            return true;
    }
    return false;
}

void MidiController::registerAction(const Midi::Action &action, Handler handler)
{
    m_actions.append(action);
    m_handlers.insert(action.id, std::move(handler));
}

void MidiController::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    save();
    if (m_enabled) {
        openPortWatch();
        pollPorts();
        reopenInputs();
        m_pollTimer->start();
    } else {
        m_pollTimer->stop();
        m_inputs.clear();
        m_openedAddresses.clear();
        closePortWatch();
        setStatus(tr("MIDI control is off."));
    }
}

void MidiController::setPort(const QString &port)
{
    if (m_port == port)
        return;
    m_port = port;
    save();
    if (m_enabled)
        reopenInputs();
}

QString MidiController::stablePortName(const QString &name)
{
    // ALSA appends "client:port" numbers, which it hands out afresh when a
    // device is plugged in again; stored with them, the choice would not
    // survive a reboot.
    static const QRegularExpression alsaNumbers(QStringLiteral("\\s+\\d+:\\d+$"));
    QString stable = name;
    stable.remove(alsaNumbers);
    return stable.trimmed();
}

void MidiController::pollPorts()
{
    if (!m_probe)
        return;
    QStringList raw;
    try {
        const unsigned int count = m_probe->getPortCount();
        for (unsigned int i = 0; i < count; ++i)
            raw << QString::fromStdString(m_probe->getPortName(i));
    } catch (const RtMidiError &e) {
        qWarning() << "MIDI: listing inputs failed:" << e.what();
        return;
    }
    // Read the announcements whether or not the list moved: they are the
    // only sign of a controller that left and came back under the same name
    // and numbers, and whose input has been deaf since.
    const bool lost = openedPortWentAway();
    if (raw == m_rawPortNames && !lost)
        return;
    m_rawPortNames = raw;

    QStringList names;
    for (const QString &name : std::as_const(raw))
        names << stablePortName(name);
    if (names != m_portNames) {
        m_portNames = names;
        emit portsChanged(m_portNames);
    }
    if (lost)
        qInfo() << "MIDI: a controller went away and came back; reopening its input";
    if (m_enabled)
        reopenInputs();
}

void MidiController::openPortWatch()
{
#ifdef __LINUX_ALSA__
    if (m_portWatch)
        return;
    snd_seq_t *seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK) < 0)
        return; // no sequencer: the port list alone will have to do
    snd_seq_set_client_name(seq, "XFB port watch");
    // Write-only and not exported: nobody lists it as a MIDI input, XFB's
    // own MIDI window included.
    const int port = snd_seq_create_simple_port(
        seq, "announcements", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_NO_EXPORT,
        SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0
            || snd_seq_connect_from(seq, port, SND_SEQ_CLIENT_SYSTEM,
                                    SND_SEQ_PORT_SYSTEM_ANNOUNCE) < 0) {
        snd_seq_close(seq);
        return;
    }
    m_portWatch = seq;
#endif
}

void MidiController::closePortWatch()
{
#ifdef __LINUX_ALSA__
    if (m_portWatch)
        snd_seq_close(static_cast<snd_seq_t *>(m_portWatch));
#endif
    m_portWatch = nullptr;
}

bool MidiController::openedPortWentAway()
{
    bool gone = false;
#ifdef __LINUX_ALSA__
    auto *seq = static_cast<snd_seq_t *>(m_portWatch);
    if (!seq)
        return false;
    // Queued since the last poll, so a controller that left and came back in
    // between is still in here, however quickly it did so.
    for (;;) {
        snd_seq_event_t *ev = nullptr;
        const int r = snd_seq_event_input(seq, &ev);
        if (r == -ENOSPC) {
            gone = true; // announcements were lost: assume the worst
            continue;
        }
        if (r < 0 || !ev)
            break; // -EAGAIN: nothing more queued
        if (ev->type == SND_SEQ_EVENT_PORT_EXIT) {
            const QString address = QStringLiteral("%1:%2")
                                        .arg(ev->data.addr.client).arg(ev->data.addr.port);
            if (m_openedAddresses.contains(address))
                gone = true;
        } else if (ev->type == SND_SEQ_EVENT_CLIENT_EXIT) {
            const QString prefix = QStringLiteral("%1:").arg(ev->data.addr.client);
            for (const QString &address : std::as_const(m_openedAddresses)) {
                if (address.startsWith(prefix))
                    gone = true;
            }
        }
    }
#endif
    return gone;
}

void MidiController::reopenInputs()
{
    m_inputs.clear();
    m_openedAddresses.clear();
    if (!m_enabled)
        return;
    if (!m_probe) {
        setStatus(tr("XFB cannot reach the system's MIDI service, so no controller "
                     "can be found. On Linux that is the ALSA sequencer (/dev/snd/seq); "
                     "in the Flatpak it needs the device permission."));
        return;
    }

    QStringList opened;
    QStringList failed;
    unsigned int count = 0;
    try {
        count = m_probe->getPortCount();
    } catch (const RtMidiError &) {
        count = 0;
    }
    for (unsigned int i = 0; i < count; ++i) {
        QString rawName;
        try {
            rawName = QString::fromStdString(m_probe->getPortName(i));
        } catch (const RtMidiError &) {
            continue;
        }
        const QString name = stablePortName(rawName);
        if (!m_port.isEmpty() && name != m_port)
            continue;
        // The ALSA loopback carries whatever other programs send it, which
        // is nobody's control desk.
        if (m_port.isEmpty() && name.startsWith(QLatin1String("Midi Through")))
            continue;
        try {
            auto input = std::make_unique<RtMidiIn>(RtMidi::UNSPECIFIED, "XFB");
            input->openPort(i, "XFB input");
            // Sysex, timing clock and active sensing: a desk sends plenty of
            // all three and none of it is a control moving.
            input->ignoreTypes(true, true, true);
            input->setCallback(&MidiController::rtMidiCallback, this);
            m_inputs.push_back(std::move(input));
            opened << name;
            static const QRegularExpression alsaAddress(QStringLiteral("(\\d+:\\d+)$"));
            const QRegularExpressionMatch match = alsaAddress.match(rawName);
            if (match.hasMatch())
                m_openedAddresses << match.captured(1);
        } catch (const RtMidiError &e) {
            qWarning() << "MIDI: cannot open" << name << ":" << e.what();
            failed << name;
        }
    }

    QString status;
    if (!opened.isEmpty())
        status = tr("Listening to %1.").arg(opened.join(QStringLiteral(", ")));
    else if (!m_port.isEmpty())
        status = tr("%1 is not connected.").arg(m_port);
    else
        status = tr("No MIDI controller is connected.");
    if (!failed.isEmpty())
        status += QLatin1Char(' ') + tr("Could not open %1.").arg(failed.join(QStringLiteral(", ")));
    qInfo().noquote() << "MIDI:" << status;
    setStatus(status);
}

void MidiController::setStatus(const QString &status)
{
    m_status = status;
    emit statusChanged(status);
}

void MidiController::rtMidiCallback(double, std::vector<unsigned char> *message, void *userData)
{
    // RtMidi's own thread. Hand the bytes to the GUI thread, where the
    // bindings and everything the actions touch live; if the controller
    // has gone by then, the queued call is simply dropped.
    auto *self = static_cast<MidiController *>(userData);
    if (!message || !self)
        return;
    const std::vector<unsigned char> bytes = *message;
    QMetaObject::invokeMethod(self, [self, bytes]() { self->onRawMessage(bytes); },
                              Qt::QueuedConnection);
}

void MidiController::onRawMessage(const std::vector<unsigned char> &bytes)
{
    Midi::Message m;
    if (Midi::parse(bytes.data(), bytes.size(), &m))
        handleMessage(m);
}

void MidiController::handleMessage(const Midi::Message &m)
{
    emit messageSeen(tr("%1, value %2")
                         .arg(Midi::describeControl(m.kind, m.channel, m.number))
                         .arg(m.value));

    if (!m_learning.isEmpty()) {
        // Releases do not teach anything: the press that came before them
        // already did, and a key let go must not overwrite it.
        if (m.kind == Midi::Kind::Note && m.noteOff)
            return;
        Midi::Binding b;
        b.action = m_learning;
        b.kind = m.kind;
        b.channel = m.channel;
        b.number = m.number;
        for (const Midi::Action &a : std::as_const(m_actions)) {
            if (a.id == m_learning && a.type == Midi::ActionType::Jog
                    && m.kind == Midi::Kind::Control)
                b.encoding = Midi::guessEncoding(m.value);
        }
        m_learning.clear();
        setBinding(b);
        emit learned(b);
        return;
    }

    for (const Midi::Binding &b : std::as_const(m_bindings)) {
        if (!b.matches(m))
            continue;
        const Handler handler = m_handlers.value(b.action);
        if (!handler)
            continue;
        Midi::ActionType type = Midi::ActionType::Button;
        for (const Midi::Action &a : std::as_const(m_actions)) {
            if (a.id == b.action) {
                type = a.type;
                break;
            }
        }
        switch (type) {
        case Midi::ActionType::Button:
            if (Midi::isPress(m))
                handler(1.0);
            break;
        case Midi::ActionType::Hold:
            handler(Midi::isPress(m) ? 1.0 : 0.0);
            break;
        case Midi::ActionType::Fader:
            handler(Midi::faderValue(m));
            break;
        case Midi::ActionType::Jog: {
            int steps = 0;
            if (b.encoding == Midi::Encoding::Absolute || m.kind != Midi::Kind::Control) {
                // A plain knob or a bend lever used as a jog: the movement
                // is the difference from where it last was.
                const int last = m_lastAbsolute.value(b.action, m.value);
                steps = m.value - last;
                m_lastAbsolute.insert(b.action, m.value);
            } else {
                steps = Midi::relativeSteps(b.encoding, m.value);
            }
            if (steps != 0)
                handler(double(steps));
            break;
        }
        }
    }
}

const Midi::Binding *MidiController::bindingFor(const QString &action) const
{
    for (const Midi::Binding &b : m_bindings) {
        if (b.action == action)
            return &b;
    }
    return nullptr;
}

void MidiController::setBinding(const Midi::Binding &binding)
{
    // One control per action, and one action per control: learning a
    // control that already does something moves it rather than making it
    // do two things at once, which on air is never what was meant.
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        const Midi::Binding &b = m_bindings.at(i);
        const bool sameControl = b.kind == binding.kind && b.channel == binding.channel
                                 && (b.kind == Midi::Kind::PitchBend || b.number == binding.number);
        if (b.action == binding.action || sameControl)
            m_bindings.removeAt(i);
    }
    m_bindings.append(binding);
    m_lastAbsolute.remove(binding.action);
    save();
    emit bindingsChanged();
}

void MidiController::clearBinding(const QString &action)
{
    bool removed = false;
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings.at(i).action == action) {
            m_bindings.removeAt(i);
            removed = true;
        }
    }
    if (removed) {
        save();
        emit bindingsChanged();
    }
}

void MidiController::clearAllBindings()
{
    if (m_bindings.isEmpty())
        return;
    m_bindings.clear();
    save();
    emit bindingsChanged();
}

void MidiController::startLearning(const QString &action)
{
    m_learning = action;
}

void MidiController::cancelLearning()
{
    m_learning.clear();
}

void MidiController::load()
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(kGroup);
    m_enabled = settings.value(QStringLiteral("Enabled"), false).toBool();
    m_port = settings.value(QStringLiteral("Port")).toString();
    const int count = settings.beginReadArray(QStringLiteral("Bindings"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        bool ok = false;
        Midi::Binding b;
        b.action = settings.value(QStringLiteral("action")).toString();
        b.kind = kindFromName(settings.value(QStringLiteral("kind")).toString(), &ok);
        b.channel = qBound(0, settings.value(QStringLiteral("channel")).toInt(), 15);
        b.number = qBound(0, settings.value(QStringLiteral("number")).toInt(), 127);
        b.encoding = encodingFromName(settings.value(QStringLiteral("encoding")).toString());
        if (ok && !b.action.isEmpty())
            m_bindings.append(b);
    }
    settings.endArray();
    settings.endGroup();
}

void MidiController::save() const
{
    QSettings settings(configPath(), QSettings::IniFormat);
    settings.beginGroup(kGroup);
    settings.setValue(QStringLiteral("Enabled"), m_enabled);
    settings.setValue(QStringLiteral("Port"), m_port);
    settings.remove(QStringLiteral("Bindings"));
    settings.beginWriteArray(QStringLiteral("Bindings"), m_bindings.size());
    for (int i = 0; i < m_bindings.size(); ++i) {
        const Midi::Binding &b = m_bindings.at(i);
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("action"), b.action);
        settings.setValue(QStringLiteral("kind"), kindName(b.kind));
        settings.setValue(QStringLiteral("channel"), b.channel);
        settings.setValue(QStringLiteral("number"), b.number);
        settings.setValue(QStringLiteral("encoding"), encodingName(b.encoding));
    }
    settings.endArray();
    settings.endGroup();
}
