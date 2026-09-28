#include <QtTest/QtTest>

#include <QSignalSpy>
#include <QStandardPaths>

#include "services/MidiController.h"

/**
 * @brief MIDI learn, without a device.
 *
 * Messages are handed to MidiController::handleMessage() exactly as the
 * RtMidi callback hands them over, so everything but the port itself is
 * exercised:
 *
 *  - the three message types a desk sends are read correctly, including a
 *    note-on at velocity 0 being a release;
 *  - a button fires once per press, a hold is told press and release, a
 *    fader gets its position and a jog wheel the steps it turned, in each
 *    encoding;
 *  - learning takes the first press, not a release, guesses a jog wheel's
 *    encoding, and moves a control rather than giving it two jobs;
 *  - bindings survive a restart;
 *  - an ALSA port is recognised again after its numbers change.
 */
class TestMidiController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void parsesTheMessagesADeskSends();
    void ignoresEverythingElse();
    void buttonFiresOncePerPress();
    void holdIsToldPressAndRelease();
    void faderGetsItsPosition();
    void jogWheelEncodings();
    void absoluteKnobAsJogSendsTheDifference();
    void learningTakesThePressNotTheRelease();
    void learningGuessesAJogWheelsEncoding();
    void aControlHasOneJob();
    void bindingsSurviveARestart();
    void alsaPortNumbersAreDropped();

private:
    static Midi::Message cc(int number, int value, int channel = 0);
    static Midi::Message note(int number, int velocity, int channel = 0);
};

Midi::Message TestMidiController::cc(int number, int value, int channel)
{
    const unsigned char bytes[] = { static_cast<unsigned char>(0xB0 | channel),
                                    static_cast<unsigned char>(number),
                                    static_cast<unsigned char>(value) };
    Midi::Message m;
    Midi::parse(bytes, 3, &m);
    return m;
}

Midi::Message TestMidiController::note(int number, int velocity, int channel)
{
    const unsigned char bytes[] = { static_cast<unsigned char>(0x90 | channel),
                                    static_cast<unsigned char>(number),
                                    static_cast<unsigned char>(velocity) };
    Midi::Message m;
    Midi::parse(bytes, 3, &m);
    return m;
}

void TestMidiController::initTestCase()
{
    // xfb.conf goes to a throwaway location, not the developer's own.
    QStandardPaths::setTestModeEnabled(true);
}

void TestMidiController::init()
{
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                  + QStringLiteral("/xfb.conf"));
}

void TestMidiController::parsesTheMessagesADeskSends()
{
    Midi::Message m;
    const unsigned char on[] = { 0x93, 60, 100 };
    QVERIFY(Midi::parse(on, 3, &m));
    QCOMPARE(m.kind, Midi::Kind::Note);
    QCOMPARE(m.channel, 3);
    QCOMPARE(m.number, 60);
    QCOMPARE(m.value, 100);
    QVERIFY(!m.noteOff);
    QVERIFY(Midi::isPress(m));

    const unsigned char onZero[] = { 0x90, 60, 0 };
    QVERIFY(Midi::parse(onZero, 3, &m));
    QVERIFY(m.noteOff);
    QVERIFY(!Midi::isPress(m));

    const unsigned char off[] = { 0x80, 60, 64 };
    QVERIFY(Midi::parse(off, 3, &m));
    QVERIFY(m.noteOff);

    const unsigned char control[] = { 0xB1, 7, 127 };
    QVERIFY(Midi::parse(control, 3, &m));
    QCOMPARE(m.kind, Midi::Kind::Control);
    QCOMPARE(m.channel, 1);
    QCOMPARE(m.number, 7);
    QCOMPARE(Midi::faderValue(m), 1.0);

    const unsigned char bendMiddle[] = { 0xE0, 0x00, 0x40 };
    QVERIFY(Midi::parse(bendMiddle, 3, &m));
    QCOMPARE(m.kind, Midi::Kind::PitchBend);
    QCOMPARE(m.value, 8192);
    const unsigned char bendTop[] = { 0xE0, 0x7F, 0x7F };
    QVERIFY(Midi::parse(bendTop, 3, &m));
    QCOMPARE(Midi::faderValue(m), 1.0);

    QCOMPARE(Midi::describeControl(Midi::Kind::Note, 9, 60),
             QStringLiteral("note C4 (60), channel 10"));
    QCOMPARE(Midi::describeControl(Midi::Kind::Control, 0, 7),
             QStringLiteral("CC 7, channel 1"));
}

