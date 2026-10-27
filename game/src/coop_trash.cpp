// Magician's Quest co-op: the dorm's trash can and the school bell in a session, decided by the host (docs/COOP.md, "The trash can";
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
//
// The school bell (the Organ Man in the school's map 7, event 221.12) is here too: its refusal ("if t212 != 0 go to
// 32:8614", host and guest alike) is let through; the bell's melody is the town's (town block +0x8FB8, 68 bytes; Reset,
// query 0x58(-1), copies the default from main 0x020A3458), so what a guest's turn changes (Play music's tune, query
// 0x58(0), or Reset) goes to the host when the event ends, and the town sync brings it to everyone. Mystery 17's step
// at the organ (it moves the host's mystery) stays refused on a guest.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/lan.hpp"
#include "ntr/nitro.h"
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
constexpr uint32_t kBellTune = 0x8FB8, kBellSize = 68;  // the school bell's melody (main 0x02010608 resets it)
constexpr uint32_t kSlotFlag = 10212;            // t212: the console's slot + 1 while connected
constexpr int kHold = 5 * 60 * 60;               // a use let go after this many frames
constexpr int kWaitAnswer = 5 * 60;              // a guest waits this long for the host's answer

constexpr uint8_t kMsgType = 49;  // (40 talk, 41 new day, 42 lessons, 43 mysteries, 44 incantations, 46 Gateway,
                                  // 47 insects, 48 the auditorium)
enum Op : uint8_t {
    kAsk = 1,     // guest -> host: u8 its slot: may it use the trash can?
    kAnswer = 2,  // host -> that guest: u8 yes, the list (kTrashSize bytes)
    kDone = 3,    // guest -> host: u8 its slot, its screen closed; the list as it now is
    kBell = 4,    // guest -> host: the bell's melody after the guest's turn at the bell (kBellSize bytes)
};

