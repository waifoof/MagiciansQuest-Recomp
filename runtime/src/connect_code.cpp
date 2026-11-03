// Connect codes (connect_code.hpp).
#include "connect_code.hpp"

#include <cctype>
#include <cstdio>

namespace ntr {

namespace {

constexpr char kAlphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";  // (Crockford's base 32)
constexpr uint8_t kVersion = 1;
constexpr int kChars = 13;  // 64 bits, 5 to a letter

uint8_t check(const uint8_t* b, int n) {  // CRC-8 (polynomial 0x07), started at 0x5A so a code of zeros is not one
    uint8_t c = 0x5A;
    for (int i = 0; i < n; i++) {
        c ^= b[i];
        for (int k = 0; k < 8; k++) c = uint8_t(c & 0x80 ? (c << 1) ^ 0x07 : c << 1);
    }
    return c;
}

int value_of(char ch) {
    ch = char(std::toupper(static_cast<unsigned char>(ch)));
    if (ch == 'O') ch = '0';
    if (ch == 'I' || ch == 'L') ch = '1';
    for (int i = 0; i < 32; i++)
        if (kAlphabet[i] == ch) return i;
    return -1;
}

}  // namespace

std::string encode_connect_code(const ConnectCode& code) {
    uint8_t b[8];
    b[0] = uint8_t(kVersion << 4 | (code.open ? 1 : 0) | (code.symmetric ? 2 : 0) | (code.local ? 4 : 0));
    b[1] = uint8_t(code.ip >> 24), b[2] = uint8_t(code.ip >> 16), b[3] = uint8_t(code.ip >> 8), b[4] = uint8_t(code.ip);
    b[5] = uint8_t(code.port >> 8), b[6] = uint8_t(code.port);
    b[7] = check(b, 7);
    uint64_t v = 0;
    for (uint8_t x : b) v = v << 8 | x;
    std::string out;
    for (int i = kChars - 1; i >= 0; i--) {  // (the top letter holds the 64th bit and four zeros)
        out += kAlphabet[(i * 5 >= 64) ? 0 : (v >> (i * 5)) & 31];
        if (i == 9 || i == 5) out += '-';
    }
    return out;
}

bool decode_connect_code(const std::string& text, ConnectCode& out) {
    uint64_t v = 0;
    int n = 0;
    for (char ch : text) {
        if (ch == '-' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        const int x = value_of(ch);
        if (x < 0 || ++n > kChars) return false;
        if (n == 1) {
            if (x > 15) return false;  // (more than 64 bits)
        }
        v = v << 5 | uint64_t(x);
    }
    if (n != kChars) return false;
    uint8_t b[8];
    for (int i = 7; i >= 0; i--, v >>= 8) b[i] = uint8_t(v);
    if (b[0] >> 4 != kVersion || check(b, 7) != b[7]) return false;
    out.open = b[0] & 1;
    out.symmetric = b[0] & 2;
    out.local = b[0] & 4;
    out.ip = uint32_t(b[1]) << 24 | uint32_t(b[2]) << 16 | uint32_t(b[3]) << 8 | b[4];
    out.port = uint16_t(b[5] << 8 | b[6]);
    return out.ip != 0 && out.port != 0;
}

std::string connect_code_address(const ConnectCode& code) {
    char t[32];
    std::snprintf(t, sizeof t, "%u.%u.%u.%u:%u", code.ip >> 24, (code.ip >> 16) & 255, (code.ip >> 8) & 255, code.ip & 255,
                  unsigned(code.port));
    return t;
}

}  // namespace ntr
