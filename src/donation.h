#ifndef DONATION_H
#define DONATION_H

/**
 * Where a donation to XFB goes.
 *
 * One constant in one place because it is now offered from two: the desk's
 * corner notice (ui/DonationNotice.cpp) and the remote control's web page
 * (services/RemoteControlPage.cpp). Two copies of a payment address is how a
 * station ends up donating to a link that was changed in only one of them.
 *
 * It is PayPal's *hosted button* rather than an e-mail address on purpose:
 * the page that shows it is served to whoever can reach the station, and a
 * hosted button keeps the address itself off it.
 */
namespace Donation {

inline constexpr const char *kPayPalUrl =
    "https://www.paypal.com/donate/?hosted_button_id=TFDSZU78WLMC6";

} // namespace Donation

#endif // DONATION_H
