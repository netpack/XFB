#ifndef FXPARAMS_H
#define FXPARAMS_H

#include <QString>
#include <QSettings>
#include <QStandardPaths>

/**
 * @brief Parameters for one player channel of the XFB audio FX chain.
 *
 * Covers the 432 Hz retune mode, a 10-band graphic equalizer and a
 * broadcast-style dynamic range compressor. Values are persisted in the
 * same xfb.conf INI file used by the rest of the application.
 */
struct FxParams
{
    static constexpr int kBands = 10;

    // ISO octave band centre frequencies (Hz)
    static constexpr double kBandFreqHz[kBands] = {
        31.25, 62.5, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0
    };

    // Global: play everything retuned from A=440 Hz to A=432 Hz
    bool retune432 = false;

    // Equalizer
    bool eqEnabled = false;
    double eqGainDb[kBands] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    double preampDb = 0.0;

    // Compressor
    bool compEnabled = false;
    double compThresholdDb = -18.0;
    double compRatio = 3.0;        // n:1
    double compAttackMs = 10.0;
    double compReleaseMs = 150.0;
    double compMakeupDb = 0.0;

    bool anyActive() const { return retune432 || eqEnabled || compEnabled; }

    bool eqIsFlat() const
    {
        if (preampDb != 0.0)
            return false;
        for (int i = 0; i < kBands; ++i)
            if (eqGainDb[i] != 0.0)
                return false;
        return true;
    }
};

/**
 * @brief Load/save FxParams from the shared xfb.conf configuration file.
 *
 * Channel names used by XFB: "Main", "LP1", "LP2". The 432 Hz retune flag
 * is global (one switch for the whole application) while EQ and compressor
 * settings are stored per channel.
 */
namespace FxSettings
{
inline QString configFilePath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return base + "/xfb.conf";
}

inline bool loadRetune432()
{
    QSettings s(configFilePath(), QSettings::IniFormat);
    return s.value("Fx/Retune432", false).toBool();
}

inline void saveRetune432(bool on)
{
    QSettings s(configFilePath(), QSettings::IniFormat);
    s.setValue("Fx/Retune432", on);
}

inline FxParams loadChannel(const QString &channel)
{
    QSettings s(configFilePath(), QSettings::IniFormat);
    FxParams p;
    p.retune432 = s.value("Fx/Retune432", false).toBool();
    const QString prefix = "Fx/" + channel + "/";
    p.eqEnabled = s.value(prefix + "EqEnabled", false).toBool();
    p.preampDb = s.value(prefix + "PreampDb", 0.0).toDouble();
    for (int i = 0; i < FxParams::kBands; ++i)
        p.eqGainDb[i] = s.value(prefix + QString("EqBand%1").arg(i), 0.0).toDouble();
    p.compEnabled = s.value(prefix + "CompEnabled", false).toBool();
    p.compThresholdDb = s.value(prefix + "CompThresholdDb", -18.0).toDouble();
    p.compRatio = s.value(prefix + "CompRatio", 3.0).toDouble();
    p.compAttackMs = s.value(prefix + "CompAttackMs", 10.0).toDouble();
    p.compReleaseMs = s.value(prefix + "CompReleaseMs", 150.0).toDouble();
    p.compMakeupDb = s.value(prefix + "CompMakeupDb", 0.0).toDouble();
    return p;
}

inline void saveChannel(const QString &channel, const FxParams &p)
{
    // Note: the global Fx/Retune432 key is only ever written by
    // saveRetune432() (Options dialog) — channel saves must not touch it.
    QSettings s(configFilePath(), QSettings::IniFormat);
    const QString prefix = "Fx/" + channel + "/";
    s.setValue(prefix + "EqEnabled", p.eqEnabled);
    s.setValue(prefix + "PreampDb", p.preampDb);
    for (int i = 0; i < FxParams::kBands; ++i)
        s.setValue(prefix + QString("EqBand%1").arg(i), p.eqGainDb[i]);
    s.setValue(prefix + "CompEnabled", p.compEnabled);
    s.setValue(prefix + "CompThresholdDb", p.compThresholdDb);
    s.setValue(prefix + "CompRatio", p.compRatio);
    s.setValue(prefix + "CompAttackMs", p.compAttackMs);
    s.setValue(prefix + "CompReleaseMs", p.compReleaseMs);
    s.setValue(prefix + "CompMakeupDb", p.compMakeupDb);
}
} // namespace FxSettings

#endif // FXPARAMS_H
