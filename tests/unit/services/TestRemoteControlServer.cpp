#include <QtTest/QtTest>

#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>

#include "services/RemoteControlPage.h"
#include "services/RemoteControlServer.h"

/**
 * @brief The network remote control: keys, scopes, routing and its limits.
 *
 * Everything here is about who may do what, because getting any of it wrong
 * means a stranger on the station's network can take it off air:
 *
 *  - no route but hello answers without a key, and a wrong key is a 401;
 *  - a read key can never reach a command that changes anything;
 *  - arguments reach the handler exactly as sent, and only as a JSON object;
 *  - the key itself never lands in xfb.conf, and revoking one ends it;
 *  - an address guessing keys is locked out;
 *  - event subscribers are told the state at once and again when it changes.
 *
 * The server runs in this process on 127.0.0.1; the client is a plain
 * QTcpSocket driven from a local event loop, so both ends share the thread.
 */
class TestRemoteControlServer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void helloIsTheOnlyOpenRoute();
    void aWrongKeyIsRefused();
    void aReadKeyCannotControl();
    void argumentsReachTheHandler();
    void onlyJsonObjectsAreAccepted();
    void unknownPathsAndMethods();
    void theKeyIsNeverStored();
    void aRevokedKeyStopsWorking();
    void guessingKeysLocksTheAddressOut();
    void oversizedRequestsAreRefused();
    void subscribersHearTheStateAndItsChanges();
    void theControlPageIsServedAndSelfContained();
    void whoAmITellsThePageWhatItMayDo();
    void everyStringThePageAsksForExists();

private:
    struct Response {
        int status = 0;
        QByteArray head;
        QByteArray body;
        QJsonObject json() const { return QJsonDocument::fromJson(body).object(); }
    };

    Response request(const QByteArray &method, const QByteArray &path,
                     const QString &key = QString(), const QByteArray &body = QByteArray(),
                     const QByteArray &contentType = "application/json");
    Response raw(const QByteArray &bytes);

    RemoteControlServer *m_server = nullptr;
    QString m_lastCommand;
    QJsonObject m_lastArgs;
    int m_statusCounter = 0;
};

void TestRemoteControlServer::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("XFB-TestRemoteControl"));
    QCoreApplication::setApplicationName(QStringLiteral("XFB-TestRemoteControl"));
}

void TestRemoteControlServer::init()
{
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                  + QStringLiteral("/xfb.conf"));
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

    RemoteControlServer::setLocalOnlySetting(true);
    RemoteControlServer::setServeWebAppSetting(true);

    m_server = new RemoteControlServer(this);
    m_server->setCommandHandler([this](const QString &command, const QJsonObject &args) {
        if (command == QLatin1String("status")) {
            QJsonObject body;
            body.insert(QStringLiteral("counter"), m_statusCounter);
            return RemoteControlServer::Reply::ok(body);
        }
        m_lastCommand = command;
        m_lastArgs = args;
        QJsonObject body;
        body.insert(QStringLiteral("command"), command);
        return RemoteControlServer::Reply::ok(body);
    });

    // An ephemeral-looking port, retried: another run or a real XFB may hold one.
    bool listening = false;
    for (int attempt = 0; attempt < 20 && !listening; ++attempt) {
        RemoteControlServer::setPortSetting(quint16(20000 + QRandomGenerator::global()->bounded(40000)));
        listening = m_server->start();
    }
    QVERIFY(listening);
    m_lastCommand.clear();
    m_lastArgs = QJsonObject();
    m_statusCounter = 0;
}

void TestRemoteControlServer::cleanup()
{
    delete m_server;
    m_server = nullptr;
}

