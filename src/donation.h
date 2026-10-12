#ifndef DONATION_H
#define DONATION_H

/**
 * Where support for XFB goes.
 *
 * One constant per destination, in one place, because they are offered from
 * several: the desk's corner notice (ui/DonationNotice.cpp), the remote
 * control's web page (services/RemoteControlPage.cpp) and the About box. Two
 * copies of a link is how a station ends up following one that was changed in
 * only one of them.
 *
 * Both are public pages rather than a payment address on purpose: the remote
 * control page is served to whoever can reach the station, and nothing
 * personal should ride along on it. (This used to be a PayPal hosted button
 * for the same reason; Patreon replaced it on 2026-10-12.)
 */
namespace Donation {

inline constexpr const char *kPatreonUrl = "https://www.patreon.com/netpack";
inline constexpr const char *kGitHubSponsorsUrl = "https://github.com/sponsors/netpack";

} // namespace Donation

#endif // DONATION_H
