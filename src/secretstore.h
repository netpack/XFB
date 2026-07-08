#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <QByteArray>
#include <QFile>
#include <QString>

/**
 * @brief Obfuscation for credentials stored in xfb.conf, plus config file
 *        permission hardening.
 *
 * This is deliberately labelled obfuscation, not encryption: the key ships
 * with the application, so it only protects against casual disclosure
 * (shoulder-surfing, pasting a config into a bug report, backup tools that
 * index plaintext). The real access control is restrictFile(), which keeps
 * the config readable by the owning user only. For stronger protection a
 * platform keychain would be required.
 *
 * Format: "xfb1:" + base64(XOR(utf8(secret), key)). Legacy plaintext values
 * (no prefix) are still readable and get sealed on the next save.
 */
namespace SecretStore
{
inline QByteArray xorWithKey(QByteArray data)
{
    static const QByteArray key = QByteArrayLiteral("XFB.SecretStore.v1.obfuscation-key");
    for (int i = 0; i < data.size(); ++i)
        data[i] = data[i] ^ key[i % key.size()];
    return data;
}

inline QString seal(const QString &plain)
{
    if (plain.isEmpty())
        return plain;
    return QStringLiteral("xfb1:")
           + QString::fromLatin1(xorWithKey(plain.toUtf8()).toBase64());
}

inline QString open(const QString &stored)
{
    if (!stored.startsWith(QStringLiteral("xfb1:")))
        return stored; // legacy plaintext value
    const QByteArray raw = QByteArray::fromBase64(stored.mid(5).toLatin1());
    return QString::fromUtf8(xorWithKey(raw));
}

/** Make a file readable/writable by its owner only (chmod 600). */
inline void restrictFile(const QString &filePath)
{
    if (QFile::exists(filePath))
        QFile::setPermissions(filePath, QFile::ReadOwner | QFile::WriteOwner);
}
} // namespace SecretStore

#endif // SECRETSTORE_H