TestRemoteControlServer::Response TestRemoteControlServer::raw(const QByteArray &bytes)
{
    QTcpSocket socket;
    QByteArray received;
    QEventLoop loop;
    connect(&socket, &QTcpSocket::connected, &loop, [&]() { socket.write(bytes); });
    connect(&socket, &QTcpSocket::readyRead, &loop, [&]() { received += socket.readAll(); });
    connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    connect(&socket, &QTcpSocket::errorOccurred, &loop, [&]() {
        received += socket.readAll();
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, m_server->port());
    loop.exec();

    Response response;
    const int split = received.indexOf("\r\n\r\n");
    response.head = received.left(split);
    response.body = split >= 0 ? received.mid(split + 4) : QByteArray();
    const QList<QByteArray> statusLine = response.head.split('\n').value(0).split(' ');
    response.status = statusLine.value(1).toInt();
    return response;
}

TestRemoteControlServer::Response TestRemoteControlServer::request(
    const QByteArray &method, const QByteArray &path, const QString &key,
    const QByteArray &body, const QByteArray &contentType)
{
    QByteArray bytes = method + ' ' + path + " HTTP/1.1\r\nHost: localhost\r\n";
    if (!key.isEmpty())
        bytes += "Authorization: Bearer " + key.toUtf8() + "\r\n";
    if (!body.isEmpty()) {
        bytes += "Content-Type: " + contentType + "\r\n";
        bytes += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    }
    bytes += "\r\n" + body;
    return raw(bytes);
}

void TestRemoteControlServer::helloIsTheOnlyOpenRoute()
{
    const Response hello = request("GET", "/api/v1/hello");
    QCOMPARE(hello.status, 200);
    QCOMPARE(hello.json().value(QStringLiteral("api")).toInt(), RemoteControlServer::apiVersion());

    for (const RemoteControlServer::Route &route : RemoteControlServer::routes()) {
        if (route.command == QLatin1String("events"))
            continue; // a 401 is still a normal answer there, checked below
        const Response refused = request(route.method, route.path.toUtf8());
        QVERIFY2(refused.status == 401, qPrintable(route.path));
    }
    QVERIFY(m_lastCommand.isEmpty());
    QCOMPARE(request("GET", "/api/v1/events").status, 401);
}

void TestRemoteControlServer::aWrongKeyIsRefused()
{
    m_server->createKey(QStringLiteral("desk"), RemoteControlServer::Scope::Control);
    QCOMPARE(request("GET", "/api/v1/status", QStringLiteral("xfb_notTheKey")).status, 401);
    QVERIFY(m_lastCommand.isEmpty());
}

void TestRemoteControlServer::aReadKeyCannotControl()
{
    const QString reader = m_server->createKey(QStringLiteral("wall"), RemoteControlServer::Scope::Read);
    const QString controller = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);

    QCOMPARE(request("GET", "/api/v1/status", reader).status, 200);

    for (const RemoteControlServer::Route &route : RemoteControlServer::routes()) {
        if (route.scope != RemoteControlServer::Scope::Control)
            continue;
        QVERIFY2(request(route.method, route.path.toUtf8(), reader).status == 403,
                 qPrintable(route.path));
    }
    QVERIFY(m_lastCommand.isEmpty());

    const Response played = request("POST", "/api/v1/transport/play", controller);
    QCOMPARE(played.status, 200);
    QCOMPARE(m_lastCommand, QStringLiteral("transport.play"));
}

void TestRemoteControlServer::argumentsReachTheHandler()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);

    QCOMPARE(request("POST", "/api/v1/volume", key, R"({"volume": 42})").status, 200);
    QCOMPARE(m_lastCommand, QStringLiteral("volume.set"));
    QCOMPARE(m_lastArgs.value(QStringLiteral("volume")).toInt(), 42);

    QCOMPARE(request("GET", "/api/v1/library?q=Nina%20Simone&source=music&limit=5", key).status, 200);
    QCOMPARE(m_lastCommand, QStringLiteral("library.search"));
    QCOMPARE(m_lastArgs.value(QStringLiteral("q")).toString(), QStringLiteral("Nina Simone"));
    QCOMPARE(m_lastArgs.value(QStringLiteral("limit")).toString(), QStringLiteral("5"));

    QCOMPARE(request("GET", "/api/v1/library?q=a+b", key).status, 200);
    QCOMPARE(m_lastArgs.value(QStringLiteral("q")).toString(), QStringLiteral("a b"));

    // A trailing slash is the same endpoint.
    QCOMPARE(request("POST", "/api/v1/transport/next/", key).status, 200);
    QCOMPARE(m_lastCommand, QStringLiteral("transport.next"));
}

void TestRemoteControlServer::onlyJsonObjectsAreAccepted()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);

    QCOMPARE(request("POST", "/api/v1/volume", key, "volume=42",
                     "application/x-www-form-urlencoded").status, 415);
    QCOMPARE(request("POST", "/api/v1/volume", key, "{not json").status, 400);
    QCOMPARE(request("POST", "/api/v1/volume", key, "[42]").status, 400);
    QVERIFY(m_lastCommand.isEmpty());
}

void TestRemoteControlServer::unknownPathsAndMethods()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);

    QCOMPARE(request("GET", "/api/v1/nothing", key).status, 404);
    QCOMPARE(request("GET", "/../../etc/passwd", key).status, 404);
    QCOMPARE(request("GET", "/api/v1/transport/stop", key).status, 405);
    QCOMPARE(request("DELETE", "/api/v1/status", key).status, 405);
    // No CORS preflight to pass: a browser on another origin gets nothing.
    const Response options = request("OPTIONS", "/api/v1/transport/stop", key);
    QCOMPARE(options.status, 405);
    QVERIFY(!options.head.toLower().contains("access-control-allow"));
    QCOMPARE(raw("garbage\r\n\r\n").status, 400);
    QVERIFY(m_lastCommand.isEmpty());
}

