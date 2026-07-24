#include "mediaduration.h"
#include "services/DependencyChecker.h"
#include "audio/FxEngine.h"

#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

namespace {

QString formatSeconds(double totalSeconds)
{
    if (totalSeconds < 0)
        return QString();
    qint64 secs = qint64(totalSeconds + 0.5);
    qint64 h = secs / 3600;
    qint64 m = (secs % 3600) / 60;
    qint64 s = secs % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h)
        .arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0'));
}

} // namespace

namespace MediaDuration {

QString normalize(const QString &raw)
{
    const QString value = raw.trimmed();
    if (value.isEmpty())
        return QString();

    // "h:mm:ss" or "hh:mm:ss.ff" (exiftool and ffmpeg timestamp forms)
    static const QRegularExpression hms(
        QStringLiteral("(\\d+):(\\d{1,2}):(\\d{1,2}(?:\\.\\d+)?)"));
    QRegularExpressionMatch m = hms.match(value);
    if (m.hasMatch()) {
        return formatSeconds(m.captured(1).toDouble() * 3600.0
                             + m.captured(2).toDouble() * 60.0
                             + m.captured(3).toDouble());
    }

    // "mm:ss" (exiftool uses this for tracks under an hour in some formats)
    static const QRegularExpression ms(QStringLiteral("^(\\d{1,3}):(\\d{2}(?:\\.\\d+)?)$"));
    m = ms.match(value);
    if (m.hasMatch())
        return formatSeconds(m.captured(1).toDouble() * 60.0 + m.captured(2).toDouble());

    // "3.07 s" / "185 s" (exiftool's plain-seconds form for short files —
    // the old split(" ").last() parse stored a literal "s" for these)
    static const QRegularExpression secs(QStringLiteral("^(\\d+(?:\\.\\d+)?)\\s*s\\b"));
    m = secs.match(value);
    if (m.hasMatch())
        return formatSeconds(m.captured(1).toDouble());

    return QString();
}

static QString viaExiftool(const QString &filePath)
{
    const QString exiftool = DependencyChecker::resolveExecutable(QStringLiteral("exiftool"));
    if (exiftool.isEmpty())
        return QString();

    QProcess p;
    // -T -Duration prints just the value (tab-separated table mode)
    p.start(exiftool, {QStringLiteral("-T"), QStringLiteral("-Duration"), filePath});
    if (!p.waitForFinished(15000)) {
        p.kill();
        p.waitForFinished(1000);
        return QString();
    }
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
        return QString();
    return normalize(QString::fromUtf8(p.readAllStandardOutput()));
}

static QString viaFfmpeg(const QString &filePath)
{
    const QString ffmpeg = FxEngine::ffmpegExecutable();
    if (ffmpeg.isEmpty())
        return QString();

    // "ffmpeg -i <file>" exits non-zero (no output file) but prints
    // "Duration: 00:03:45.12," to stderr — that's all we need.
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(ffmpeg, {QStringLiteral("-hide_banner"), QStringLiteral("-i"), filePath});
    if (!p.waitForFinished(15000)) {
        p.kill();
        p.waitForFinished(1000);
        return QString();
    }
    const QString out = QString::fromUtf8(p.readAllStandardOutput());
    static const QRegularExpression dur(
        QStringLiteral("Duration:\\s*(\\d+:\\d{2}:\\d{2}(?:\\.\\d+)?)"));
    const QRegularExpressionMatch m = dur.match(out);
    if (!m.hasMatch())
        return QString();
    return normalize(m.captured(1));
}

QString forFile(const QString &filePath)
{
    QString time = viaExiftool(filePath);
    if (time.isEmpty())
        time = viaFfmpeg(filePath);
    if (time.isEmpty())
        qWarning() << "MediaDuration: could not determine duration of" << filePath;
    return time;
}

bool probeAvailable()
{
    return !DependencyChecker::resolveExecutable(QStringLiteral("exiftool")).isEmpty()
        || !FxEngine::ffmpegExecutable().isEmpty();
}

} // namespace MediaDuration
