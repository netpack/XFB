#include <QtTest/QtTest>

#include <cmath>
#include <vector>

#include "audio/FxDsp.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;
}

/**
 * @brief The DJ decks' tempo resampler (fxdsp::Varispeed).
 *
 * What a deck's pitch fader rests on:
 *
 *  - at rate 1 the output is the input, sample for sample, so a deck at 0 %
 *    sounds exactly as it did before tempo existed;
 *  - at any other rate the output is that much shorter or longer, and the
 *    input it reports consumed adds up to exactly what went through;
 *  - a tone comes out at the tempo's pitch (varispeed: 6 % faster is 6 %
 *    higher), with no clicks where one call's input meets the next;
 *  - a rate change between calls carries on from where it was.
 */
class TestVarispeed : public QObject
{
    Q_OBJECT

private slots:
    void rateOneIsACopy();
    void lengthFollowsTheRate_data();
    void lengthFollowsTheRate();
    void toneComesOutAtTheNewPitch();
    void chunkBoundariesAreSeamless();
    void neverConsumesMoreThanItIsGiven();

private:
    static std::vector<float> sine(double hz, int frames);
    /** Feeds @a in through in chunks of @a chunk frames, as the engine's fifo does. */
    static std::vector<float> run(fxdsp::Varispeed &v, const std::vector<float> &in,
                                  double rate, int chunk, int *consumedTotal = nullptr);
};

std::vector<float> TestVarispeed::sine(double hz, int frames)
{
    std::vector<float> out(size_t(frames) * 2);
    for (int i = 0; i < frames; ++i) {
        const float v = float(0.5 * std::sin(2.0 * kPi * hz * i / 48000.0));
        out[size_t(i) * 2] = v;
        out[size_t(i) * 2 + 1] = v;
    }
    return out;
}

std::vector<float> TestVarispeed::run(fxdsp::Varispeed &v, const std::vector<float> &in,
                                      double rate, int chunk, int *consumedTotal)
{
    std::vector<float> fifo;
    std::vector<float> out;
    std::vector<float> buf(4096 * 2);
    size_t fed = 0;
    int total = 0;
    while (true) {
        // Top the fifo up a chunk at a time, like a decoder pipe does.
        if (fed < in.size()) {
            const size_t take = std::min(in.size() - fed, size_t(chunk) * 2);
            fifo.insert(fifo.end(), in.begin() + long(fed), in.begin() + long(fed + take));
            fed += take;
        }
        int consumed = 0;
        const int n = v.process(fifo.data(), int(fifo.size() / 2), buf.data(), 4096,
                                rate, &consumed);
        out.insert(out.end(), buf.begin(), buf.begin() + n * 2);
        fifo.erase(fifo.begin(), fifo.begin() + consumed * 2);
        total += consumed;
        if (n == 0 && fed >= in.size())
            break;
    }
    if (consumedTotal)
        *consumedTotal = total;
    return out;
}

void TestVarispeed::rateOneIsACopy()
{
    const std::vector<float> in = sine(997.0, 20000);
    fxdsp::Varispeed v;
    int consumed = 0;
    const std::vector<float> out = run(v, in, 1.0, 1000, &consumed);

    // Everything but the two frames of lookahead it keeps.
    QCOMPARE(int(out.size() / 2), 20000 - 2);
    QCOMPARE(consumed, 20000 - 2);
    for (size_t i = 0; i < out.size(); ++i)
        QCOMPARE(out[i], in[i]);
    QVERIFY(v.atFrameBoundary());
}

void TestVarispeed::lengthFollowsTheRate_data()
{
    QTest::addColumn<double>("rate");
    QTest::newRow("-8 %") << 0.92;
    QTest::newRow("+3.12 %") << 1.0312;
    QTest::newRow("+8 %") << 1.08;
    QTest::newRow("+50 %") << 1.5;
    QTest::newRow("-50 %") << 0.5;
}

void TestVarispeed::lengthFollowsTheRate()
{
    QFETCH(double, rate);
    const int frames = 48000;
    fxdsp::Varispeed v;
    int consumed = 0;
    const std::vector<float> out = run(v, sine(440.0, frames), rate, 777, &consumed);

    const double expected = frames / rate;
    // Short by the two frames of lookahead it holds back, at this rate.
    const double tolerance = 2.0 / rate + 1.0;
    QVERIFY2(std::abs(double(out.size() / 2) - expected) <= tolerance,
             qPrintable(QString("%1 frames out, expected %2").arg(out.size() / 2).arg(expected)));
    QVERIFY(consumed <= frames);
    QVERIFY(consumed >= frames - 2);
}

void TestVarispeed::toneComesOutAtTheNewPitch()
{
    const double rate = 1.06;
    fxdsp::Varispeed v;
    const std::vector<float> out = run(v, sine(1000.0, 48000 * 2), rate, 2048);

    // Count rising zero crossings over one second of output.
    int crossings = 0;
    for (size_t i = 2; i < 48000 * 2; i += 2) {
        if (out[i - 2] < 0.0f && out[i] >= 0.0f)
            ++crossings;
    }
    QVERIFY2(std::abs(crossings - 1060) <= 1, qPrintable(QString::number(crossings)));
}

void TestVarispeed::chunkBoundariesAreSeamless()
{
    // A low tone: consecutive samples differ by little, so a click where one
    // call's input meets the next would stand out as a large step.
    const double rate = 1.0312;
    fxdsp::Varispeed v;
    const std::vector<float> out = run(v, sine(100.0, 48000), rate, 333);

    const double maxStep = 0.5 * 2.0 * kPi * 100.0 * rate / 48000.0; // the sine's own slope
    for (size_t i = 2; i < out.size(); i += 2)
        QVERIFY2(std::abs(out[i] - out[i - 2]) <= maxStep * 1.05,
                 qPrintable(QString("step %1 at frame %2").arg(out[i] - out[i - 2]).arg(i / 2)));
}

void TestVarispeed::neverConsumesMoreThanItIsGiven()
{
    fxdsp::Varispeed v;
    const std::vector<float> in = sine(440.0, 5);
    std::vector<float> out(200);
    for (const double rate : { 0.5, 1.0, 1.5, 1.9 }) {
        v.reset();
        int consumed = -1;
        const int n = v.process(in.data(), 5, out.data(), 100, rate, &consumed);
        QVERIFY(consumed >= 0);
        QVERIFY(consumed <= 5);
        QVERIFY(n >= 0);
    }
    // Nothing to interpolate between: nothing out, nothing taken.
    int consumed = -1;
    QCOMPARE(v.process(in.data(), 2, out.data(), 100, 1.2, &consumed), 0);
    QCOMPARE(consumed, 0);
}

QTEST_GUILESS_MAIN(TestVarispeed)
#include "TestVarispeed.moc"
