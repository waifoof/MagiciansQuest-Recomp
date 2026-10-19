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
};
LanStatus lan_status();

}  // namespace ntr
