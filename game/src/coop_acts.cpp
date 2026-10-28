// Magician's Quest co-op: the actions of a partner's student copy (docs/COOP.md, "The actions of the partner's copy").
//
// Each console shows every other student of a session by a remote player object (*(field+0x3E4C), 3 x 0x638) driven
// by the 16-byte packets of its console: a packet of type 0x13 sets the copy's action (+0xA2, main 0x0201E4DC, from
// main 0x02031404's 0x13 case; the sender, main 0x0202BBFC, sends only actions 0, 8 (also for 16), 12, 15, 23, 26,
// 27, 28 and 41), other packet types set others (0x1E talk: 2, 0x24 a tool: 4, 0x23: 30, 0x26: 17, 0x28: 33, ...),
// and the copy's own act function (main 0x020325E0, its class's vtable +0x64, once a frame while it stands in this
// console's map) runs the state function for that action: the local student's own (main 0x0202211C's) for 4 (the
// tools, main 0x02029C60: the net 0x02029F2C, ...), 8, 12, 13, 15, 22, 26, 30, 33, 41, copy-only ones for 2, 5, 17,
// 19, 31, 46, and nothing for the others (the rod's 11 only ends itself). Those functions test the student's +0xAA
// (1 = the local student) before what changes more than the student (packets sent, the bag, the variable store,
// messages), with three exceptions found (docs/COOP.md, "The actions of the partner's copy"): the net's sweep through
// the air runs the catch test for any student (coop_insects.cpp, remote_catch); action 8 (taking something, from
// standing) clears the field's catch view (+0x135FE) at its step 0 and, with the copy's bag full in areas 22-25, asks
// for the field's message and sets the copy's action 7; a student's standing animation (main 0x0202014C, unless flag
// bytes 33/34/214/215 say otherwise) writes its +0xA8 into the town's global store byte 74 (town+0x322), which on a
// guest would go to the host as the guest's own (not seen written in the tests: a write watch on it stayed quiet).
// This file wraps that act function:
// - In a session (always; MQ_COOP_ACTS_OFF: not): the guard below. MQ_COOP_ACTS_LOG: each byte it puts back.
// - MQ_ACT_AUDIT=<log file>: research. Main RAM is compared before and after each act of a copy (4 MB, slow), and each
//   changed run outside the copy's own object is written once per action episode, with the action and its step; the
//   local student's action changes are logged too (mq_acts_frame). docs/COOP.md has the table it gave.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <utility>
#include <vector>

#include "ntr/recomp.h"

