#include "QrCode.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace QrCode {
namespace {

constexpr int kMinVersion = 1;
constexpr int kMaxVersion = 10;

/** Per version (1..10) at error correction level M. */
struct VersionSpec {
    int ecCodewordsPerBlock;
    int group1Blocks;
    int group1DataCodewords;
    int group2Blocks;
    int group2DataCodewords;
};

// Straight from the standard's block table, level M. The consistency check
// that matters: group data plus ec times blocks equals the version's total
// codeword count, which qrTotalCodewords() below reproduces from geometry.
constexpr std::array<VersionSpec, 10> kSpecs = {{
    { 10, 1, 16, 0,  0 },   // 1
    { 16, 1, 28, 0,  0 },   // 2
    { 26, 1, 44, 0,  0 },   // 3
    { 18, 2, 32, 0,  0 },   // 4
    { 24, 2, 43, 0,  0 },   // 5
    { 16, 4, 27, 0,  0 },   // 6
    { 18, 4, 31, 0,  0 },   // 7
    { 22, 2, 38, 2, 39 },   // 8
    { 22, 3, 36, 2, 37 },   // 9
    { 26, 4, 43, 1, 44 },   // 10
}};

/** Alignment pattern centre coordinates, versions 1..10. */
const std::array<std::vector<int>, 10> kAlignment = {{
    {},                 // 1 has none
    { 6, 18 },
    { 6, 22 },
    { 6, 26 },
    { 6, 30 },
    { 6, 34 },
    { 6, 22, 38 },
    { 6, 24, 42 },
    { 6, 26, 46 },
    { 6, 28, 50 },
}};

const VersionSpec &specFor(int version) { return kSpecs[std::size_t(version - 1)]; }

int dataCodewordsFor(int version)
{
    const VersionSpec &spec = specFor(version);
    return spec.group1Blocks * spec.group1DataCodewords
         + spec.group2Blocks * spec.group2DataCodewords;
}

int blockCountFor(int version)
{
    const VersionSpec &spec = specFor(version);
    return spec.group1Blocks + spec.group2Blocks;
}

// ---------------------------------------------------------------- GF(256)

struct GaloisField
{
    std::array<quint8, 512> exp{};
    std::array<quint8, 256> log{};

    GaloisField()
    {
        int x = 1;
        for (int i = 0; i < 255; ++i) {
            exp[std::size_t(i)] = quint8(x);
            log[std::size_t(x)] = quint8(i);
            x <<= 1;
            if (x & 0x100)
                x ^= 0x11D;         // the primitive polynomial QR uses
        }
        for (int i = 255; i < 512; ++i)
            exp[std::size_t(i)] = exp[std::size_t(i - 255)];
    }

    quint8 multiply(quint8 a, quint8 b) const
    {
        if (a == 0 || b == 0)
            return 0;
        return exp[std::size_t(log[a]) + std::size_t(log[b])];
    }
};

const GaloisField &field()
{
    static const GaloisField instance;
    return instance;
}

/** Reed-Solomon generator polynomial of the given degree. */
std::vector<quint8> generatorPolynomial(int degree)
{
    std::vector<quint8> result{1};
    for (int i = 0; i < degree; ++i) {
        // multiply by (x - alpha^i)
        std::vector<quint8> next(result.size() + 1, 0);
        for (std::size_t j = 0; j < result.size(); ++j) {
            next[j] ^= result[j];
            next[j + 1] ^= field().multiply(result[j], field().exp[std::size_t(i)]);
        }
        result = next;
    }
    return result;
}

std::vector<quint8> reedSolomon(const std::vector<quint8> &data, int ecCount)
{
    const std::vector<quint8> generator = generatorPolynomial(ecCount);
    std::vector<quint8> remainder(std::size_t(ecCount), 0);

    for (quint8 byte : data) {
        const quint8 factor = byte ^ remainder.front();
        remainder.erase(remainder.begin());
        remainder.push_back(0);
        for (std::size_t i = 0; i < remainder.size(); ++i)
            remainder[i] ^= field().multiply(generator[i + 1], factor);
    }
    return remainder;
}

// ------------------------------------------------------------- BCH codes

/**
 * Polynomial division in GF(2). @a degree is the generator's degree, which is
 * where the leading term to cancel sits — not the number of data bits, which
 * is only how many terms there are to cancel.
 */
int bchRemainder(int value, int generator, int dataBits, int degree)
{
    int result = value;
    for (int bit = dataBits - 1; bit >= 0; --bit) {
        if (result & (1 << (bit + degree)))
            result ^= generator << bit;
    }
    return result;
}

/** 15-bit format information for level M and the given mask. */
int formatBits(int mask)
{
    const int data = (0x00 << 3) | mask;          // level M is 0b00
    const int bch = bchRemainder(data << 10, 0x537, 5, 10);
    return ((data << 10) | bch) ^ 0x5412;
}

/** 18-bit version information, versions 7 and up. */
int versionBits(int version)
{
    const int bch = bchRemainder(version << 12, 0x1F25, 6, 12);
    return (version << 12) | bch;
}

// ------------------------------------------------------------ the canvas

struct Canvas
{
    int size = 0;
    std::vector<bool> dark;
    std::vector<bool> reserved;   ///< function patterns; data never lands here

