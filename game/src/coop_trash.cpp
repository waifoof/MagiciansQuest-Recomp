// Magician's Quest co-op: the dorm's trash can in a session, decided by the host (docs/COOP.md, "The trash can";
// docs/COOP_GAME.md, item 9).
//
// What the game does. Event 225 block 1 (touching the trash can) begins "if t212 == 1 go to <999:1021 This action
// cannot be executed while using wireless communications>" (flag byte 212: the console's slot + 1 while connected, so
// only guests are refused), then query 0x54 (ov028 0x02116CA4) opens ov017's TRASH CAN screen, which moves items
// between the student's bag (its own record, +0x636) and a list in the *town* block (+0x644A, 15 items of 2 bytes,
// 0xFFFF for none). In a session the town block is the host's, kept the same on every console (coop_sync.cpp), but a
// guest's changes outside the variable store are its own ("kept here"): lifted as it was, a guest's thrown item stayed
// in its copy only, and the host's thrown items could be taken out of it again (a duplicate).
//
// What is built (only in a session; message type 49):
// - On every console 225.1's test is made to end the event (its jump goes to the end, always) and our event 90050
//   runs instead: it asks for the trash can (the mark kTrashAsk, seen at our command 0x38 hook), waits until the
//   answer is in t195 (1 yours, 2 not now), opens the game's own screen (query 0x54) and marks its end (kTrashDone).
// - The host decides who uses it: one student at a time. A guest's ask goes to the host, which says yes when no one
//   else uses it, with its list; the guest writes that list into its copy before the screen opens. When the guest's
//   screen closes it sends its list as it now is, and the host writes it into its town (the town sync brings it to
//   everyone). The host's own ask is answered on its console the same way. Meanwhile the other student is told
//   "Someone else is using the trash can right now."
// - A use that never ends (the guest gone) is let go after five minutes; nothing is kept between sessions.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

namespace content = mq::content;

constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the session)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+0x2254: flag bytes; +0x23BC start kind)
constexpr uint32_t kEventNote = 0x020B8AAC;      // +2 the running event's block, +6 its file
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kEventBusy = 0x0201DF58;      // (2): an event runs or is queued
constexpr uint32_t kTown = 0x020AF020, kTownSize = 0x993C;
constexpr uint32_t kTrashList = 0x644A, kTrashSize = 30;  // the town's trash can: 15 items (u16, 0xFFFF none)
constexpr uint32_t kChecksum = 0x02011D9C;       // (block, size)
constexpr uint32_t kSlotFlag = 10212;            // t212: the console's slot + 1 while connected
constexpr int kHold = 5 * 60 * 60;               // a use let go after this many frames
constexpr int kWaitAnswer = 5 * 60;              // a guest waits this long for the host's answer

constexpr uint8_t kMsgType = 49;  // (40 talk, 41 new day, 42 lessons, 43 mysteries, 44 incantations, 46 Gateway,
                                  // 47 insects, 48 the auditorium)
enum Op : uint8_t {
    kAsk = 1,     // guest -> host: u8 its slot: may it use the trash can?
    kAnswer = 2,  // host -> that guest: u8 yes, the list (kTrashSize bytes)
    kDone = 3,    // guest -> host: u8 its slot, its screen closed; the list as it now is
};

struct State {
    int holder = -1;   // host: the slot using the trash can (-1 no one)
    int since = 0;     // ... from this frame
    int asked = -1;    // guest: the frame it asked (-1 not waiting)
    int pending = -1;  // the frame 225.1 was ended for ours (-1 none)
    bool using_it = false;  // this console's screen is (about to be) open
    int frames = 0;
    bool listening = false;
} st;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0, uint32_t a1 = 0, uint32_t a2 = 0, uint32_t a3 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0, c->r[1] = a1, c->r[2] = a2, c->r[3] = a3;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

bool in_session(NtrCpu* c) {
    const uint32_t game = ntr_rd32(c, kGame);
    return cfie::coop_anytime && ntr::multiplayer_active() && ram(game) && ram(ntr_rd32(c, game + 0xA28));
}

void set_flag(NtrCpu* c, uint32_t var, uint8_t value) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + (var - 10000), value);
}
uint8_t flag(NtrCpu* c, uint32_t var) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (var - 10000)) : 0;
}

int event_file(NtrCpu* c) { return int(int16_t(ntr_rd16(c, kEventNote + 6))); }
int event_block(NtrCpu* c) { return int(int16_t(ntr_rd16(c, kEventNote + 2))); }

void read_list(NtrCpu* c, uint8_t* out) {
    for (uint32_t i = 0; i < kTrashSize; i++) out[i] = uint8_t(ntr_rd8(c, kTown + kTrashList + i));
}
void write_list(NtrCpu* c, const uint8_t* in) {
    for (uint32_t i = 0; i < kTrashSize; i++) ntr_wr8(c, kTown + kTrashList + i, in[i]);
    call(c, kChecksum, kTown, kTownSize);
}

void send(const std::vector<uint8_t>& m, uint32_t to = 0) { ntr::coop_send(kMsgType, m.data(), m.size(), to); }

// The host: may this slot use it now?
bool host_grant(int slot) {
    if (st.holder >= 0 && st.holder != slot && st.frames - st.since < kHold) return false;
    st.holder = slot, st.since = st.frames;
    return true;
}