namespace {

constexpr uint32_t kAct = 0x020325E0;
constexpr uint32_t kField = 0x020AD7EC, kLocal = 0x383C, kRemotes = 0x3E4C, kRemoteSize = 0x638;  // (+0x3E4C: a pointer)
constexpr uint32_t kGame = 0x020ACA58, kSession = 0xA28;
constexpr uint32_t sAction = 0xA2, sStep = 0xA5, sLocalFlag = 0xAA;
constexpr uint32_t kRam = 0x02000000, kRamSize = 0x400000;

bool in_act = false;
unsigned act_frame = 0;  // (the frame in_act was set: a thread switch inside the act can leave it set)

bool ram(uint32_t a) { return a >= kRam && a < kRam + kRamSize; }

bool in_session(NtrCpu* c) {
    const uint32_t g = ntr_rd32(c, kGame);
    return ram(g) && ntr_rd32(c, g + kSession) != 0;
}

// --- research: the audit ---
FILE* audit_log() {
    static FILE* f = [] {
        const char* p = std::getenv("MQ_ACT_AUDIT");
        return p && *p ? std::fopen(p, "a") : nullptr;
    }();
    return f;
}

std::vector<uint8_t> before;
unsigned frames = 0;
struct Episode {
    uint8_t action = 0xFF;
    std::set<uint32_t> seen;  // run starts already written in this episode
};
Episode episodes[3];

void snapshot(NtrCpu* c) {
    before.resize(kRamSize);
    for (uint32_t p = 0; p < kRamSize; p += 0x1000)
        if (const uint8_t* src = c->read_pages[(kRam + p) >> NTR_PAGE_SHIFT]) std::memcpy(&before[p], src, 0x1000);
}

void compare(NtrCpu* c, uint32_t o, int idx, uint32_t sp) {
    FILE* f = audit_log();
    const uint8_t action = ntr_rd8(c, o + sAction), step = ntr_rd8(c, o + sStep);
    Episode& ep = episodes[idx];
    if (action != ep.action) {
        ep.action = action;
        ep.seen.clear();
        std::fprintf(f, "%u copy %d (%08x) action %u step %u field %08x\n", frames, idx, o, action, step,
                     ntr_rd32(c, kField));
    }
    int lines = 0;
    for (uint32_t p = 0; p < kRamSize; p += 0x1000) {
        const uint8_t* now = c->read_pages[(kRam + p) >> NTR_PAGE_SHIFT];
        if (!now || std::memcmp(now, &before[p], 0x1000) == 0) continue;
        for (uint32_t i = 0; i < 0x1000; i++) {
            if (now[i] == before[p + i]) continue;
            uint32_t j = i, last = i;
            while (j < 0x1000 && j - last <= 8) {
                if (now[j] != before[p + j]) last = j;
                j++;
            }
            const uint32_t a = kRam + p + i, len = last - i + 1;
            i = last;
            if (a >= o && a < o + kRemoteSize) continue;
            if (a + len > sp - 0x4000 && a < sp) continue;  // (the act's own stack)
            if (!ep.seen.insert(a).second || lines >= 64) continue;
            lines++;
            std::fprintf(f, "%u   a%u s%u %08x +%u:", frames, action, step, a, len);
            for (uint32_t k = 0; k < std::min<uint32_t>(len, 12); k++)
                std::fprintf(f, " %02x>%02x", before[a - kRam + k], now[a - kRam + k]);
            std::fputc('\n', f);
        }
    }
}


// --- the guard: what a copy's act must not change on this console ---
// The town block (the host's, kept the same on every console: coop_sync.cpp), the local student's record, the field's
// catch view (+0x135FE) and its message request (+0x151BC..+0x151BF). Each is copied before the act and every byte
// the act changed is put back (and logged with MQ_COOP_ACTS_LOG).
constexpr uint32_t kTown = 0x020AF020, kTownSize = 0x993C, kTownTime = 0x2C8, kRecord = 0x020AD824, kRecordSize = 0x1858;
constexpr uint32_t kCatchView = 0x135FE, kMessage = 0x151BC, kMessageSize = 4;
struct Guarded {
    const char* name;
    uint32_t base, at, size;  // (base: what the logged offsets count from)
};
std::vector<uint8_t> guard_buf;
std::set<std::pair<uint32_t, uint16_t>> guard_logged;  // (address, action << 8 | step) already logged

bool guard_log() {
    static const bool on = std::getenv("MQ_COOP_ACTS_LOG") != nullptr;
    return on;
}

void copy_out(NtrCpu* c, uint32_t at, uint32_t size, uint8_t* dst) {
    while (size) {
        const uint32_t n = std::min<uint32_t>(size, 0x1000 - (at & 0xFFF));
        if (const uint8_t* p = c->read_pages[at >> NTR_PAGE_SHIFT]) std::memcpy(dst, p + (at & 0xFFF), n);
        else for (uint32_t k = 0; k < n; k++) dst[k] = uint8_t(ntr_rd8(c, at + k));
        at += n, dst += n, size -= n;
    }
}

int guarded(NtrCpu* c, uint32_t field, Guarded* g) {
    int n = 0;
    g[n++] = {"town", kTown, kTown, kTownTime};  // (not the town time object, +0x2C8: each console's own clock)
    g[n++] = {"town", kTown, kTown + kTownTime + 0x10, kTownSize - kTownTime - 0x10};
    g[n++] = {"the local student's record", kRecord, kRecord, kRecordSize};
    if (ram(field)) {
        g[n++] = {"field (catch view)", field, field + kCatchView, 1};
        g[n++] = {"field (message request)", field, field + kMessage, kMessageSize};
    }
    return n;
}

void guard_before(NtrCpu* c, uint32_t field) {
    Guarded g[5];
    const int n = guarded(c, field, g);
    uint32_t total = 0;
    for (int k = 0; k < n; k++) total += g[k].size;
    guard_buf.resize(total);
    uint32_t off = 0;
    for (int k = 0; k < n; k++) copy_out(c, g[k].at, g[k].size, &guard_buf[off]), off += g[k].size;
}

void guard_after(NtrCpu* c, uint32_t field, uint32_t o) {
    Guarded g[5];
    const int n = guarded(c, field, g);
    std::vector<uint8_t> now(guard_buf.size());
    uint32_t off = 0;
    for (int k = 0; k < n; k++) copy_out(c, g[k].at, g[k].size, &now[off]), off += g[k].size;
    if (now == guard_buf) return;
    const uint16_t act = uint16_t(ntr_rd8(c, o + sAction) << 8 | ntr_rd8(c, o + sStep));
    off = 0;
    for (int k = 0; k < n; k++) {
        for (uint32_t i = 0; i < g[k].size; i++) {
            if (now[off + i] == guard_buf[off + i]) continue;
            ntr_wr8(c, g[k].at + i, guard_buf[off + i]);
            if (guard_log() && guard_logged.insert({g[k].at + i, act}).second)
                std::fprintf(stderr,
                             "coop acts: the partner's copy (action %u step %u) wrote %s+0x%X (%u -> %u) here: undone\n",
                             act >> 8, act & 0xFF, g[k].name, g[k].at + i - g[k].base, guard_buf[off + i], now[off + i]);
        }
        off += g[k].size;
    }
}

}  // namespace