    explicit Canvas(int version)
        : size(17 + 4 * version)
        , dark(std::size_t(size * size), false)
        , reserved(std::size_t(size * size), false)
    {}

    std::size_t index(int x, int y) const
    {
        return std::size_t(y) * std::size_t(size) + std::size_t(x);
    }
    void set(int x, int y, bool value, bool isFunction)
    {
        dark[index(x, y)] = value;
        if (isFunction)
            reserved[index(x, y)] = true;
    }
    bool isReserved(int x, int y) const { return reserved[index(x, y)]; }
    bool at(int x, int y) const { return dark[index(x, y)]; }
};

void drawFinder(Canvas &canvas, int cx, int cy)
{
    // The finder plus its separator, clipped at the symbol edge.
    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            const int x = cx + dx;
            const int y = cy + dy;
            if (x < 0 || y < 0 || x >= canvas.size || y >= canvas.size)
                continue;
            const int distance = std::max(std::abs(dx), std::abs(dy));
            canvas.set(x, y, distance != 2 && distance <= 3, true);
        }
    }
}

void drawAlignment(Canvas &canvas, int cx, int cy)
{
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int distance = std::max(std::abs(dx), std::abs(dy));
            canvas.set(cx + dx, cy + dy, distance != 1, true);
        }
    }
}

void drawFunctionPatterns(Canvas &canvas, int version)
{
    const int last = canvas.size - 1;

    drawFinder(canvas, 3, 3);
    drawFinder(canvas, last - 3, 3);
    drawFinder(canvas, 3, last - 3);

    // Timing patterns.
    for (int i = 8; i < last - 7; ++i) {
        canvas.set(i, 6, i % 2 == 0, true);
        canvas.set(6, i, i % 2 == 0, true);
    }

    // Alignment patterns, skipping the three that collide with finders.
    const std::vector<int> &centres = kAlignment[std::size_t(version - 1)];
    for (int cy : centres) {
        for (int cx : centres) {
            const bool topLeft     = (cx == 6 && cy == 6);
            const bool topRight    = (cx == last - 6 && cy == 6);
            const bool bottomLeft  = (cx == 6 && cy == last - 6);
            if (topLeft || topRight || bottomLeft)
                continue;
            drawAlignment(canvas, cx, cy);
        }
    }

    // Reserve the format information areas, and the always-dark module.
    for (int i = 0; i < 9; ++i) {
        if (i != 6) {
            canvas.set(i, 8, false, true);
            canvas.set(8, i, false, true);
        }
    }
    for (int i = 0; i < 8; ++i) {
        canvas.set(last - i, 8, false, true);
        canvas.set(8, last - i, false, true);
    }
    canvas.set(8, last - 7, true, true);   // the dark module

    // Version information blocks, versions 7 and up.
    if (version >= 7) {
        const int bits = versionBits(version);
        for (int i = 0; i < 18; ++i) {
            const bool on = (bits >> i) & 1;
            const int a = i / 3;
            const int b = i % 3;
            canvas.set(a, last - 10 + b, on, true);
            canvas.set(last - 10 + b, a, on, true);
        }
    }
}

void drawFormat(Canvas &canvas, int mask)
{
    const int bits = formatBits(mask);
    const int last = canvas.size - 1;

    for (int i = 0; i < 15; ++i) {
        const bool on = (bits >> i) & 1;

        // Copy beside the top-left finder.
        if (i < 6)
            canvas.set(8, i, on, true);
        else if (i == 6)
            canvas.set(8, 7, on, true);
        else if (i == 7)
            canvas.set(8, 8, on, true);
        else if (i == 8)
            canvas.set(7, 8, on, true);
        else
            canvas.set(14 - i, 8, on, true);

        // The redundant copy: bits 0-6 run down beside the bottom-left finder,
        // bits 7-14 run along the top-right one.
        if (i < 7)
            canvas.set(8, last - i, on, true);
        else
            canvas.set(last - 14 + i, 8, on, true);
    }
}

