#include <QtTest/QtTest>

#include "../../../src/services/UpdateCheckService.h"

/**
 * @brief Unit tests for UpdateCheckService::isNewerVersion.
 *
 * This exists because of a real, silent failure. XFB's versions are digits of
 * pi, so the fractional segment keeps growing, and at "3.14159265358" it passed
 * INT_MAX (14159265358 > 2147483647). The old implementation used
 * QVersionNumber, whose segments are int: it stopped parsing at the overflow
 * and returned plain "3", which compares as OLDER than 3.141592653. Every user
 * asking for an update was told they already had the latest version, with
 * nothing in the log to say otherwise.
 *
 * The release sequence below is the project's actual history, so a future digit
 * that breaks the comparison again fails here first.
 *
 * An XFB version is a number — pi, to as many places as the release deserves —
 * so the digits after the point are compared as a decimal fraction, not as an
 * integer. That is what makes 3.1416 (pi rounded at four places) newer than
 * 3.14159265358 (pi truncated at eleven) despite being shorter.
 */
class TestUpdateCheckService : public QObject
{
    Q_OBJECT

private slots:
    void releaseSequenceIsStrictlyIncreasing();
    void theOverflowRegression();
    void ignoresLeadingV();
    void identicalVersionIsNotNewer();
    void survivesFarMoreDigits();
    void unparseableTagsNeverOfferAnUpdate_data();
    void unparseableTagsNeverOfferAnUpdate();
    void missingSegmentsCountAsZero();
    void theFractionIsADecimalNotAnInteger();
    void roundedPiBeatsLongerTruncatedPi();
};

// Every version XFB has shipped, oldest first. Note the last step: the digits
// stop growing and pi is rounded instead, which is shorter but larger.
static const QStringList kReleases = {
    QStringLiteral("3.14159"),
    QStringLiteral("3.141592"),
    QStringLiteral("3.1415926"),
    QStringLiteral("3.14159265"),
    QStringLiteral("3.141592653"),
    QStringLiteral("3.1415926535"),
    QStringLiteral("3.14159265358"),
    QStringLiteral("3.1416"),
};

void TestUpdateCheckService::releaseSequenceIsStrictlyIncreasing()
{
    for (int i = 1; i < kReleases.size(); ++i) {
        const QString older = kReleases.at(i - 1);
        const QString newer = kReleases.at(i);
        QVERIFY2(UpdateCheckService::isNewerVersion(newer, older),
                 qPrintable(QStringLiteral("%1 should be newer than %2").arg(newer, older)));
        QVERIFY2(!UpdateCheckService::isNewerVersion(older, newer),
                 qPrintable(QStringLiteral("%1 must not be newer than %2").arg(older, newer)));
    }

    // Also across the whole history, not just neighbours: the newest release
    // must be offered to someone still on the very first one.
    QVERIFY(UpdateCheckService::isNewerVersion(kReleases.last(), kReleases.first()));
}

void TestUpdateCheckService::theOverflowRegression()
{
    // The exact case that was reported: running 3.141592653, offered v3.14159265358.
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("v3.14159265358"),
                                               QStringLiteral("3.141592653")));
}

void TestUpdateCheckService::ignoresLeadingV()
{
    // GitHub tags carry the "v"; the running version does not.
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("v3.14159265358"),
                                               QStringLiteral("3.1415926535")));
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("V3.14159265358"),
                                               QStringLiteral("3.1415926535")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("v3.1415926535"),
                                                QStringLiteral("3.14159265358")));
}

void TestUpdateCheckService::identicalVersionIsNotNewer()
{
    for (const QString &v : kReleases)
        QVERIFY(!UpdateCheckService::isNewerVersion(v, v));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("v3.14159265358"),
                                                QStringLiteral("3.14159265358")));
}