void TestMidiController::ignoresEverythingElse()
{
    Midi::Message m;
    const unsigned char clock[] = { 0xF8 };
    QVERIFY(!Midi::parse(clock, 1, &m));
    const unsigned char aftertouch[] = { 0xA0, 60, 10 };
    QVERIFY(!Midi::parse(aftertouch, 3, &m));
    const unsigned char programChange[] = { 0xC0, 5, 0 };
    QVERIFY(!Midi::parse(programChange, 3, &m));
    QVERIFY(!Midi::parse(nullptr, 0, &m));
}

void TestMidiController::buttonFiresOncePerPress()
{
    MidiController c;
    int fired = 0;
    c.registerAction({ QStringLiteral("play"), QStringLiteral("Station"), QStringLiteral("Play"),
                       Midi::ActionType::Button },
                     [&](double) { ++fired; });
    c.setBinding({ QStringLiteral("play"), Midi::Kind::Note, 0, 36 });

    c.handleMessage(note(36, 127));
    c.handleMessage(note(36, 0));   // release
    QCOMPARE(fired, 1);
    c.handleMessage(note(37, 127)); // another pad
    c.handleMessage(note(36, 127, 1)); // same pad, other channel
    QCOMPARE(fired, 1);
    c.handleMessage(note(36, 90));
    QCOMPARE(fired, 2);
}

void TestMidiController::holdIsToldPressAndRelease()
{
    MidiController c;
    QList<double> values;
    c.registerAction({ QStringLiteral("nudge"), QStringLiteral("Deck 1"), QStringLiteral("Nudge"),
                       Midi::ActionType::Hold },
                     [&](double v) { values << v; });
    c.setBinding({ QStringLiteral("nudge"), Midi::Kind::Control, 0, 20 });

    c.handleMessage(cc(20, 127));
    c.handleMessage(cc(20, 0));
    QCOMPARE(values, (QList<double>{ 1.0, 0.0 }));
}

void TestMidiController::faderGetsItsPosition()
{
    MidiController c;
    double last = -1;
    c.registerAction({ QStringLiteral("vol"), QStringLiteral("Station"), QStringLiteral("Volume"),
                       Midi::ActionType::Fader },
                     [&](double v) { last = v; });
    c.setBinding({ QStringLiteral("vol"), Midi::Kind::Control, 0, 7 });

    c.handleMessage(cc(7, 0));
    QCOMPARE(last, 0.0);
    c.handleMessage(cc(7, 127));
    QCOMPARE(last, 1.0);
    c.handleMessage(cc(7, 64));
    QVERIFY(std::abs(last - 64.0 / 127.0) < 1e-9);
}

void TestMidiController::jogWheelEncodings()
{
    MidiController c;
    QList<double> steps;
    c.registerAction({ QStringLiteral("jog"), QStringLiteral("Deck 1"), QStringLiteral("Jog"),
                       Midi::ActionType::Jog },
                     [&](double s) { steps << s; });

    c.setBinding({ QStringLiteral("jog"), Midi::Kind::Control, 0, 30,
                   Midi::Encoding::TwosComplement });
    c.handleMessage(cc(30, 1));
    c.handleMessage(cc(30, 3));
    c.handleMessage(cc(30, 127));
    c.handleMessage(cc(30, 125));
    QCOMPARE(steps, (QList<double>{ 1, 3, -1, -3 }));

    steps.clear();
    c.setBinding({ QStringLiteral("jog"), Midi::Kind::Control, 0, 30,
                   Midi::Encoding::Offset64 });
    c.handleMessage(cc(30, 65));
    c.handleMessage(cc(30, 60));
    c.handleMessage(cc(30, 64)); // still: nothing to do
    QCOMPARE(steps, (QList<double>{ 1, -4 }));
}

void TestMidiController::absoluteKnobAsJogSendsTheDifference()
{
    MidiController c;
    QList<double> steps;
    c.registerAction({ QStringLiteral("jog"), QStringLiteral("Deck 1"), QStringLiteral("Jog"),
                       Midi::ActionType::Jog },
                     [&](double s) { steps << s; });
    c.setBinding({ QStringLiteral("jog"), Midi::Kind::Control, 0, 30,
                   Midi::Encoding::Absolute });

    c.handleMessage(cc(30, 50)); // where it is; no movement yet
    c.handleMessage(cc(30, 53));
    c.handleMessage(cc(30, 51));
    QCOMPARE(steps, (QList<double>{ 3, -2 }));
}