/** Walks the symbol in the standard two-column zigzag, laying down data bits. */
void placeData(Canvas &canvas, const std::vector<quint8> &codewords)
{
    std::size_t bitIndex = 0;
    const std::size_t totalBits = codewords.size() * 8;
    bool upward = true;

    for (int right = canvas.size - 1; right >= 1; right -= 2) {
        if (right == 6)
            right = 5;   // column 6 is the vertical timing pattern

        for (int step = 0; step < canvas.size; ++step) {
            const int y = upward ? (canvas.size - 1 - step) : step;
            for (int offset = 0; offset < 2; ++offset) {
                const int x = right - offset;
                if (canvas.isReserved(x, y))
                    continue;
                bool on = false;
                if (bitIndex < totalBits) {
                    const quint8 byte = codewords[bitIndex / 8];
                    on = (byte >> (7 - (bitIndex % 8))) & 1;
                    ++bitIndex;
                }
                canvas.set(x, y, on, false);
            }
        }
        upward = !upward;
    }
}

bool maskAt(int mask, int x, int y)
{
    switch (mask) {
    case 0: return (x + y) % 2 == 0;
    case 1: return y % 2 == 0;
    case 2: return x % 3 == 0;
    case 3: return (x + y) % 3 == 0;
    case 4: return (y / 2 + x / 3) % 2 == 0;
    case 5: return (x * y) % 2 + (x * y) % 3 == 0;
    case 6: return ((x * y) % 2 + (x * y) % 3) % 2 == 0;
    case 7: return ((x + y) % 2 + (x * y) % 3) % 2 == 0;
    default: return false;
    }
}

void applyMask(Canvas &canvas, int mask)
{
    for (int y = 0; y < canvas.size; ++y) {
        for (int x = 0; x < canvas.size; ++x) {
            if (canvas.isReserved(x, y))
                continue;
            if (maskAt(mask, x, y))
                canvas.dark[canvas.index(x, y)] = !canvas.dark[canvas.index(x, y)];
        }
    }
}

int penalty(const Canvas &canvas)
{
    const int n = canvas.size;
    int score = 0;

    // Rule 1: runs of five or more of the same colour.
    for (int pass = 0; pass < 2; ++pass) {
        for (int a = 0; a < n; ++a) {
            int runLength = 1;
            bool runColour = pass == 0 ? canvas.at(0, a) : canvas.at(a, 0);
            for (int b = 1; b < n; ++b) {
                const bool colour = pass == 0 ? canvas.at(b, a) : canvas.at(a, b);
                if (colour == runColour) {
                    ++runLength;
                } else {
                    if (runLength >= 5)
                        score += 3 + (runLength - 5);
                    runColour = colour;
                    runLength = 1;
                }
            }
            if (runLength >= 5)
                score += 3 + (runLength - 5);
        }
    }

    // Rule 2: any 2x2 block of one colour.
    for (int y = 0; y < n - 1; ++y) {
        for (int x = 0; x < n - 1; ++x) {
            const bool c = canvas.at(x, y);
            if (c == canvas.at(x + 1, y) && c == canvas.at(x, y + 1)
                && c == canvas.at(x + 1, y + 1)) {
                score += 3;
            }
        }
    }

    // Rule 3: finder-like 1:1:3:1:1 sequences with four light modules beside.
    const std::array<bool, 11> forward =
        { true, false, true, true, true, false, true, false, false, false, false };
    const std::array<bool, 11> backward =
        { false, false, false, false, true, false, true, true, true, false, true };
    for (int a = 0; a < n; ++a) {
        for (int b = 0; b + 11 <= n; ++b) {
            bool matchRowF = true, matchRowB = true, matchColF = true, matchColB = true;
            for (int k = 0; k < 11; ++k) {
                const bool row = canvas.at(b + k, a);
                const bool col = canvas.at(a, b + k);
                if (row != forward[std::size_t(k)])  matchRowF = false;
                if (row != backward[std::size_t(k)]) matchRowB = false;
                if (col != forward[std::size_t(k)])  matchColF = false;
                if (col != backward[std::size_t(k)]) matchColB = false;
            }
            if (matchRowF) score += 40;
            if (matchRowB) score += 40;
            if (matchColF) score += 40;
            if (matchColB) score += 40;
        }
    }

    // Rule 4: drift away from an even balance of dark and light.
    int darkCount = 0;
    for (bool module : canvas.dark)
        if (module) ++darkCount;
    const int total = n * n;
    const int percent = (darkCount * 100) / total;
    score += 10 * (std::abs(percent - 50) / 5);

    return score;
}

} // namespace