bool mq_in_remote_act() { return in_act && act_frame == frames; }

// main 0x020325E0 (remote player): a partner's student copy's act, its action's state function, once a frame. In a
// session it runs as the game has it, then what it changed of the town, the local student's record and the field's
// catch view and message request is put back: only the console of the student who acts decides those.
// MQ_COOP_ACTS_OFF: not guarded.
extern "C" int mq_remote_act(NtrCpu* c) {
    if (in_act && act_frame != frames) in_act = false;
    if (in_act || !in_session(c)) return 0;
    static const bool off = std::getenv("MQ_COOP_ACTS_OFF") != nullptr;
    const uint32_t o = c->r[0], field = ntr_rd32(c, kField);
    const uint32_t list = ram(field) ? ntr_rd32(c, field + kRemotes) : 0;
    int idx = -1;
    if (ram(list))
        for (int k = 0; k < 3; k++)
            if (o == list + uint32_t(k) * kRemoteSize) idx = k;
    FILE* f = audit_log();
    if (idx < 0 || (off && !f)) return 0;
    if (f) snapshot(c);
    if (!off) guard_before(c, field);
    const uint32_t sp = c->r[13];
    in_act = true, act_frame = frames;
    ntr_call(c, kAct | 1);
    in_act = false;
    if (f) {
        compare(c, o, idx, sp);
        std::fflush(f);
    }
    if (!off) guard_after(c, field, o);
    return 1;
}

// Once a frame (from mq_frame_end): research, the local student's action changes.
void mq_acts_frame(NtrCpu* c) {
    frames++;  // (also counts for the guard's in_act)
    in_act = false;  // (an act the game left by a thread switch never came back to the wrapper)
    FILE* f = audit_log();
    if (!f) return;
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return;
    const uint32_t s = field + kLocal;
    static uint16_t last = 0xFFFF;
    const uint16_t now = uint16_t(ntr_rd8(c, s + sAction) << 8 | ntr_rd8(c, s + sStep));
    if (now != last && ntr_rd8(c, s + sLocalFlag) == 1) {
        std::fprintf(f, "%u local action %u step %u\n", frames, now >> 8, now & 0xFF);
        std::fflush(f);
    }
    last = now;
}
