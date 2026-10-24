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

}  // namespace coop_sync

// From the hooks in cfie_hooks.cpp: the variable setter (main 0x0201D9F8) and the store write (main 0x0201E070) are
// about to run; the town sync tells the guest's own actions from its rolls by them.
void mq_coop_sync_set(NtrCpu* c);
void mq_coop_sync_store(NtrCpu* c);
