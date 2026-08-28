#include "IcecastSource.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

namespace {

/** The `\r\n\r\n` that ends a header block, wherever the peer puts it. */
int headerEnd(const QByteArray &buffer)
{
    const int crlf = buffer.indexOf("\r\n\r\n");
    if (crlf >= 0)
        return crlf + 4;
    // Some servers (and a few proxies in front of them) answer with bare
    // newlines; refusing those would be pedantry with the station off air.
    const int lf = buffer.indexOf("\n\n");
    return lf >= 0 ? lf + 2 : -1;
}

/** Status code from "HTTP/1.0 200 OK" or "ICE/1.0 200 OK"; -1 if unparsable. */
int statusCodeOf(const QByteArray &buffer)
{
    int lineEnd = buffer.indexOf('\n');
    if (lineEnd < 0)
        lineEnd = buffer.size();
    const QByteArray line = buffer.left(lineEnd).trimmed();
    const QList<QByteArray> parts = line.split(' ');
    if (parts.size() < 2)
        return -1;
    bool ok = false;
    const int code = parts.at(1).toInt(&ok);
    return ok ? code : -1;
}

} // namespace

IcecastSource::IcecastSource(QObject *parent)
    : QObject(parent)
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_wantConnected)
            openSocket();
    });

    m_handshakeTimer = new QTimer(this);
    m_handshakeTimer->setSingleShot(true);
    connect(m_handshakeTimer, &QTimer::timeout, this, [this]() {
        if (!m_handshakeDone)
            fail(tr("the server never answered the source request"), true);
    });
}

IcecastSource::~IcecastSource()
{
    m_wantConnected = false;
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
    }
}

void IcecastSource::setConfig(const Config &config)
{
    m_config = config;
}

QString IcecastSource::normalisedMount() const
{
    QString mount = m_config.mount.trimmed();
    if (mount.isEmpty())
        mount = QStringLiteral("/stream");
    if (!mount.startsWith(QLatin1Char('/')))
        mount.prepend(QLatin1Char('/'));
    return mount;
}

QByteArray IcecastSource::authorizationHeader() const
{
    const QString user = m_config.user.isEmpty() ? QStringLiteral("source")
                                                 : m_config.user;
    const QByteArray pair = (user + QLatin1Char(':') + m_config.password).toUtf8();
    return "Basic " + pair.toBase64();
}

void IcecastSource::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void IcecastSource::connectToServer()
{
    if (m_wantConnected)
        return;
    m_wantConnected = true;
    m_reconnectDelayMs = kFirstReconnectDelayMs;
    openSocket();
}

void IcecastSource::disconnectFromServer()
{
    if (!m_wantConnected && !m_socket)
        return;
    m_wantConnected = false;
    m_reconnectTimer->stop();
    m_handshakeTimer->stop();

    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_pending.clear();
    m_handshakeDone = false;
    m_lastError.clear();
    setState(State::Disconnected);
    emit disconnected(tr("disconnected by the operator"));
}

void IcecastSource::openSocket()
{
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    m_handshakeDone = false;
    m_responseBuffer.clear();
    m_bytesSent = 0;
    m_droppedBytes = 0;
    m_dropping = false;

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &IcecastSource::sendRequest);
    connect(m_socket, &QTcpSocket::readyRead, this, &IcecastSource::readHandshake);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        if (m_state == State::Disconnected)
            return;
        fail(m_handshakeDone ? tr("the server closed the connection")
                             : tr("the server closed the connection during the handshake"),
             true);
    });
    connect(m_socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
        if (!m_socket)
            return;
        fail(m_socket->errorString(), true);
    });

    setState(State::Connecting);
    emit logMessage(tr("Connecting to %1:%2%3...")
                        .arg(m_config.host)
                        .arg(m_config.port)
                        .arg(normalisedMount()));
    m_socket->connectToHost(m_config.host, m_config.port);
    m_handshakeTimer->start(kHandshakeTimeoutMs);
}

