// Co-op between PCs: connect codes (docs/COOP_INTERNET.md, "Connect codes"). A PC's address on the internet as its
// router shows it (learned from public STUN servers on the co-op socket itself), with what is known of the way in,
// written as 13 letters and digits to copy, paste or type: "K3F9-2QXA-7MNPQ" (Crockford's base 32: no I, L, O or U;
// read back case-blind, with I and L as 1, O as 0, and any dashes or spaces ignored). 8 bytes: the version and flags,
// the IPv4 address, the port, a check byte.
#pragma once

#include <cstdint>
#include <string>

namespace ntr {

struct ConnectCode {
    uint32_t ip = 0;      // IPv4, host byte order
    uint16_t port = 0;
    bool open = false;    // its port is open on its router (UPnP / NAT-PMP, or forwarded): reachable without punching
    bool symmetric = false;  // its router gives a new port for each address (a direct connection is unlikely)
    bool local = false;   // a local network address (no internet address was learned)
};

std::string encode_connect_code(const ConnectCode& code);
// A code (as above), false if it is not one (a wrong letter, a typo caught by the check byte, another version).
bool decode_connect_code(const std::string& text, ConnectCode& out);
// "a.b.c.d:port"
std::string connect_code_address(const ConnectCode& code);

}  // namespace ntr