Matrix encode(const QByteArray &data)
{
    // Pick the smallest version whose capacity holds the payload. The character
    // count indicator grows from 8 to 16 bits at version 10, so the header is
    // sized per candidate rather than once.
    int version = 0;
    for (int candidate = kMinVersion; candidate <= kMaxVersion; ++candidate) {
        const int countBits = candidate < 10 ? 8 : 16;
        const int neededBits = 4 + countBits + data.size() * 8;
        if (neededBits <= dataCodewordsFor(candidate) * 8) {
            version = candidate;
            break;
        }
    }
    if (version == 0)
        return Matrix();   // too long for this encoder

    const VersionSpec &spec = specFor(version);
    const int totalDataCodewords = dataCodewordsFor(version);
    const int countBits = version < 10 ? 8 : 16;

    // --- the bit stream ---
    std::vector<bool> bits;
    bits.reserve(std::size_t(totalDataCodewords * 8));

    auto appendBits = [&bits](int value, int count) {
        for (int i = count - 1; i >= 0; --i)
            bits.push_back((value >> i) & 1);
    };

    appendBits(0b0100, 4);                  // byte mode
    appendBits(int(data.size()), countBits);
    for (char byte : data)
        appendBits(quint8(byte), 8);

    // Terminator, then pad to a byte boundary, then the alternating pad bytes.
    const int capacityBits = totalDataCodewords * 8;
    for (int i = 0; i < 4 && int(bits.size()) < capacityBits; ++i)
        bits.push_back(false);
    while (bits.size() % 8 != 0)
        bits.push_back(false);

    std::vector<quint8> dataCodewords;
    dataCodewords.reserve(std::size_t(totalDataCodewords));
    for (std::size_t i = 0; i < bits.size(); i += 8) {
        quint8 byte = 0;
        for (int b = 0; b < 8; ++b)
            byte = quint8((byte << 1) | (bits[i + std::size_t(b)] ? 1 : 0));
        dataCodewords.push_back(byte);
    }
    bool alternate = true;
    while (int(dataCodewords.size()) < totalDataCodewords) {
        dataCodewords.push_back(alternate ? 0xEC : 0x11);
        alternate = !alternate;
    }

    // --- split into blocks, compute error correction ---
    std::vector<std::vector<quint8>> blocks;
    std::vector<std::vector<quint8>> ecBlocks;
    blocks.reserve(std::size_t(blockCountFor(version)));

    std::size_t offset = 0;
    auto addBlocks = [&](int count, int size) {
        for (int i = 0; i < count; ++i) {
            std::vector<quint8> block(dataCodewords.begin() + long(offset),
                                      dataCodewords.begin() + long(offset) + size);
            offset += std::size_t(size);
            ecBlocks.push_back(reedSolomon(block, spec.ecCodewordsPerBlock));
            blocks.push_back(std::move(block));
        }
    };
    addBlocks(spec.group1Blocks, spec.group1DataCodewords);
    addBlocks(spec.group2Blocks, spec.group2DataCodewords);

    // --- interleave ---
    std::vector<quint8> finalCodewords;
    const int longestData = std::max(spec.group1DataCodewords, spec.group2DataCodewords);
    for (int i = 0; i < longestData; ++i)
        for (const std::vector<quint8> &block : blocks)
            if (i < int(block.size()))
                finalCodewords.push_back(block[std::size_t(i)]);
    for (int i = 0; i < spec.ecCodewordsPerBlock; ++i)
        for (const std::vector<quint8> &block : ecBlocks)
            finalCodewords.push_back(block[std::size_t(i)]);

    // --- lay out, then choose the mask that scores best ---
    Canvas base(version);
    drawFunctionPatterns(base, version);
    placeData(base, finalCodewords);

    int bestMask = 0;
    int bestScore = -1;
    Canvas best = base;
    for (int mask = 0; mask < 8; ++mask) {
        Canvas candidate = base;
        applyMask(candidate, mask);
        drawFormat(candidate, mask);
        const int score = penalty(candidate);
        if (bestScore < 0 || score < bestScore) {
            bestScore = score;
            bestMask = mask;
            best = candidate;
        }
    }
    Q_UNUSED(bestMask)

    Matrix matrix;
    matrix.size = best.size;
    matrix.modules = best.dark;
    return matrix;
}

} // namespace QrCode