void TestMidiController::learningTakesThePressNotTheRelease()
{
    MidiController c;
    c.registerAction({ QStringLiteral("play"), QStringLiteral("Station"), QStringLiteral("Play"),
                       Midi::ActionType::Button },
                     [](double) {});
    QSignalSpy learned(&c, &MidiController::learned);

    c.startLearning(QStringLiteral("play"));
    c.handleMessage(note(40, 0)); // a key let go from earlier
    QCOMPARE(learned.count(), 0);
    QCOMPARE(c.learningAction(), QStringLiteral("play"));

    c.handleMessage(note(41, 100, 2));
    QCOMPARE(learned.count(), 1);
    QVERIFY(c.learningAction().isEmpty());
    const Midi::Binding *b = c.bindingFor(QStringLiteral("play"));
    QVERIFY(b);
    QCOMPARE(b->kind, Midi::Kind::Note);
    QCOMPARE(b->number, 41);
    QCOMPARE(b->channel, 2);
}

void TestMidiController::learningGuessesAJogWheelsEncoding()
{
    MidiController c;
    c.registerAction({ QStringLiteral("jog"), QStringLiteral("Deck 1"), QStringLiteral("Jog"),
                       Midi::ActionType::Jog },
                     [](double) {});

    c.startLearning(QStringLiteral("jog"));
    c.handleMessage(cc(30, 127));
    QCOMPARE(c.bindingFor(QStringLiteral("jog"))->encoding, Midi::Encoding::TwosComplement);

    c.startLearning(QStringLiteral("jog"));
    c.handleMessage(cc(31, 63));
    QCOMPARE(c.bindingFor(QStringLiteral("jog"))->encoding, Midi::Encoding::Offset64);

    c.startLearning(QStringLiteral("jog"));
    c.handleMessage(cc(32, 40));
    QCOMPARE(c.bindingFor(QStringLiteral("jog"))->encoding, Midi::Encoding::Absolute);
}

void TestMidiController::aControlHasOneJob()
{
    MidiController c;
    int play = 0, stop = 0;
    c.registerAction({ QStringLiteral("play"), QStringLiteral("Station"), QStringLiteral("Play"),
                       Midi::ActionType::Button },
                     [&](double) { ++play; });
    c.registerAction({ QStringLiteral("stop"), QStringLiteral("Station"), QStringLiteral("Stop"),
                       Midi::ActionType::Button },
                     [&](double) { ++stop; });
    c.setBinding({ QStringLiteral("play"), Midi::Kind::Note, 0, 36 });
    // The same pad learnt for Stop: it stops, and no longer also plays.
    c.setBinding({ QStringLiteral("stop"), Midi::Kind::Note, 0, 36 });

    c.handleMessage(note(36, 127));
    QCOMPARE(play, 0);
    QCOMPARE(stop, 1);
    QVERIFY(!c.bindingFor(QStringLiteral("play")));
}

void TestMidiController::bindingsSurviveARestart()
{
    {
        MidiController c;
        c.setBinding({ QStringLiteral("deck1.jog"), Midi::Kind::Control, 4, 33,
                       Midi::Encoding::Offset64 });
        c.setBinding({ QStringLiteral("station.volume"), Midi::Kind::PitchBend, 1, 0 });
        c.setPort(QStringLiteral("Some Controller MIDI 1"));
    }
    MidiController c;
    QCOMPARE(c.bindings().size(), 2);
    const Midi::Binding *jog = c.bindingFor(QStringLiteral("deck1.jog"));
    QVERIFY(jog);
    QCOMPARE(jog->channel, 4);
    QCOMPARE(jog->number, 33);
    QCOMPARE(jog->encoding, Midi::Encoding::Offset64);
    const Midi::Binding *vol = c.bindingFor(QStringLiteral("station.volume"));
    QVERIFY(vol);
    QCOMPARE(vol->kind, Midi::Kind::PitchBend);
    QCOMPARE(c.port(), QStringLiteral("Some Controller MIDI 1"));
    QVERIFY(!c.isEnabled()); // off until the operator turns it on
}

void TestMidiController::alsaPortNumbersAreDropped()
{
    QCOMPARE(MidiController::stablePortName(
                 QStringLiteral("Traktor Kontrol S2 MK3:Traktor Kontrol S2 MK3 MIDI 1 24:0")),
             QStringLiteral("Traktor Kontrol S2 MK3:Traktor Kontrol S2 MK3 MIDI 1"));
    QCOMPARE(MidiController::stablePortName(QStringLiteral("DJControl Inpulse 200")),
             QStringLiteral("DJControl Inpulse 200"));
}

QTEST_GUILESS_MAIN(TestMidiController)
#include "TestMidiController.moc"
