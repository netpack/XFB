#ifndef QRCODE_H
#define QRCODE_H

#include <QByteArray>
#include <vector>

/**
 * @brief A small, self-contained QR encoder.
 *
 * XFB ships through six packaging channels by hand, so pulling in libqrencode
 * would mean a new dependency in the PKGBUILD, the Debian control file, the
 * Homebrew cask and both Windows builds. This encoder exists to avoid that.
 *
 * It is deliberately narrow rather than general: byte mode, error correction
 * level M, versions 1 to 10 (up to 213 bytes). That covers XFB's pairing URI
 * several times over, and everything outside it returns an invalid matrix
 * rather than silently producing a code that will not scan.
 */
namespace QrCode {

struct Matrix
{
    int size = 0;                 ///< side length in modules
    std::vector<bool> modules;    ///< row-major; true is dark

    bool isValid() const { return size > 0; }
    bool at(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= size || y >= size)
            return false;
        return modules[std::size_t(y) * std::size_t(size) + std::size_t(x)];
    }
};

/**
 * Encodes @a data as a QR symbol. Returns an invalid Matrix when the payload
 * does not fit in version 10 at error correction level M.
 */
Matrix encode(const QByteArray &data);

} // namespace QrCode

#endif // QRCODE_H