void answer(NtrCpu* c, bool yes) {
    set_flag(c, content::kTrashAnswer, yes ? 1 : 2);
    st.using_it = yes;
    st.asked = -1;
}

void on_message(NtrCpu* c, uint32_t from, const uint8_t* d, size_t len) {
    if (len < 1) return;
    if (d[0] == kAsk && len >= 2 && coop_sync::hosting()) {
        const int slot = d[1];
        const bool yes = slot > 0 && host_grant(slot);
        std::vector<uint8_t> m = {kAnswer, uint8_t(yes ? 1 : 0)};
        uint8_t list[kTrashSize];
        read_list(c, list);
        m.insert(m.end(), list, list + kTrashSize);
        send(m, from);
        std::fprintf(stderr, "coop trash: slot %d asks for the trash can: %s\n", slot, yes ? "yes" : "no, in use");
    } else if (d[0] == kAnswer && len >= 2 + kTrashSize && !coop_sync::hosting()) {
        if (st.asked < 0) return;  // (an answer to an ask given up)
        if (d[1]) write_list(c, d + 2);
        answer(c, d[1] != 0);
        std::fprintf(stderr, "coop trash: the host says %s\n", d[1] ? "yes: the host's list in our town" : "no, in use");
    } else if (d[0] == kDone && len >= 2 + kTrashSize && coop_sync::hosting()) {
        const int slot = d[1];
        if (slot != st.holder) {
            std::fprintf(stderr, "coop trash: slot %d's list not taken (slot %d uses it)\n", slot, st.holder);
            return;
        }
        write_list(c, d + 2);
        st.holder = -1;
        std::fprintf(stderr, "coop trash: slot %d is done: its list is the town's\n", slot);
    }
}

}  // namespace

// Every event command 0x38 (if-variable-goto), before it runs (from mq_coop_gate, ov028 0x02112E20; r0 the command).
void mq_trash_command(NtrCpu* c, uint32_t cmd) {
    if (ntr_rd8(c, cmd) != 0x38 || !in_session(c) || std::getenv("MQ_COOP_TRASH_GAME")) return;
    const int32_t value = int32_t(ntr_rd32(c, cmd + 8));
    if (ntr_rd32(c, cmd + 4) == content::kQueryResult && value == content::kTrashAsk) {
        set_flag(c, content::kTrashAnswer, 0);
        if (coop_sync::hosting()) {
            const bool yes = host_grant(coop_sync::own_slot());
            answer(c, yes);
            std::fprintf(stderr, "coop trash: the host uses the trash can: %s\n", yes ? "yes" : "no, a guest uses it");
        } else {
            st.asked = st.frames;
            send({kAsk, uint8_t(coop_sync::own_slot())});
            std::fprintf(stderr, "coop trash: asked the host for the trash can\n");
        }
        return;
    }
    if (ntr_rd32(c, cmd + 4) == content::kQueryResult && value == content::kTrashDone) {
        if (!st.using_it) return;
        st.using_it = false;
        if (coop_sync::hosting()) {
            if (st.holder == coop_sync::own_slot()) st.holder = -1;
            std::fprintf(stderr, "coop trash: the host is done\n");
        } else {
            std::vector<uint8_t> m = {kDone, uint8_t(coop_sync::own_slot())};
            uint8_t list[kTrashSize];
            read_list(c, list);
            m.insert(m.end(), list, list + kTrashSize);
            send(m);
            std::fprintf(stderr, "coop trash: done: our list to the host\n");
        }
        return;
    }
    // 225.1's refusal test ("if t212 == 1 go to #3"): made to go to the event's end (#4) always, and ours queued.
    if (ntr_rd32(c, cmd + 4) == kSlotFlag && ntr_rd8(c, cmd + 2) == 0 && value == 1 && event_file(c) == 225 &&
        event_block(c) == 1 && ntr_rd32(c, cmd + 12) == 3) {
        ntr_wr8(c, cmd + 2, 1);          // !=
        ntr_wr32(c, cmd + 8, 0x7FFF);    // a value the flag never has: holds
        ntr_wr32(c, cmd + 12, 4);        // the end
        st.pending = st.frames;          // (ours is queued once 225 has ended: mq_trash_frame)
        std::fprintf(stderr, "coop trash: the trash can (225.1): our event instead\n");
    }
}

// The end of a frame (mq_frame_end).
void mq_trash_frame(NtrCpu* c) {
    st.frames++;
    if (!st.listening) {
        coop_sync::listen(kMsgType, on_message);
        st.listening = true;
    }
    if (!in_session(c)) {
        st.holder = -1, st.asked = -1, st.using_it = false, st.pending = -1;
        return;
    }
    if (st.pending >= 0) {  // (225.1 has ended: ours)
        const uint32_t ctx = ntr_rd32(c, kScriptContext);
        const int id = content::kEventTrash;
        if (st.frames - st.pending > 120) {
            st.pending = -1;
            std::fprintf(stderr, "coop trash: our event could not be queued\n");
        } else if (ram(ctx) && ntr_rd8(c, ctx + 8) == 0 && call(c, kEventBusy, 2) == 0 &&
                   call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) != 0) {
            ntr_wr8(c, ctx + 0x23BC, 2);
            st.pending = -1;
        }
    }
    if (st.asked >= 0 && st.frames - st.asked > kWaitAnswer) {  // (no answer: not now)
        answer(c, false);
        std::fprintf(stderr, "coop trash: no answer from the host\n");
    }
    (void)flag;
}