void TestRemoteControlServer::theKeyIsNeverStored()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);
    QVERIFY(key.startsWith(QStringLiteral("xfb_")));
    QCOMPARE(key.size(), 44);

    QFile conf(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
               + QStringLiteral("/xfb.conf"));
    QVERIFY(conf.open(QIODevice::ReadOnly));
    const QByteArray stored = conf.readAll();
    QVERIFY(!stored.contains(key.toUtf8()));
    QVERIFY(!stored.contains(key.mid(12).toUtf8()));

    // And the digest is enough: a server made afresh from the file accepts it.
    const quint16 port = m_server->port();
    delete m_server;
    m_server = new RemoteControlServer(this);
    m_server->setCommandHandler([](const QString &, const QJsonObject &) {
        return RemoteControlServer::Reply::ok();
    });
    RemoteControlServer::setPortSetting(port);
    QVERIFY(m_server->start());
    QCOMPARE(m_server->keys().size(), 1);
    QCOMPARE(m_server->keys().first().scope, RemoteControlServer::Scope::Control);
    QCOMPARE(request("POST", "/api/v1/transport/stop", key).status, 200);
}

void TestRemoteControlServer::aRevokedKeyStopsWorking()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);
    QCOMPARE(request("GET", "/api/v1/status", key).status, 200);
    m_server->revokeKey(m_server->keys().first().id);
    QCOMPARE(request("GET", "/api/v1/status", key).status, 401);
}

void TestRemoteControlServer::guessingKeysLocksTheAddressOut()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);
    QSignalSpy blocked(m_server, &RemoteControlServer::addressBlocked);

    for (int i = 0; i < 9; ++i)
        QCOMPARE(request("GET", "/api/v1/status", QStringLiteral("xfb_guess%1").arg(i)).status, 401);
    QCOMPARE(request("GET", "/api/v1/status", key).status, 200);

    QCOMPARE(request("GET", "/api/v1/status", QStringLiteral("xfb_guess10")).status, 401);
    QCOMPARE(blocked.count(), 1);
    // Now even the right key, and even the open route, is refused from here.
    QCOMPARE(request("GET", "/api/v1/status", key).status, 429);
    QCOMPARE(request("GET", "/api/v1/hello").status, 429);
}

void TestRemoteControlServer::oversizedRequestsAreRefused()
{
    const QString key = m_server->createKey(QStringLiteral("deck"), RemoteControlServer::Scope::Control);

    QByteArray big = "{\"x\":\"" + QByteArray(70 * 1024, 'a') + "\"}";
    QCOMPARE(request("POST", "/api/v1/volume", key, big).status, 413);

    QByteArray headers = "GET /api/v1/status HTTP/1.1\r\nX-Filler: " + QByteArray(20 * 1024, 'b') + "\r\n\r\n";
    QCOMPARE(raw(headers).status, 413);

    QCOMPARE(raw("POST /api/v1/volume HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n").status, 501);
    QVERIFY(m_lastCommand.isEmpty());
}

