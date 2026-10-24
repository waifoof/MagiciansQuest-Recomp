// The co-op side channel: messages of ours between the consoles of a Multi-Card Play session, beside the game's own
// wireless frames (which go on unchanged). The game's link carries one 16-byte packet per console per frame and its
// movement packet fills it every frame, so anything bigger (a block of the town's save, a student's record) travels
// here instead: reliable and in order from each sender, over whichever link the session uses (other PCs by UDP,
// include/ntr/lan.hpp; other copies on this PC by shared memory).
//
// Only consoles in the game's multiplayer right now (between melonDS's MP_Begin and MP_End) are reached; outside a
// session nothing is sent or kept. Call from one thread (the game's): coop_poll once a frame, coop_send any time.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ntr {

constexpr size_t kCoopMaxMessage = 0x10000;  // the biggest message coop_send takes

// A message to every other console in the session (to = 0), or to one (a node from coop_poll or coop_peers).
// False: no session, no such console, or too big.
bool coop_send(uint8_t type, const void* data, size_t len, uint32_t to = 0);

// Takes in what has arrived and keeps the link going (acks, resends). Each whole message is handed to fn, in the
// order its sender sent it.
using CoopReceive = void (*)(uint32_t from, uint8_t type, const uint8_t* data, size_t len, void* user);
void coop_poll(CoopReceive fn, void* user);

// This console's node (0: no link), and the other consoles in the session as of the last coop_poll.
uint32_t coop_node();
size_t coop_peers(uint32_t* out, size_t max);

}  // namespace ntr
