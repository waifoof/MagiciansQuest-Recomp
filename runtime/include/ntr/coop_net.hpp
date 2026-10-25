// Co-op over the internet (docs/COOP_INTERNET.md, design D: a stand-in console on each PC).
//
// The game's own radio exchange cannot cross the internet (every turn waits for the other console). So each PC runs,
// beside the player's copy of the game, a hidden stand-in copy for the far player, linked to the player's copy over
// the same-PC link where the exchange takes microseconds. Between the PCs only game data travels:
// - the game's packets: the stand-in hears what its partner (the player's copy on its PC) sends in the session
//   (taps) and the player's copy hands them to the far PC, which puts them into its own session as if its stand-in had
//   sent them (and drops what its stand-in sends itself). Movement goes newest-wins; every other packet reliably and
//   in order.
// - the side channel (ntr/coop.hpp: the town sync, the players' messages), between the two players' copies.
// - each player's save, once, so the far PC's stand-in starts as that player (its student, its town), and the state
//   of each player (connecting as host or guest, in a session), which its stand-in follows.
//
// Call configure_net before the runtime is created (with configure_lan). Thread-safe unless noted.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ntr {

struct NetConfig {
    bool internet = false;     // a player's copy, over the internet (needs a LAN mode: Host or Join)
    bool standin = false;      // this process is a stand-in (started by a player's copy)
    uint16_t parent_port = 0;  // stand-in: the player's copy's port on this PC (UDP, 127.0.0.1)
};
void configure_net(const NetConfig& config);
const NetConfig& net_config();

// One of the game's session packets: a message (the session's receive; 16 bytes, or as long as the game sent it) or,
// with kNetShared in aid, a console's 16 bytes of a frame's DataSharing step (movement and actions: the same bytes
// again every frame until the next packet).
constexpr uint8_t kNetShared = 0x80;
struct NetPacket {
    uint8_t aid = 0;
    uint8_t len = 0;
    uint8_t data[128] = {};
};

// What a player's copy tells the far PC about itself (its stand-in there follows it).
struct NetLocal {
    uint8_t role = 0;        // connecting or connected as: 0 none, 1 host, 2 guest
    bool in_session = false; // in the game's session (connected)
    uint8_t standin = 0;     // this PC's stand-in (NetStandin::State)
};
// ... and what it hears of the far PC.
struct NetFar {
    bool linked = false;     // heard from in the last seconds
    int rtt_ms = -1;         // the round trip (-1: not measured yet)
    uint8_t role = 0;
    bool in_session = false;
    uint8_t standin = 0;     // the far PC's stand-in (for us)
    int save_percent = -1;   // our save on its way there (100: arrived; -1 not started)
    int far_save_percent = -1;  // its save on its way here
};

// ---- a player's copy ----
void net_set_local(const NetLocal& local);
NetFar net_far();
// Our save (and the DS clock to start the far stand-in at, "YYYY-MM-DDTHH:MM:SS"), sent to each far PC once it is
// heard from (again to a PC that restarted).
void net_offer_save(std::vector<uint8_t> save, const std::string& clock);
// The far player's save, once it has all arrived (true once per save received).
bool net_take_far_save(std::vector<uint8_t>& save, std::string& clock);
// The far player's packets, for the game's thread to put into the session. Events (messages, and shared bytes other
// than movement) in order: peek, then pop once it went in. Movement (shared bytes of types 0x0E-0x12): the newest one
// not yet taken.
bool net_peek_event(NetPacket& out);
void net_pop_event();
bool net_take_move(NetPacket& out);
void net_drop_far_packets();  // (not in a session: what was waiting belongs to none)
// Counts, for the log and the window's title.
struct NetCounts {
    uint64_t taps = 0, events_out = 0, moves_out = 0, events_in = 0, moves_in = 0, moves_stale = 0;
};
NetCounts net_counts();

// The stand-in on this PC: the port it reports to (127.0.0.1; 0 if none could be opened), its state, and what it
// should do (repeated until it says it heard).
struct NetStandin {
    enum State : uint8_t { None = 0, Booting = 1, Home = 2, Connecting = 3, InSession = 4, Leaving = 5 };
    bool alive = false;      // heard from in the last seconds
    uint8_t state = None;
    uint32_t frame = 0;
};
uint16_t net_standin_port();
NetStandin net_standin();
enum class NetCommand : uint8_t { None = 0, Host = 1, Join = 2, Leave = 3, Quit = 4 };
void net_standin_command(NetCommand command);

// ---- a stand-in ----
void net_tap(const NetPacket& packet);           // a packet its partner sent in the session, for the far PC
NetCommand net_parent_command();                 // what the player's copy asks
void net_report(uint8_t state, uint32_t frame);  // (NetStandin::State)
bool net_parent_gone();                          // nothing heard from the player's copy for a while

}  // namespace ntr