void TestRemoteControlServer::subscribersHearTheStateAndItsChanges()
{
    const QString key = m_server->createKey(QStringLiteral("wall"), RemoteControlServer::Scope::Read);

    QTcpSocket socket;
    QByteArray received;
    QEventLoop loop;
    connect(&socket, &QTcpSocket::connected, &loop, [&]() {
        socket.write("GET /api/v1/events HTTP/1.1\r\nAuthorization: Bearer " + key.toUtf8() + "\r\n\r\n");
    });
    connect(&socket, &QTcpSocket::readyRead, &loop, [&]() {
        received += socket.readAll();
        if (received.count("event: status") >= 1)
            loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, m_server->port());
    loop.exec();

    QVERIFY(received.startsWith("HTTP/1.1 200"));
    QVERIFY(received.contains("text/event-stream"));
    QVERIFY(received.contains("data: {\"counter\":0,\"ok\":true}"));

    // A change is pushed without waiting for anything to ask.
    m_statusCounter = 7;
    m_server->notifyStateChanged();
    QTRY_VERIFY_WITH_TIMEOUT(received.contains("\"counter\":7"), 3000);
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
}

void TestRemoteControlServer::theControlPageIsServedAndSelfContained()
{
    // No key: this is the page a key is typed into.
    const Response page = request("GET", "/");
    QCOMPARE(page.status, 200);
    QVERIFY(page.head.contains("text/html"));

    // It must FETCH nothing from anywhere — no stylesheet, script, font or
    // image from off the machine. That is what lets the policy below be
    // default-src 'none', and what keeps a studio with no internet working.
    const QByteArray lower = page.body.toLower();
    QVERIFY(!lower.contains("<link"));
    QVERIFY(!lower.contains("src=\"http"));
    QVERIFY(!lower.contains("//cdn"));
    QVERIFY(!lower.contains("url(http"));
    QVERIFY(!lower.contains("@import"));

    // A link the reader may choose to follow is another matter, and there is
    // exactly one: the donation link, which must be an anchor and must go to
    // PayPal. Anything else remote in this page is a mistake.
    static const QRegularExpression remote(QStringLiteral("https?://[^\"'\\s]+"));
    auto found = remote.globalMatch(QString::fromUtf8(page.body));
    int links = 0;
    while (found.hasNext()) {
        const QString url = found.next().captured(0);
        QVERIFY2(url.startsWith(QStringLiteral("https://www.paypal.com/")), qPrintable(url));
        QVERIFY2(page.body.contains(("<a href=\"" + url + "\"").toUtf8()), qPrintable(url));
        ++links;
    }
    QCOMPARE(links, 1);
    const QByteArray headLower = page.head.toLower();
    QVERIFY(headLower.contains("content-security-policy: default-src 'none'"));
    QVERIFY(headLower.contains("frame-ancestors 'none'"));
    QVERIFY(headLower.contains("x-content-type-options: nosniff"));
    // It carries no key of its own, and no station data.
    QVERIFY(!page.body.contains("xfb_"));

    QCOMPARE(request("POST", "/").status, 405);

    // Switched off, its address is as unknown as any other.
    RemoteControlServer::setServeWebAppSetting(false);
    QCOMPARE(request("GET", "/").status, 404);
    QCOMPARE(request("GET", "/app").status, 404);
    RemoteControlServer::setServeWebAppSetting(true);
    QCOMPARE(request("GET", "/app").status, 200);
}

void TestRemoteControlServer::whoAmITellsThePageWhatItMayDo()
{
    const QString reader = m_server->createKey(QStringLiteral("Corridor display"),
                                               RemoteControlServer::Scope::Read);
    const QString controller = m_server->createKey(QStringLiteral("Studio deck"),
                                                   RemoteControlServer::Scope::Control);

    QCOMPARE(request("GET", "/api/v1/whoami").status, 401);

    const Response asReader = request("GET", "/api/v1/whoami", reader);
    QCOMPARE(asReader.status, 200);
    QCOMPARE(asReader.json().value(QStringLiteral("name")).toString(), QStringLiteral("Corridor display"));
    QCOMPARE(asReader.json().value(QStringLiteral("scope")).toString(), QStringLiteral("read"));

    const Response asController = request("GET", "/api/v1/whoami", controller);
    QCOMPARE(asController.json().value(QStringLiteral("scope")).toString(), QStringLiteral("control"));
    QCOMPARE(asController.json().value(QStringLiteral("name")).toString(), QStringLiteral("Studio deck"));

    // whoami is the server's own answer, so it never reaches the handler.
    QVERIFY(m_lastCommand.isEmpty());

    // The cover is a command like any other, and a read key may ask for it.
    QCOMPARE(request("GET", "/api/v1/artwork", reader).status, 200);
    QCOMPARE(m_lastCommand, QStringLiteral("artwork"));
}

// A missing string is invisible in C++ and shows up as a blank in the page —
// the key's scope read as nothing at all until this was caught by eye. Both
// halves of the page name their strings by key, so both can be checked.
void TestRemoteControlServer::everyStringThePageAsksForExists()
{
    const QMap<QString, QString> table = RemoteControlPage::strings();
    const QString page = RemoteControlPage::html();

    // Markup: no placeholder may survive into what is served.
    QVERIFY(!page.contains(QStringLiteral("{{t.")));

    // Script: every S.<key> it reads must be in the table it is handed.
    static const QRegularExpression used(QStringLiteral("\\bS\\.([A-Za-z_][A-Za-z0-9_]*)"));
    QStringList missing;
    auto it = used.globalMatch(page);
    while (it.hasNext()) {
        const QString key = it.next().captured(1);
        if (!table.contains(key) && !missing.contains(key))
            missing << key;
    }
    QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("no such string: ") + missing.join(QStringLiteral(", "))));

    // And every string in the table is actually asked for somewhere.
    for (auto row = table.cbegin(); row != table.cend(); ++row) {
        const bool inMarkupOrScript = page.contains(QStringLiteral("S.") + row.key())
                                      || page.contains(row.value());
        QVERIFY2(inMarkupOrScript, qPrintable(QStringLiteral("unused string: ") + row.key()));
    }
}

QTEST_GUILESS_MAIN(TestRemoteControlServer)
#include "TestRemoteControlServer.moc"
