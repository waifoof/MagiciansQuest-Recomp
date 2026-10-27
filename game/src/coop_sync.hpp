// Magician's Quest co-op: the town kept the same on every console of a session (game/src/coop_sync.cpp), and the
// game's side of the co-op side channel (ntr/coop.hpp) for other syncs built on it.
#pragma once

#include <cstddef>
#include <cstdint>

#include "ntr/recomp.h"

namespace coop_sync {

// Message types on the side channel: 1-15 are the town sync's own; other syncs take theirs from kFirstFree.
constexpr uint8_t kFirstFree = 16;

// A handler for one message type. Called in the game's thread at the end of a frame (mq_frame_end), only while this
// console stands in the host's town in a session, in the order each sender sent its messages.
using Handler = void (*)(NtrCpu* c, uint32_t from, const uint8_t* data, size_t len);
void listen(uint8_t type, Handler handler);

// While in the host's town in a session: this console's session slot (-1 otherwise), and whether it hosts.
int own_slot();
bool hosting();

// The host's alone (kHostVars in coop_sync.cpp: the week's mystery and its stages, the festivals' done flags, the
// homeroom's transfer): whether a global variable is one, and which bits of the variable store's byte i hold them.
bool host_var(uint32_t var);
uint8_t host_var_bits(uint32_t store_byte);

}  // namespace coop_sync

// From the hooks in cfie_hooks.cpp: the variable setter (main 0x0201D9F8) and the store write (main 0x0201E070) are
// about to run; the town sync tells the guest's own actions from its rolls by them.
void mq_coop_sync_set(NtrCpu* c);
void mq_coop_sync_store(NtrCpu* c);
// A byte of the town from the host while a guest's own safe deposit is open in its place (game/src/coop_trash.cpp):
// true if it was put aside with the host's counts instead of written.
bool mq_safedep_hold(NtrCpu* c, uint32_t at, uint8_t v);
// Whether this console decides the NPC with this id in its field (game/src/coop_npc.cpp): 1 yes, 0 no, -1 not known yet.
int mq_npc_decides(NtrCpu* c, uint8_t id);