struct State {
    int holder = -1;   // host: the slot using the trash can (-1 no one)
    int since = 0;     // ... from this frame
    int asked = -1;    // guest: the frame it asked (-1 not waiting)
    int pending = -1;  // the frame 225.1 was ended for ours (-1 none)
    bool bell = false;               // this console's student is at the bell (event 221.12)
    uint8_t tune[kBellSize] = {};    // ... the melody when it began
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
    } else if (d[0] == kBell && len >= 1 + kBellSize && coop_sync::hosting()) {
        for (uint32_t i = 0; i < kBellSize; i++) ntr_wr8(c, kTown + kBellTune + i, d[1 + i]);
        call(c, kChecksum, kTown, kTownSize);
        std::fprintf(stderr, "coop bell: a guest's melody is the town's\n");
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
    // The school bell (221.12, "if t212 != 0 go to #4", the refusal 32:8614): let through. Its melody (town +0x8FB8)
    // is the town's: what the student's turn changes (Play music's tune, query 0x58(0); Reset, 0x58(-1)) goes to the
    // host when the event ends (mq_trash_frame).
    if (ntr_rd32(c, cmd + 4) == kSlotFlag && ntr_rd8(c, cmd + 2) == 1 && value == 0 && event_file(c) == 221 &&
        event_block(c) == 12 && ntr_rd32(c, cmd + 12) == 4 && !std::getenv("MQ_COOP_BELL_GAME")) {
        // (mystery 17's step at the organ, g1000 = 17, g1001 = 1, g1010 = 1, moves the host's mystery (g1001 = 2) and
        // warps away: on a guest it stays refused, the host plays it)
        const uint32_t store = kTown + 0x2D8;
        if (!coop_sync::hosting() && ntr_rd8(c, store) == 17 && ntr_rd8(c, store + 1) == 1 && ntr_rd8(c, store + 10) == 1) {
            std::fprintf(stderr, "coop bell: mystery 17's step at the organ: the host's (refused here)\n");
            return;
        }
        ntr_wr8(c, cmd + 2, 0);          // ==
        ntr_wr32(c, cmd + 8, 0x7FFF);    // a value the flag never has
        st.bell = true;
        for (uint32_t i = 0; i < kBellSize; i++) st.tune[i] = uint8_t(ntr_rd8(c, kTown + kBellTune + i));
        std::fprintf(stderr, "coop bell: the bell (221.12) let through\n");
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
        st.holder = -1, st.asked = -1, st.using_it = false, st.pending = -1, st.bell = false;
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
    if (st.bell) {  // (the turn at the bell has ended: its melody to the host)
        const uint32_t ctx = ntr_rd32(c, kScriptContext);
        if (!ram(ctx) || ntr_rd8(c, ctx + 8) == 0 || event_file(c) != 221) {
            st.bell = false;
            uint8_t now[kBellSize];
            for (uint32_t i = 0; i < kBellSize; i++) now[i] = uint8_t(ntr_rd8(c, kTown + kBellTune + i));
            if (std::memcmp(now, st.tune, kBellSize) != 0) {
                if (!coop_sync::hosting()) {
                    std::vector<uint8_t> m = {kBell};
                    m.insert(m.end(), now, now + kBellSize);
                    send(m);
                }
                std::fprintf(stderr, "coop bell: the melody changed%s\n", coop_sync::hosting() ? "" : ": to the host");
            }
        }
    }
    if (st.asked >= 0 && st.frames - st.asked > kWaitAnswer) {  // (no answer: not now)
        answer(c, false);
        std::fprintf(stderr, "coop trash: no answer from the host\n");
    }
    (void)flag;
}

// ---------------------------------------------------------------------------------------------------------------------
// A guest's own safe deposit (the bank's George, "Use safe deposit"; docs/COOP.md, "The bank").
//
// The safe deposit keeps its items as counts in the *town* block: 4 bits per item slot (main 0x02010F84 reads,
// 0x020111CC writes; the slot is main 0x0203A024's for the item, 0-0x549), from town +0x8 (0x2A5 bytes, to +0x2AD;
// the byte +0x2AC also holds a flag bit, main 0x02010F24's bit 0x1527, so of that byte only its low half, slot 0x548,
// is the safe deposit's here). In a visit the town in RAM is the host's, so a guest's own safe deposit is not loaded:
// the game refuses it (menu 5808's "you can't use that service while visiting another town").
//
// What is built (a guest in a session only):
// - When the visit begins this console takes its own safe deposit's counts from its own save (the save memory's town
//   block, which is its own town as it was last saved: the save memory holds the town at +0x20 and the student's record
//   at +0x9960, each with its checksum at +0). Menu 5808's "Use safe deposit" then keeps its own target (5987: event
//   214.4, query 0x55, the game's SAFE DEPOSIT screen; mq_bank_target).
// - When query 0x55 runs, the counts in the host's town in RAM are put aside and the guest's own are put in their place;
//   the screen moves items between the guest's bag and them as alone. When event 214 has ended the counts there are
//   the guest's new ones, and the host's are put back. The town sync never sends these bytes (a guest's writes outside
//   the variable store are kept on the guest, coop_sync.cpp), and what the host sends for them meanwhile is put aside
//   with the host's (mq_safedep_hold), so the host's safe deposit is never touched.
// - The guest's new counts reach its save with its student: the bag and the safe deposit must not be saved apart (an
//   item in the bag and in the safe deposit is a copy, in neither a loss). The game saves the guest's record only (its
//   connect save and the visit's end, "Saving."; never its town, which is not in RAM; the card's path programs the
//   save a page at a time over some frames: the town +0x20-0x995C, the record +0x9960-0xB144, then the header
//   +0x10-0x20). So when the game's write completes the record in the save memory, the counts are written into the
//   save memory's town block in the same write (the runtime's backup write hook, before the save file is handed to
//   the frontend): one file write holds both. A console that drops
//   before that (the game closed, the PC off) has neither in its save; a session that ends without the save leaves the
//   counts written nowhere and they are dropped with the visit (the game's own end of such a visit gives the console
//   its student as last saved, docs/COOP_RELATIONSHIPS.md). At home the town is loaded from the save memory: the items
//   are in the guest's own safe deposit.
// `MQ_COOP_SAFEDEP_GAME=1`: refused, as the game has it. `MQ_SAFEDEP_LOG=1`: more lines.
namespace {

constexpr uint32_t kDepositAt = 0x8, kDepositSize = 0x2A5;  // town +0x8: the counts (+0x2AC: only its low half)
constexpr uint32_t kSaveTown = 0x20, kSaveRecord = 0x9960, kSaveRecordSize = 0x17E4;  // in the save memory
constexpr uint32_t kTownIdAt = 2, kTownIdSize = 6;

struct SafeDeposit {
    bool visiting = false;  // a guest in a session, its own counts taken
    bool leaving = false;   // ... the session is over, the visit's save and the load at home still to come
    bool swapped = false;   // its own counts in the host's town in RAM now
    bool dirty = false;     // its own counts changed since they were last written into the save memory
    bool home_due = false;  // written into the save memory: into the town in RAM too once it is our own again
    uint8_t own[kDepositSize] = {};
    uint8_t host[kDepositSize] = {};  // the host's, put aside while swapped
    uint8_t town_id[kTownIdSize] = {};
    int ended = 0;  // frames since event 214 ended (swapped)
} sd;

bool sd_log() {
    static const bool v = std::getenv("MQ_SAFEDEP_LOG") != nullptr;
    return v;
}

// The town block's checksum as main 0x02011D9C makes it (main 0x02092AFC: the one's complement sum of the halfwords,
// +0 taken as 0, complemented).
uint16_t town_checksum(const uint8_t* t) {
    uint32_t s = 0;
    for (uint32_t i = 2; i + 1 < kTownSize; i += 2) s += uint32_t(t[i]) | uint32_t(t[i + 1]) << 8;
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return uint16_t(~s);
}
bool save_town_ok(const uint8_t* save, uint32_t size) {
    if (!save || size < kSaveTown + kTownSize) return false;
    const uint8_t* t = save + kSaveTown;
    return town_checksum(t) == uint16_t(t[0] | t[1] << 8);
}

// The counts' bytes in a town (RAM or the save memory): all of the first kDepositSize - 1, the low half of the last.
uint8_t merge_last(uint8_t into, uint8_t from) { return uint8_t((into & 0xF0) | (from & 0x0F)); }

std::string changes(const uint8_t* before, const uint8_t* after) {
    std::string s;
    int n = 0;
    for (uint32_t i = 0; i < kDepositSize; i++)
        for (int h = 0; h < 2; h++) {
            if (i == kDepositSize - 1 && h == 1) continue;
            const int a = (before[i] >> (4 * h)) & 15, b = (after[i] >> (4 * h)) & 15;
            if (a == b) continue;
            if (n++ < 8) {
                char t[32];
                std::snprintf(t, sizeof t, " slot %u: %d -> %d", i * 2 + h, a, b);
                s += t;
            }
        }
    return n ? s : std::string(" none");
}

// The runtime's backup write hook: a write of the student's record carries the guest's own counts with it.
void sd_backup_written(NtrCpu*, uint32_t offset, uint32_t length, uint8_t* save, uint32_t size) {
    if (!(sd.visiting || sd.leaving) || !(sd.dirty || sd.home_due) || sd.swapped) return;
    // (the record's last page: the card's own path programs the save a page at a time, in order, over some frames,
    // then the header at +0x10; the counts go with the page that completes the record)
    const uint32_t record_end = kSaveRecord + kSaveRecordSize;
    if (offset >= record_end || offset + length < record_end) return;
    if (!save_town_ok(save, size)) {
        std::fprintf(stderr, "coop safe deposit: the save's town block does not check: our counts not written\n");
        return;
    }
    uint8_t* t = save + kSaveTown;
    uint8_t before[kDepositSize];
    std::memcpy(before, t + kDepositAt, kDepositSize);
    std::memcpy(t + kDepositAt, sd.own, kDepositSize - 1);
    t[kDepositAt + kDepositSize - 1] = merge_last(t[kDepositAt + kDepositSize - 1], sd.own[kDepositSize - 1]);
    const uint16_t ck = town_checksum(t);
    t[0] = uint8_t(ck), t[1] = uint8_t(ck >> 8);
    sd.dirty = false;
    sd.home_due = true;  // (and again with any later save, until the town in RAM has them: mq_safedep_frame)
    std::fprintf(stderr, "coop safe deposit: our own counts saved with our student (record write at %05x):%s\n", offset,
                 changes(before, t + kDepositAt).c_str());
}

void read_counts(NtrCpu* c, uint8_t* out) {
    for (uint32_t i = 0; i < kDepositSize; i++) out[i] = uint8_t(ntr_rd8(c, kTown + kDepositAt + i));
}
void write_counts(NtrCpu* c, const uint8_t* in) {
    for (uint32_t i = 0; i + 1 < kDepositSize; i++) ntr_wr8(c, kTown + kDepositAt + i, in[i]);
    const uint32_t last = kTown + kDepositAt + kDepositSize - 1;
    ntr_wr8(c, last, merge_last(uint8_t(ntr_rd8(c, last)), in[kDepositSize - 1]));
    call(c, kChecksum, kTown, kTownSize);
}
bool same_town_id(NtrCpu* c) {
    for (uint32_t i = 0; i < kTownIdSize; i++)
        if (ntr_rd8(c, kTown + kTownIdAt + i) != sd.town_id[i]) return false;
    return true;
}

// The host's counts back in RAM (the screen done, or the visit over while it was open).
void swap_back(NtrCpu* c, bool keep) {
    uint8_t now[kDepositSize];
    read_counts(c, now);
    const bool changed = std::memcmp(now, sd.own, kDepositSize - 1) != 0 ||
                         (now[kDepositSize - 1] & 0x0F) != (sd.own[kDepositSize - 1] & 0x0F);
    if (keep && changed) {
        std::fprintf(stderr, "coop safe deposit: our own safe deposit changed:%s (saved with our student)\n",
                     changes(sd.own, now).c_str());
        std::memcpy(sd.own, now, kDepositSize);
        sd.dirty = true;
    } else if (!keep && changed) {
        std::fprintf(stderr, "coop safe deposit: the visit ended with the screen open: what it changed is dropped:%s\n",
                     changes(sd.own, now).c_str());
    }
    if (same_town_id(c)) write_counts(c, sd.host);
    sd.swapped = false;
}

}  // namespace

// A guest may use its own safe deposit (its counts taken at the visit's start).
bool mq_safedep_ready(NtrCpu* c) {
    return in_session(c) && sd.visiting && !coop_sync::hosting() && !std::getenv("MQ_COOP_SAFEDEP_GAME");
}

// A query about to run (from mq_hall_query, in a session): on a guest, its own counts in for event 214.4 (Use safe
// deposit: its queries before 0x55, the SAFE DEPOSIT screen, read the counts too).
void mq_safedep_query(NtrCpu* c, int q) {
    if (!mq_safedep_ready(c) || event_file(c) != 214) return;
    if (sd_log()) std::fprintf(stderr, "coop safe deposit: query %02x in 214.%d\n", q, event_block(c));
    if (event_block(c) != 4 || sd.swapped) return;
    read_counts(c, sd.host);
    for (uint32_t i = 0; i < kTownIdSize; i++) sd.town_id[i] = uint8_t(ntr_rd8(c, kTown + kTownIdAt + i));
    write_counts(c, sd.own);
    sd.swapped = true;
    sd.ended = 0;
    std::fprintf(stderr, "coop safe deposit: the screen opens on our own safe deposit (the host's put aside)\n");
}

// A byte of the town from the host (coop_sync.cpp) while our own counts are in its place: put aside with the host's.
bool mq_safedep_hold(NtrCpu* c, uint32_t at, uint8_t v) {
    if (!sd.swapped || at < kDepositAt || at >= kDepositAt + kDepositSize) return false;
    sd.host[at - kDepositAt] = v;
    if (at == kDepositAt + kDepositSize - 1)  // (the flag bit's half is the host's: written)
        ntr_wr8(c, kTown + at, uint8_t((ntr_rd8(c, kTown + at) & 0x0F) | (v & 0xF0)));
    if (sd_log()) std::fprintf(stderr, "coop safe deposit: the host's +%x = %u put aside\n", at, v);
    return true;
}

// The end of a frame (from mq_trash_frame's caller).
void mq_safedep_frame(NtrCpu* c) {
    static bool hooked = false;
    if (!hooked) {
        ntr::nitro::set_backup_write_hook(&sd_backup_written);
        hooked = true;
    }
    if (!in_session(c)) {
        // The session is over: the game's own end of a visit then saves (the guest: its own town, which the game has
        // put back in RAM, and its student; our counts go with the record, sd_backup_written) and loads the field at
        // home. Once the field stands loaded outside a session the town in RAM is our own: our counts in it too (the
        // load may have taken the town from RAM, not from the save memory).
        if (sd.visiting) {
            if (sd.swapped) swap_back(c, false);
            sd.visiting = false;
            sd.leaving = true;
            std::fprintf(stderr, "coop safe deposit: the session ended%s\n",
                         sd.dirty ? ": our own safe deposit's changes go with the visit's save" : "");
        }
        if (sd.leaving) {
            const uint32_t field = ntr_rd32(c, 0x020AD7EC);
            const bool home = ram(field) && ntr_rd8(c, field + 0x3EAF) == 0 && ntr_rd8(c, field + 0x3EAE) == 0;
            if (home && sd.home_due) {
                uint8_t now[kDepositSize];
                read_counts(c, now);
                write_counts(c, sd.own);
                std::fprintf(stderr, "coop safe deposit: home: our own safe deposit in our town:%s\n", changes(now, sd.own).c_str());
                sd = SafeDeposit{};
            } else if (home) {
                if (sd.dirty)  // (no save of our student since the change: the game gave it back as last saved)
                    std::fprintf(stderr, "coop safe deposit: home without our student saved since the change: our own "
                                         "safe deposit as last saved (the change dropped with the visit)\n");
                sd = SafeDeposit{};
            }
        }
        return;
    }
    if (sd.leaving) {  // (a new session before the field stood at home: what was not saved is dropped)
        std::fprintf(stderr, "coop safe deposit: a new session%s\n", sd.dirty ? ": unsaved changes dropped" : "");
        sd = SafeDeposit{};
    }
    const int slot = coop_sync::own_slot();
    if (!sd.visiting && slot > 0) {
        uint32_t size = 0;
        const uint8_t* save = ntr::nitro::backup_memory(c, &size);
        if (save_town_ok(save, size)) {
            std::memcpy(sd.own, save + kSaveTown + kDepositAt, kDepositSize);
            sd.visiting = true;
            sd.dirty = false;
            int items = 0;
            for (uint32_t i = 0; i < kDepositSize; i++) items += (sd.own[i] & 15) + (i + 1 < kDepositSize ? sd.own[i] >> 4 : 0);
            std::fprintf(stderr, "coop safe deposit: our own safe deposit from our save (%d items)\n", items);
        } else if (st.frames % 600 == 0) {
            std::fprintf(stderr, "coop safe deposit: our save's town block does not check: our own safe deposit stays closed\n");
        }
    }
    if (sd.swapped) {
        const uint32_t ctx = ntr_rd32(c, kScriptContext);
        const bool on = ram(ctx) && ntr_rd8(c, ctx + 8) != 0 && event_file(c) == 214;
        if (on) sd.ended = 0;
        else if (++sd.ended >= 2) {
            swap_back(c, true);
            std::fprintf(stderr, "coop safe deposit: the screen closed: the host's counts back\n");
        }
    }
}
