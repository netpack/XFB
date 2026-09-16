#ifndef REMOTECONTROLPAGE_H
#define REMOTECONTROLPAGE_H

#include <QMap>
#include <QString>

/**
 * @brief The control page RemoteControlServer serves at "/": a small web
 *        application that drives the station through the same API, so that
 *        using the remote control needs no programming at all.
 *
 * Why it is served by XFB itself rather than being a separate project:
 *
 *  - **Same origin.** The API deliberately answers no CORS headers, so no page
 *    from anywhere else can read it. A page served by this very server is the
 *    one page that does not need permission — and that is the point: the
 *    absence of CORS stays absolute for everybody else.
 *  - **Nothing to install or host.** The presenter opens the station's address
 *    in a phone or a browser and signs in with a key. There is no build step,
 *    no CDN, no account, and nothing leaves the station's network.
 *  - **One contract.** The page uses the documented API and nothing private,
 *    so anything it can do, a script can do too — and a change to the API is
 *    visible here immediately rather than in a client nobody rebuilt.
 *
 * It is one self-contained document: its stylesheet and its script are inline
 * and it loads nothing from anywhere, which is what lets the server serve it
 * under a content policy of `default-src 'none'`.
 *
 * The strings live in strings() so they go through tr() like the rest of XFB:
 * the markup carries `{{t.key}}` placeholders, and the same table is handed to
 * the script as `S` for the text it builds at runtime.
 */
namespace RemoteControlPage {

/** Every translatable string of the page, keyed by the name the markup uses. */
QMap<QString, QString> strings();

/** The whole page, placeholders substituted and the table injected. */
QString html();

} // namespace RemoteControlPage

#endif // REMOTECONTROLPAGE_H