void TestUpdateCheckService::survivesFarMoreDigits()
{
    // Well past what even a 64-bit integer segment could hold, so however many
    // digits a future release carries, the comparison still works.
    const QString many = QStringLiteral("3.1415926535897932384626433832795");
    QVERIFY(UpdateCheckService::isNewerVersion(many, QStringLiteral("3.14159265358")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14159265358"), many));

    // And it is still correctly BELOW 3.1416: pi truncated is always less than
    // pi rounded up at the fourth place, no matter how far the truncation runs.
    QVERIFY(!UpdateCheckService::isNewerVersion(many, QStringLiteral("3.1416")));
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("3.1416"), many));
}

void TestUpdateCheckService::unparseableTagsNeverOfferAnUpdate_data()
{
    QTest::addColumn<QString>("tag");
    QTest::newRow("empty")          << QString();
    QTest::newRow("word")           << QStringLiteral("nightly");
    QTest::newRow("suffix")         << QStringLiteral("3.14159-beta");
    QTest::newRow("text segment")   << QStringLiteral("3.14.beta");
    QTest::newRow("double dot")     << QStringLiteral("3..14159");
    QTest::newRow("trailing dot")   << QStringLiteral("3.14159.");
    QTest::newRow("leading dot")    << QStringLiteral(".14159");
    QTest::newRow("just a v")       << QStringLiteral("v");
}

void TestUpdateCheckService::unparseableTagsNeverOfferAnUpdate()
{
    QFETCH(QString, tag);
    // A tag shape we do not understand must never be treated as an upgrade,
    // in either direction.
    QVERIFY(!UpdateCheckService::isNewerVersion(tag, QStringLiteral("3.14159")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14159"), tag));
}

void TestUpdateCheckService::missingSegmentsCountAsZero()
{
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14.0"), QStringLiteral("3.14")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14"), QStringLiteral("3.14.0")));
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("3.14.1"), QStringLiteral("3.14")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14"), QStringLiteral("3.14.1")));
}

void TestUpdateCheckService::theFractionIsADecimalNotAnInteger()
{
    // Trailing zeros are not significant: .14 == .140
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.140"), QStringLiteral("3.14")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14"), QStringLiteral("3.140")));

    // Leading zeros ARE significant: .0014 < .14 (as an integer they would tie)
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("3.14"), QStringLiteral("3.0014")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.0014"), QStringLiteral("3.14")));

    // A shorter fraction can still be larger: .2 > .14
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("3.2"), QStringLiteral("3.14")));
    QVERIFY(!UpdateCheckService::isNewerVersion(QStringLiteral("3.14"), QStringLiteral("3.2")));

    // The whole part still wins outright, and still compares as an integer.
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("4.0"), kReleases.last()));
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("10.1"), QStringLiteral("9.9")));
}

void TestUpdateCheckService::roundedPiBeatsLongerTruncatedPi()
{
    // 3.1416 is pi rounded at four places: .14160000000 > .14159265358, so it
    // must be offered to everyone on any earlier release even though the
    // version string is shorter than the one it replaces.
    const QString rounded = QStringLiteral("3.1416");
    for (const QString &v : kReleases) {
        if (v == rounded)
            continue; // it is the current release, not an earlier one
        QVERIFY2(UpdateCheckService::isNewerVersion(rounded, v),
                 qPrintable(QStringLiteral("3.1416 should be newer than %1").arg(v)));
        QVERIFY2(!UpdateCheckService::isNewerVersion(v, rounded),
                 qPrintable(QStringLiteral("%1 must not be newer than 3.1416").arg(v)));
    }
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("v3.1416"),
                                               QStringLiteral("3.14159265358")));

    // And the scheme keeps working from there.
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("3.14161"), rounded));
    QVERIFY(UpdateCheckService::isNewerVersion(QStringLiteral("3.1417"), rounded));
    QVERIFY(!UpdateCheckService::isNewerVersion(rounded, QStringLiteral("3.1417")));
}

QTEST_MAIN(TestUpdateCheckService)
#include "TestUpdateCheckService.moc"