void IcecastSource::sendRequest()
{
    const QString mount = normalisedMount();
    QByteArray request;

    if (m_config.usePut) {
        // Icecast 2.4.1+ speaks plain HTTP here. Expect/100-continue is what
        // tells the server to admit the source before the body starts.
        request += "PUT " + mount.toUtf8() + " HTTP/1.1\r\n";
        request += "Host: " + m_config.host.toUtf8() + ':'
                + QByteArray::number(m_config.port) + "\r\n";
        request += "Expect: 100-continue\r\n";
    } else {
        request += "SOURCE " + mount.toUtf8() + " ICE/1.0\r\n";
    }

    request += "Authorization: " + authorizationHeader() + "\r\n";
    request += "User-Agent: XFB/"
            + QCoreApplication::applicationVersion().toUtf8() + " (libshout-compatible)\r\n";
    request += "Content-Type: " + m_config.contentType.toUtf8() + "\r\n";

    const auto addIce = [&request](const char *name, const QString &value) {
        if (value.trimmed().isEmpty())
            return;
        // Header values cannot carry newlines; a station name pasted from a
        // document sometimes does.
        QString clean = value;
        clean.replace(QLatin1Char('\r'), QLatin1Char(' '));
        clean.replace(QLatin1Char('\n'), QLatin1Char(' '));
        request += QByteArray(name) + ": " + clean.trimmed().toUtf8() + "\r\n";
    };
    addIce("ice-name", m_config.name);
    addIce("ice-genre", m_config.genre);
    addIce("ice-description", m_config.description);
    addIce("ice-url", m_config.url);
    request += QByteArray("ice-public: ") + (m_config.isPublic ? "1" : "0") + "\r\n";
    request += "ice-audio-info: bitrate=" + QByteArray::number(m_config.bitrateKbps)
            + ";channels=" + QByteArray::number(m_config.channels)
            + ";samplerate=" + QByteArray::number(m_config.sampleRate) + "\r\n";
    request += "\r\n";

    m_socket->write(request);
}

void IcecastSource::readHandshake()
{
    if (!m_socket)
        return;

    if (m_handshakeDone) {
        // Once audio is flowing the server has nothing to say that is good
        // news; log it, because it usually explains the drop that follows.
        const QByteArray extra = m_socket->readAll().trimmed();
        if (!extra.isEmpty())
            emit logMessage(tr("Server said: %1")
                                .arg(QString::fromUtf8(extra.left(200))));
        return;
    }

    m_responseBuffer += m_socket->readAll();
    if (m_responseBuffer.size() > 8192) {
        fail(tr("the server sent a nonsensical response"), true);
        return;
    }
    if (headerEnd(m_responseBuffer) < 0)
        return;   // headers not complete yet

    handleHandshakeResponse();
}

void IcecastSource::handleHandshakeResponse()
{
    const int code = statusCodeOf(m_responseBuffer);

    // 100 is the answer to Expect: 100-continue, 200 the answer to SOURCE.
    // Both mean "start sending".
    if (code != 100 && code != 200) {
        QString reason;
        switch (code) {
        case 401:
        case 403:
            reason = tr("the server rejected the user name or password");
            break;
        case 404:
            reason = tr("the server has no mount point %1").arg(normalisedMount());
            break;
        case 409:
            reason = tr("the mount point %1 is already in use by another source")
                         .arg(normalisedMount());
            break;
        case 500:
            reason = tr("the server reported an internal error");
            break;
        case -1:
            reason = tr("the server did not answer with a status line");
            break;
        default:
            reason = tr("the server refused the source connection (status %1)")
                         .arg(code);
            break;
        }
        // Bad credentials will not fix themselves, but a station is better
        // served by a client that keeps trying than by one that gives up
        // while somebody corrects the password on the server.
        fail(reason, true);
        return;
    }

    m_handshakeTimer->stop();
    m_handshakeDone = true;
    m_responseBuffer.clear();
    m_reconnectDelayMs = kFirstReconnectDelayMs;
    m_lastError.clear();
    setState(State::Connected);
    emit logMessage(tr("On air at %1:%2%3 (%4).")
                        .arg(m_config.host)
                        .arg(m_config.port)
                        .arg(normalisedMount(), m_config.contentType));
    emit connected();

    if (!m_pending.isEmpty()) {
        m_socket->write(m_pending);
        m_bytesSent += m_pending.size();
        m_pending.clear();
    }

    // A listener who tunes in after a reconnect should still see the title.
    if (!m_song.isEmpty())
        pushMetadata();
}

