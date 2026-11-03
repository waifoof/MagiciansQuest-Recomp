// Local wireless between PCs (co-op over a LAN, or over the internet with the host's port forwarded): the DS Wi-Fi
// frames of the game's own multiplayer ("Multi-Card Play") travel as UDP datagrams. One PC hosts (it relays every
// frame to the others), the others join it by address. Without it, local wireless reaches other instances on the same
// PC only.
#pragma once

#include <cstdint>
#include <string>

namespace ntr {

struct LanConfig {
    enum class Mode { Off, Host, Join } mode = Mode::Off;
    std::string host;      // Join: the hosting PC's address (name or IPv4)
    uint16_t port = 49251; // the host's UDP port (Host: the one it listens on)
    bool port_mapping = false;  // Host: open the port on the router by itself (UPnP / NAT-PMP, portmap.hpp)
    // Learn this PC's address on the internet from public STUN servers, on this socket, and show it as a connect code
    // (connect_code.hpp). Join: `host` may be the host's code (then its router is reached by hole punching, and the
    // host's own network is searched too). Host: `peer` the guest's code (or address) to punch through to.
    bool stun = false;
    std::string peer;
};

// Call before the runtime is created (the console's MAC address depends on it). Not thread-safe.
void configure_lan(const LanConfig& config);

// What the link sees, for the frontend: the other players (instances or PCs) it hears from, and those in the game's
// multiplayer right now. Thread-safe.
struct LanStatus {
    bool active = false;   // a LAN link is configured and its socket open
    bool hosting = false;
    int peers = 0;         // other PCs heard from in the last few seconds
    int in_session = 0;    // of them, in the game's multiplayer
    std::string error;     // why the link could not open (empty: none)
    // The port opened on the router (Host with port_mapping): 0 not asked, 1 trying, 2 open, 3 failed; in words.
    int router = 0;
    std::string router_text;
    // Connect codes (stun): this PC's code (empty: not known yet), and why it may not work (a symmetric NAT, ...).
    std::string code;
    bool code_open = false;     // its port is open on the router: the other side need not give its code
    std::string code_note;
    int stun = 0;               // 0 not asked, 1 asking, 2 answered, 3 no answer (no internet, or UDP blocked)
    // Reaching the other PC by its code or address: 0 nothing to reach, 1 trying, 2 reached, 3 no answer yet.
    int punch = 0;
    std::string punch_text;
};

// The other PC's connect code (or address), while the game runs: a host punches through to it, a joining PC makes it
// its host. False (and why in *error) if it is neither a code nor an address.
bool lan_set_peer(const std::string& text, std::string* error = nullptr);
LanStatus lan_status();

// The program is ending: what the link holds on the router (an opened port) is given back. The link itself stays.
void lan_shutdown();

// The console's wireless is in the game's multiplayer mode right now (between melonDS's MP_Begin and MP_End: a
// Multi-Card Play session, hosting or visiting), whichever link carries it. Thread-safe.
bool multiplayer_active();

}  // namespace ntr
