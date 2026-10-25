// Co-op over the internet (docs/COOP_INTERNET.md): what coop_net.cpp needs of the PC-to-PC link (lan_wireless.cpp).
// In that mode the UDP link does not carry the game's radio frames; it carries our own datagrams (kind Net) beside
// the side channel's (kind Coop), and the game's radio talks to a stand-in copy on this PC over the same-PC link.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ntr {

class NetPort {
public:
    virtual ~NetPort() = default;
    virtual uint32_t net_node() const = 0;
    // To one PC (or every other, 0); at most kNetDatagram bytes.
    virtual void net_send(uint32_t to, const uint8_t* data, size_t len) = 0;
    virtual bool net_recv(uint32_t& from, std::vector<uint8_t>& data) = 0;
    // The other PCs heard from in the last few seconds (in the game's multiplayer or not).
    virtual void net_peers(std::vector<uint32_t>& out) = 0;
};
constexpr size_t kNetDatagram = 1300;

// The PC-to-PC link's NetPort (nullptr: no link configured, or its socket did not open).
NetPort* lan_net_port();

// coop_net.cpp: the link was asked for in internet mode (configure_net), so the game's radio uses the same-PC link.
bool net_internet_mode();

}  // namespace ntr