void IcecastSource::writeAudio(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    if (m_state == State::Connecting) {
        // Hold a little audio so the stream does not start with a hole, but
        // only a little: anything older than a second is not worth sending.
        if (m_pending.size() + data.size() <= kMaxHandshakeBuffer)
            m_pending += data;
        return;
    }

    if (m_state != State::Connected || !m_socket)
        return;

    if (m_socket->bytesToWrite() > kMaxPendingBytes) {
        m_droppedBytes += data.size();
        if (!m_dropping) {
            m_dropping = true;
            emit logMessage(tr("The server is not accepting audio fast enough; "
                               "dropping data."));
        }
        return;
    }

    if (m_dropping) {
        m_dropping = false;
        emit logMessage(tr("The server caught up (%1 kB was dropped).")
                            .arg(m_droppedBytes / 1024));
        m_droppedBytes = 0;
    }

    m_socket->write(data);
    m_bytesSent += data.size();
}

void IcecastSource::setMetadata(const QString &song)
{
    if (song == m_song)
        return;
    m_song = song;
    if (m_state == State::Connected)
        pushMetadata();
}

void IcecastSource::pushMetadata()
{
    if (m_song.isEmpty())
        return;

    // A separate, short-lived connection: the source connection's body is
    // audio from the first byte to the last and cannot carry a request.
    auto *meta = new QTcpSocket(this);
    const QString mount = normalisedMount();
    const QString song = m_song;
    const QString host = m_config.host;
    const quint16 port = m_config.port;
    const QByteArray auth = authorizationHeader();

    connect(meta, &QTcpSocket::connected, meta, [meta, mount, song, host, port, auth]() {
        const QByteArray path = "/admin/metadata?mode=updinfo&mount="
                + QUrl::toPercentEncoding(mount)
                + "&song=" + QUrl::toPercentEncoding(song);
        QByteArray request;
        request += "GET " + path + " HTTP/1.0\r\n";
        request += "Host: " + host.toUtf8() + ':' + QByteArray::number(port) + "\r\n";
        request += "Authorization: " + auth + "\r\n";
        request += "User-Agent: XFB\r\n";
        request += "Connection: close\r\n\r\n";
        meta->write(request);
    });

    connect(meta, &QTcpSocket::readyRead, this, [this, meta, song]() {
        const QByteArray response = meta->readAll();
        const int code = statusCodeOf(response);
        if (code == 200)
            emit logMessage(tr("Now playing sent to the server: %1").arg(song));
        else
            emit logMessage(tr("The server refused the now-playing update "
                               "(status %1).").arg(code));
        meta->close();
    });
    connect(meta, &QTcpSocket::errorOccurred, this,
            [this, meta](QAbstractSocket::SocketError) {
        emit logMessage(tr("Could not send the now-playing update: %1")
                            .arg(meta->errorString()));
        meta->deleteLater();
    });
    connect(meta, &QTcpSocket::disconnected, meta, &QObject::deleteLater);

    // Never let a stuck admin endpoint leak a socket per track.
    QTimer::singleShot(10000, meta, [meta]() {
        if (meta->state() != QAbstractSocket::UnconnectedState)
            meta->abort();
        meta->deleteLater();
    });

    meta->connectToHost(host, port);
}

void IcecastSource::fail(const QString &reason, bool retry)
{
    m_handshakeTimer->stop();

    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_handshakeDone = false;
    m_pending.clear();
    m_lastError = reason;

    const bool wasSomething = (m_state != State::Disconnected);
    setState(State::Disconnected);
    if (wasSomething) {
        emit logMessage(tr("Stream dropped: %1").arg(reason));
        emit disconnected(reason);
    }

    // RemoteHostClosedError arrives as both errorOccurred and disconnected;
    // without this the second one would restart the timer and double the
    // backoff for a single drop.
    if (retry && m_wantConnected && !m_reconnectTimer->isActive())
        scheduleReconnect();
}

void IcecastSource::scheduleReconnect()
{
    emit logMessage(tr("Reconnecting in %1 s.").arg(m_reconnectDelayMs / 1000));
    m_reconnectTimer->start(m_reconnectDelayMs);
    m_reconnectDelayMs = qMin(kMaxReconnectDelayMs, m_reconnectDelayMs * 2);
}
