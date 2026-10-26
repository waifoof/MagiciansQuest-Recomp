// Magician's Quest co-op: the fish of a field are the same on every console of a session (docs/COOP.md, "Fish
// together").
//
// Each field has a fish manager (field+0xF87C, ov001): when the field is made (0x020D2748) it loads the place's kinds
// (fishtbl.dat, by the place, the month and the hour), rolls 32 spawn points by the water (0x020D1D4C, +0x2C80: 16
// bytes each, place, kind, state 1 free / 2 taken) and up to 8 fish at once (0x020D1EAC, +0x24, 0x1E4 bytes each:
// kind +0x122, 16 sizes +0xD0, the size picked +0x125, its point +0x127, place +0x1A0, home +0x1AC, a think state
// +0x1C8 with a count +0x1CA and a length +0x1CC, active +0x1DC), all with the RNG. A fish thinks only within 18 units
// of the student (0x020D3098): every 80-120 frames (40-60 in a session) it moves to a new spot within a unit of its
// home (0x020D2E18, the RNG) and, while a rod is out (+0x1E2), leaves a ripple where it was (0x020D2EA0, effect 27).
// The lure: the local student's rod gives the manager the lure's place (+0x2C6C) and its own (+0x2C70); each X that
// reels the lure in pulls a trigger (+0x1140, 0x020D29F0), and the manager update (0x020D24C4) counts the triggers
// that found a fish within 4 units of the lure (+0x2C74): at 2 the nearest fish becomes the "hooked" fish (+0xF44, a
// copy of its kind and sizes, put 0.75-1.25 units beside the lure) that swims to the lure, nibbles and bites (its
// state +0x1C8 / manager +0x110C, active +0x1DC / manager +0x1120); the slot it came from is kept (+0x2C7E). While the
// hooked fish is active the other fish stand still. The rod plays the bite and the reeling; a catch puts the fish in
// the bag (item from the kind, the size in the high bits, main 0x0204F880) and moves the slot's fish to another free
// point of its kind (0x020D2958). Left alone, each console rolled its own fish and the partner's lure was nothing.
//
// In a session (both consoles in the same field):
// - Every fish slot has an owner, whose console runs it as the game does; the other console's copy does not think (its
//   active flag is cleared while the manager update runs, so it is skipped there and found by no lure) and takes the
//   owner's record (kind, sizes, place, home, point, think state; the hooked fish's place while one is hooked from it).
//   The host owns them all, and its spawn points; a guest's own spawns are replaced by the host's.
// - The host's lure is the game's. A guest's lure finds the host's fish by the game's own rule (this file works it out
//   ahead of the update: the trigger pulled, the second count, the nearest fish within the reach): before the hooked
//   fish is made from a slot it does not own, the trigger is held back and the host is asked for that slot. The host
//   says yes when its own hooked fish did not come from it, and hands the slot over (the guest's console runs it from
//   then on); no otherwise (the guest leaves that fish alone for two seconds). On yes the guest's trigger is pulled
//   again with only that fish in reach, and the game makes the hooked fish. So a fish is hooked by one lure only.
// - The hooked fish is shown on the other console by its slot's copy, which swims to the hooking console's lure; the
//   ripples and splashes of either console's fish (0x020D2EA0) are made on the other console too. While anyone's lure
//   is in the water, the fish of both consoles make their ripples.
// - When the guest's hook is over (the catch moved the fish, or it swam off and the lure came out), the guest gives the
//   slot back with its record; the host takes the place, the point (its spawn points updated) and runs it again.
// In a field the host is not in (no record for 90 frames) a guest's fish are its own, as alone.
// MQ_COOP_FISH_OFF: none of this. MQ_COOP_FISH_LOG: what it does, on stderr.
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/recomp.h"

namespace {

constexpr uint8_t kMsgType = 50;  // (40 talk, 41 new day, 42 lessons, 43 mysteries, 44 incantations, 46 Gateway,
                                  // 47 insects, 48 the hall, 49 the trash can)
enum Op : uint8_t { kOpState = 1, kOpAsk = 2, kOpAnswer = 3, kOpRelease = 4, kOpFx = 5 };

// The game's addresses (USA, YNNE).
constexpr uint32_t kField = 0x020AD7EC;
constexpr uint32_t kManager = 0xF87C;  // field+: the fish manager
constexpr uint32_t kArea = 0x3EB4;     // field+: the map it shows
constexpr uint32_t kReach = 0x020FD424;  // ov001: how near the lure a fish must be (4 units)
constexpr uint32_t kShowAll = 0x020FE360;  // ov001: a flag: fish are thought and shown at any distance
// In the manager.
constexpr uint32_t mFish = 0x24, kFishSize = 0x1E4;
constexpr int kFish = 8;
constexpr uint32_t mHooked = 0xF44, mHookActive = 0x1120, mHookState = 0x110C, mTrigger = 0x1140, mLure = 0x2C6C,
                   mStudentPos = 0x2C70, mCount = 0x2C74, mSource = 0x2C7E, mHour = 0x2C60,
                   mPoints = 0x2C80;
constexpr int kPoints = 32;
// In a fish (and the hooked fish).
constexpr uint32_t fShown = 0xCC, fNear = 0xCD, fPoint = 0x127, fPos = 0x1A0, fLureStudent = 0x1C4,
                   fWatch = 0x1D0, fActive = 0x1DC, fRipple = 0x1E2;
// The parts of a fish that make it: [0xD0,0x128) sizes, kind, point; [0x144,0x180) its kind's three records;
// [0x1A0,0x1C4) place, home, a vector; [0x1C8,0x1D0) think state, count, length; [0x1D4,0x1DC); active; +0x1E0.
struct Range {
    uint32_t at, len;
};
constexpr Range kRanges[] = {{0xD0, 0x58}, {0x144, 0x3C}, {0x1A0, 0x24}, {0x1C8, 0x08}, {0x1D4, 0x08}, {0x1DC, 0x04},
                             {0x1E0, 0x02}};
constexpr size_t kRecordSize = 0x58 + 0x3C + 0x24 + 0x08 + 0x08 + 0x04 + 0x02;
// Functions.
constexpr uint32_t kUpdate = 0x020D24C4, kFx = 0x020D2EA0, kAnim = 0x0200CD04, kFishReset = 0x020D31AC,
                   kFishInit = 0x020D314C;

constexpr int kSendEvery = 6, kFullEvery = 60, kStale = 90, kAskTimeout = 90, kDenied = 120, kHoldMin = 60,
              kIdleRelease = 30;
constexpr int32_t kSnap = 4 << 12;

#pragma pack(push, 1)
struct Header {
    uint8_t op;          // kOpState
    uint8_t slot;        // the sender's session slot
    int8_t area;         // the map its field shows
    uint8_t hour;        // the manager's hour slot
    uint32_t field;      // the field's class (its vtable)
    uint32_t epoch;      // the host's: a number for this manager's fish (a new field, a new number)
    uint8_t owners;      // the host's: bit per slot the guest runs
    uint8_t count;       // records that follow
    uint8_t fishing;     // the sender's lure is in the water
    uint8_t points;      // 1: the spawn points follow the records
};
struct Record {
    uint8_t slot, flags;  // flags: 1 the place is a hooked fish's (from this slot)
    uint8_t data[kRecordSize];
};
struct Point {
    int32_t pos[3];
    int8_t kind;
    uint8_t state;
};
struct Ask {
    uint8_t op, slot, fish, pad;  // kOpAsk: the asking console's session slot, the fish's slot
    uint32_t epoch;
};
struct Answer {
    uint8_t op, fish, yes, pad;  // kOpAnswer
    uint32_t epoch;
};
struct Release {
    uint8_t op, pad[3];  // kOpRelease, then a Record
    uint32_t epoch;
    Record rec;
};
struct Fx {
    uint8_t op, fish, id, pad;  // kOpFx: the slot (the hooked fish's: the slot it came from), the effect
    uint32_t epoch;
    int32_t pos[3];
};
#pragma pack(pop)

struct Copy {
    Record rec{};        // the owner's latest
    bool has = false;
    int32_t target[3] = {};
    bool gliding = false;
    int seen = -100000;
};

struct State {
    uint32_t manager = 0;
    int frames = 0;
    uint32_t epoch = 0;          // host: its own; guest: the host's
    // who runs each slot here: true this console
    std::array<bool, kFish> mine{};
    std::array<Copy, kFish> copy{};
    // host
    std::array<bool, kFish> guest_owned{};
    std::array<int, kFish> guest_seen{};
    std::array<std::array<uint8_t, kRecordSize>, kFish> sent{};
    std::array<uint8_t, kFish> sent_flags{};
    std::array<bool, kFish> was_sent{};
    std::array<uint8_t, kPoints> points_sent{};
    // guest
    bool host_known = false;
    int host_seen = -100000;
    bool remote_fishing = false;
    int remote_fishing_seen = -100000;
    int asked = -1, asked_frame = 0;
    int force = -1;                          // granted: pull the trigger again with this fish alone in reach
    std::array<int, kFish> denied_until{};
    std::array<int, kFish> granted_frame{};
    std::array<int, kFish> idle{};           // frames the granted fish has not been hooked or near the lure
    std::vector<Answer> answers;
    // both
    std::array<uint32_t, kFish> held{};      // the active flags cleared for the update (restored after)
    std::array<bool, kFish> was_held{};
    std::array<uint16_t, kFish> ripple_held{};
    std::array<bool, kFish> ripple_set{};
    int last_source = -1;
    bool last_fishing = false;
} st;

bool in_update = false, in_fx = false;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }
bool log_on() {
    static const bool on = std::getenv("MQ_COOP_FISH_LOG") != nullptr;
    return on;
}

int area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? int8_t(ntr_rd8(c, field + kArea)) : -128;
}

// The manager while this console is in a session, else 0.
uint32_t manager(NtrCpu* c) {
    static const bool off = std::getenv("MQ_COOP_FISH_OFF") != nullptr;
    if (off || coop_sync::own_slot() < 0) return 0;
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return 0;
    return field + kManager;
}

uint32_t fish(uint32_t m, int i) { return m + mFish + uint32_t(i) * kFishSize; }

bool mirroring() { return !coop_sync::hosting() && st.host_known && st.frames - st.host_seen <= kStale; }
// Whether this console runs slot i (the host: all but those it gave the guest; a guest: those it was given, and all
// when there is no host here).
bool runs(int i) {
    if (coop_sync::hosting()) return !st.guest_owned[i];
    return !mirroring() || st.mine[i];
}

void read_record(NtrCpu* c, uint32_t f, int i, Record& r) {
    r.slot = uint8_t(i);
    r.flags = 0;
    size_t at = 0;
    for (const Range& g : kRanges)
        for (uint32_t k = 0; k < g.len; k++) r.data[at++] = ntr_rd8(c, f + g.at + k);
}

int32_t rec_pos(const Record& r, int k) {  // the place in a record ([0x1A0,...) is the third range, at 0x58 + 0x3C)
    int32_t v;
    std::memcpy(&v, r.data + 0x58 + 0x3C + k * 4, 4);
    return v;
}
void set_rec_pos(Record& r, const int32_t* p) { std::memcpy(r.data + 0x58 + 0x3C, p, 12); }
uint32_t rec_active(const Record& r) {
    uint32_t v;
    std::memcpy(&v, r.data + 0x58 + 0x3C + 0x24 + 0x08 + 0x08, 4);
    return v;
}
int8_t rec_point(const Record& r) { return int8_t(r.data[fPoint - 0xD0]); }

// Writes the owner's record into this console's copy of the slot (a fish made first where there was none).
void write_record(NtrCpu* c, uint32_t m, int i, const Record& r, bool keep_place) {
    const uint32_t f = fish(m, i);
    const bool active = rec_active(r) != 0;
    if (active && ntr_rd32(c, f + fActive) == 0 && !st.was_held[i]) {
        uint32_t saved[16];
        std::memcpy(saved, c->r, sizeof saved);
        c->r[0] = f;
        ntr_call(c, kFishReset | 1);
        c->r[0] = f, c->r[1] = m;
        ntr_call(c, kFishInit | 1);  // (the model, the animation, active)
        std::memcpy(c->r, saved, sizeof saved);
    }
    int32_t place[3];
    for (int k = 0; k < 3; k++) place[k] = int32_t(ntr_rd32(c, f + fPos + k * 4));
    size_t at = 0;
    for (const Range& g : kRanges)
        for (uint32_t k = 0; k < g.len; k++) ntr_wr8(c, f + g.at + k, r.data[at++]);
    if (keep_place)
        for (int k = 0; k < 3; k++) ntr_wr32(c, f + fPos + k * 4, uint32_t(place[k]));
    if (!active) ntr_wr8(c, f + fShown, 0);
}

uint32_t peer() {
    uint32_t nodes[8];
    const size_t n = ntr::coop_peers(nodes, 8);
    return n ? nodes[0] : 0;
}

// ---- the record of each slot this console runs, and the host's spawn points ----

void send_state(NtrCpu* c, uint32_t m) {
    const bool host = coop_sync::hosting();
    const bool full = st.frames % kFullEvery == 0;
    Header h{};
    h.op = kOpState;
    h.slot = uint8_t(coop_sync::own_slot());
    h.area = int8_t(area(c));
    h.hour = ntr_rd8(c, m + mHour);
    h.field = ntr_rd32(c, ntr_rd32(c, kField));
    h.epoch = st.epoch;
    h.fishing = ntr_rd32(c, m + mLure) != 0;
    if (host)
        for (int i = 0; i < kFish; i++) h.owners |= st.guest_owned[i] ? 1u << i : 0;
    std::vector<uint8_t> msg(sizeof h);
    const int source = int8_t(ntr_rd8(c, m + mSource));
    const bool hooked = ntr_rd32(c, m + mHookActive) != 0;
    for (int i = 0; i < kFish; i++) {
        if (!runs(i)) continue;
        Record r;
        read_record(c, fish(m, i), i, r);
        if (hooked && source == i) {  // (the hooked fish's place: the copy swims to this console's lure)
            int32_t p[3];
            for (int k = 0; k < 3; k++) p[k] = int32_t(ntr_rd32(c, m + mHooked + fPos + k * 4));
            set_rec_pos(r, p);
            r.flags |= 1;
        }
        // (the think count alone changes every frame: not a change)
        std::array<uint8_t, kRecordSize> cmp;
        std::memcpy(cmp.data(), r.data, kRecordSize);
        const size_t count_at = 0x58 + 0x3C + 0x24 + 2;
        cmp[count_at] = cmp[count_at + 1] = 0;
        if (!full && st.was_sent[i] && cmp == st.sent[i] && r.flags == st.sent_flags[i]) continue;
        st.sent[i] = cmp, st.sent_flags[i] = r.flags, st.was_sent[i] = true;
        const size_t at = msg.size();
        msg.resize(at + sizeof r);
        std::memcpy(msg.data() + at, &r, sizeof r);
        h.count++;
    }
    if (host) {
        std::array<uint8_t, kPoints> states;
        for (int p = 0; p < kPoints; p++) states[p] = ntr_rd8(c, m + mPoints + uint32_t(p) * 16 + 13);
        if (full || states != st.points_sent) {
            st.points_sent = states;
            h.points = 1;
            for (int p = 0; p < kPoints; p++) {
                Point pt{};
                const uint32_t a = m + mPoints + uint32_t(p) * 16;
                for (int k = 0; k < 3; k++) pt.pos[k] = int32_t(ntr_rd32(c, a + k * 4));
                pt.kind = int8_t(ntr_rd8(c, a + 12));
                pt.state = ntr_rd8(c, a + 13);
                const size_t at = msg.size();
                msg.resize(at + sizeof pt);
                std::memcpy(msg.data() + at, &pt, sizeof pt);
            }
        }
    }
    const bool fishing_changed = bool(h.fishing) != st.last_fishing;
    st.last_fishing = h.fishing;
    if (!host && h.count == 0 && !full && !fishing_changed) return;
    std::memcpy(msg.data(), &h, sizeof h);
    ntr::coop_send(kMsgType, msg.data(), msg.size());
}

bool shown_near(NtrCpu* c, uint32_t f);

// The other console's records into this console's copies (each frame: a hooked fish's copy swims, the rest are put).
void apply_copies(NtrCpu* c, uint32_t m) {
    for (int i = 0; i < kFish; i++) {
        if (runs(i)) continue;
        Copy& cp = st.copy[i];
        if (!cp.has) continue;
        const uint32_t f = fish(m, i);
        write_record(c, m, i, cp.rec, true);
        int32_t far = 0, cur[3];
        for (int k = 0; k < 3; k++) {
            cur[k] = int32_t(ntr_rd32(c, f + fPos + k * 4));
            far = std::max(far, std::abs(cp.target[k] - cur[k]));
        }
        for (int k = 0; k < 3; k++) {
            const int32_t v = !cp.gliding || far > kSnap ? cp.target[k] : cur[k] + (cp.target[k] - cur[k]) / 4;
            ntr_wr32(c, f + fPos + k * 4, uint32_t(v));
        }
        // (shown as the game shows its own: near the student; and its animation)
        const bool shown = ntr_rd32(c, f + fActive) != 0 && shown_near(c, f);
        ntr_wr8(c, f + fShown, shown);
        ntr_wr8(c, f + fNear, 0);
        if (shown) {
            uint32_t saved[16];
            std::memcpy(saved, c->r, sizeof saved);
            c->r[0] = f;
            ntr_call(c, kAnim | 1);
            std::memcpy(c->r, saved, sizeof saved);
        }
    }
}

void take_record(int i, const Record& r) {
    Copy& cp = st.copy[i];
    cp.rec = r, cp.has = true, cp.seen = st.frames;
    for (int k = 0; k < 3; k++) cp.target[k] = rec_pos(r, k);
    cp.gliding = (r.flags & 1) != 0;
}

void write_points(NtrCpu* c, uint32_t m, const Point* pts) {
    for (int p = 0; p < kPoints; p++) {
        const uint32_t a = m + mPoints + uint32_t(p) * 16;
        for (int k = 0; k < 3; k++) ntr_wr32(c, a + k * 4, uint32_t(pts[p].pos[k]));
        ntr_wr8(c, a + 12, uint8_t(pts[p].kind));
        ntr_wr8(c, a + 13, pts[p].state);
    }
}

// ---- a guest's lure: the hooked fish is made only from a slot this console runs ----

// The game's distances here are |dx| + |dz| (main 0x02035938), in 1/4096 units.
int32_t flat(NtrCpu* c, uint32_t a, uint32_t b) {
    return std::abs(int32_t(ntr_rd32(c, a)) - int32_t(ntr_rd32(c, b))) +
           std::abs(int32_t(ntr_rd32(c, a + 8)) - int32_t(ntr_rd32(c, b + 8)));
}

int32_t dist_to_lure(NtrCpu* c, uint32_t f, uint32_t lure) { return flat(c, f + fPos, lure); }

// As the fish's update has it (ov001 0x020D3098): within 18 units of the student, or all shown.
bool shown_near(NtrCpu* c, uint32_t f) {
    const uint32_t w = ntr_rd32(c, f + fWatch);
    if (!ram(w)) return false;
    return flat(c, w + 4, f + fPos) <= (18 << 12) || ntr_rd8(c, kShowAll) != 0;
}

// Before the manager update on a guest: whether this update would make the hooked fish (the trigger pulled, the
// second count, a fish in reach), and from which slot.
void guest_claims(NtrCpu* c, uint32_t m) {
    const uint32_t lure = ntr_rd32(c, m + mLure);
    if (st.force >= 0) {  // (granted last frame: the trigger again, that fish alone in reach)
        const int k = st.force;
        st.force = -1;
        if (ram(lure) && ntr_rd32(c, m + mHookActive) == 0 && st.mine[k]) {
            ntr_wr8(c, m + mTrigger, 1);
            ntr_wr16(c, m + mCount, uint16_t(std::max<int16_t>(int16_t(ntr_rd16(c, m + mCount)), 1)));
            for (int i = 0; i < kFish; i++) {
                if (i == k || st.was_held[i]) continue;
                const uint32_t f = fish(m, i);
                st.held[i] = ntr_rd32(c, f + fActive), st.was_held[i] = true;
                ntr_wr32(c, f + fActive, 0);
            }
            if (log_on()) std::fprintf(stderr, "coop fish: fish %d is ours: the lure's trigger again\n", k);
        }
        return;
    }
    if (!ram(lure) || ntr_rd32(c, m + mHookActive) != 0 || ntr_rd8(c, m + mTrigger) == 0) return;
    const int32_t reach = int32_t(ntr_rd32(c, kReach));
    int best = -1;
    int32_t bd = INT32_MAX;
    for (int i = 0; i < kFish; i++) {
        const uint32_t f = fish(m, i);
        const bool active = st.was_held[i] ? st.held[i] != 0 : ntr_rd32(c, f + fActive) != 0;
        if (!active || !shown_near(c, f)) continue;
        if (!st.mine[i] && st.frames < st.denied_until[i]) continue;
        const int32_t d = dist_to_lure(c, f, lure);
        if (d < bd && d < reach) bd = d, best = i;
    }
    if (best < 0 || st.mine[best]) return;  // (none in reach, or ours: as the game has it)
    // (the host's fish are not in the game's own search here: its count of triggers that found one, kept for it)
    const int16_t count = int16_t(ntr_rd16(c, m + mCount));
    if (count + 1 < 2) {
        ntr_wr16(c, m + mCount, uint16_t(count + 1));
        return;
    }
    ntr_wr8(c, m + mTrigger, 0);  // (held back while the host is asked)
    if (st.asked >= 0) return;
    st.asked = best, st.asked_frame = st.frames;
    const Ask a{kOpAsk, uint8_t(coop_sync::own_slot()), uint8_t(best), 0, st.epoch};
    ntr::coop_send(kMsgType, &a, sizeof a, peer());
    if (log_on()) std::fprintf(stderr, "coop fish: our lure found fish %d (%.2f units): asked the host\n", best, bd / 4096.0);
}

void guest_answers(NtrCpu* c, uint32_t m) {
    for (const Answer& a : st.answers) {
        if (a.epoch != st.epoch || int(a.fish) != st.asked) continue;
        const int k = a.fish;
        st.asked = -1;
        if (!a.yes) {
            st.denied_until[k] = st.frames + kDenied;
            if (log_on()) std::fprintf(stderr, "coop fish: fish %d is not ours (the host says no)\n", k);
            continue;
        }
        st.mine[k] = true, st.granted_frame[k] = st.frames, st.idle[k] = 0;
        if (st.copy[k].has) write_record(c, m, k, st.copy[k].rec, false);  // (from the host's latest)
        st.force = k;
        if (log_on()) std::fprintf(stderr, "coop fish: fish %d is ours (the host says yes)\n", k);
    }
    st.answers.clear();
    if (st.asked >= 0 && st.frames - st.asked_frame > kAskTimeout) {
        if (log_on()) std::fprintf(stderr, "coop fish: no answer about fish %d\n", st.asked);
        st.asked = -1;
    }
}

// A slot the guest was given goes back once its hook is over.
void guest_releases(NtrCpu* c, uint32_t m) {
    const int source = int8_t(ntr_rd8(c, m + mSource));
    const bool hooked = ntr_rd32(c, m + mHookActive) != 0, lure = ntr_rd32(c, m + mLure) != 0;
    for (int k = 0; k < kFish; k++) {
        if (!st.mine[k] || st.force == k) continue;
        const bool busy = hooked && source == k;
        st.idle[k] = busy || (lure && source == k) ? 0 : st.idle[k] + 1;
        if (st.frames - st.granted_frame[k] < kHoldMin || st.idle[k] < kIdleRelease) continue;
        Release r{};
        r.op = kOpRelease, r.epoch = st.epoch;
        read_record(c, fish(m, k), k, r.rec);
        ntr::coop_send(kMsgType, &r, sizeof r, peer());
        st.mine[k] = false;
        take_record(k, r.rec);  // (the copy stays where it is until the host's record says otherwise)
        if (log_on())
            std::fprintf(stderr, "coop fish: fish %d given back (point %d, at %.2f, %.2f)\n", k, rec_point(r.rec),
                         rec_pos(r.rec, 0) / 4096.0, rec_pos(r.rec, 2) / 4096.0);
    }
}

// ---- messages ----

void on_message(NtrCpu* c, uint32_t from, const uint8_t* data, size_t len) {
    const uint32_t m = manager(c);
    if (len < 1 || !m || m != st.manager) return;
    const bool host = coop_sync::hosting();
    if (data[0] == kOpState && len >= sizeof(Header)) {
        Header h;
        std::memcpy(&h, data, sizeof h);
        if (h.area != int8_t(area(c)) || h.field != ntr_rd32(c, ntr_rd32(c, kField))) return;  // (another field)
        if (host) {
            if (h.epoch != st.epoch) return;
        } else {
            if (h.slot != 0) return;
            if (!st.host_known || h.epoch != st.epoch) {  // (the host's fish of a new field: all the host's again)
                st.epoch = h.epoch;
                st.mine = {}, st.copy = {}, st.asked = -1, st.force = -1;
                if (log_on()) std::fprintf(stderr, "coop fish: the host's fish (epoch %u)\n", h.epoch);
            }
            st.host_known = true, st.host_seen = st.frames;
        }
        if (h.fishing) st.remote_fishing_seen = st.frames;
        st.remote_fishing = h.fishing;
        size_t at = sizeof h;
        for (size_t k = 0; k < h.count && at + sizeof(Record) <= len; k++, at += sizeof(Record)) {
            Record r;
            std::memcpy(&r, data + at, sizeof r);
            if (r.slot >= kFish) continue;
            if (host) {
                if (!st.guest_owned[r.slot]) continue;
                st.guest_seen[r.slot] = st.frames;
            } else if (st.mine[r.slot]) {
                continue;
            }
            take_record(r.slot, r);
        }
        if (!host && h.points && at + kPoints * sizeof(Point) <= len) {
            Point pts[kPoints];
            std::memcpy(pts, data + at, sizeof pts);
            write_points(c, m, pts);
        }
    } else if (data[0] == kOpAsk && len >= sizeof(Ask) && host) {
        Ask a;
        std::memcpy(&a, data, sizeof a);
        const int k = a.fish;
        bool yes = false;
        if (a.epoch == st.epoch && k >= 0 && k < kFish) {
            const uint32_t f = fish(m, k);
            const int source = int8_t(ntr_rd8(c, m + mSource));
            const bool ours = source == k && (ntr_rd32(c, m + mHookActive) != 0 || ntr_rd32(c, m + mLure) != 0);
            yes = ntr_rd32(c, f + fActive) != 0 && !ours;
            if (yes) {
                st.guest_owned[k] = true, st.guest_seen[k] = st.frames;
                st.copy[k] = Copy{};  // (no record of the guest's yet: the copy is left as it is)
            }
        }
        if (log_on())
            std::fprintf(stderr, "coop fish: slot %u asks for fish %d: %s\n", a.slot, k,
                         yes ? "yes, it is theirs to run" : "no");
        const Answer ans{kOpAnswer, uint8_t(k), uint8_t(yes), 0, st.epoch};
        ntr::coop_send(kMsgType, &ans, sizeof ans, from);
    } else if (data[0] == kOpAnswer && len >= sizeof(Answer) && !host) {
        Answer a;
        std::memcpy(&a, data, sizeof a);
        st.answers.push_back(a);
    } else if (data[0] == kOpRelease && len >= sizeof(Release) && host) {
        Release r;
        std::memcpy(&r, data, sizeof r);
        const int k = r.rec.slot;
        if (r.epoch != st.epoch || k >= kFish || !st.guest_owned[k]) return;
        const uint32_t f = fish(m, k);
        const int8_t was = int8_t(ntr_rd8(c, f + fPoint)), now = rec_point(r.rec);
        write_record(c, m, k, r.rec, false);
        if (was != now) {  // (the catch moved it: its old point free, its new one taken)
            if (was >= 0 && was < kPoints) ntr_wr8(c, m + mPoints + uint32_t(was) * 16 + 13, 1);
            if (now >= 0 && now < kPoints) ntr_wr8(c, m + mPoints + uint32_t(now) * 16 + 13, 2);
        }
        st.guest_owned[k] = false, st.copy[k] = Copy{}, st.was_sent[k] = false;
        if (log_on())
            std::fprintf(stderr, "coop fish: fish %d is ours again (point %d -> %d, at %.2f, %.2f)\n", k, was, now,
                         rec_pos(r.rec, 0) / 4096.0, rec_pos(r.rec, 2) / 4096.0);
    } else if (data[0] == kOpFx && len >= sizeof(Fx)) {
        Fx x;
        std::memcpy(&x, data, sizeof x);
        if (x.epoch != st.epoch || x.fish >= kFish || runs(x.fish)) return;
        const uint32_t f = fish(m, x.fish);
        if (!ntr_rd32(c, f + fActive)) return;
        int32_t keep[3];
        for (int k = 0; k < 3; k++) {
            keep[k] = int32_t(ntr_rd32(c, f + fPos + k * 4));
            ntr_wr32(c, f + fPos + k * 4, uint32_t(x.pos[k]));
        }
        if (!ram(ntr_rd32(c, f + fLureStudent))) ntr_wr32(c, f + fLureStudent, ntr_rd32(c, m + mStudentPos));
        uint32_t saved[16];
        std::memcpy(saved, c->r, sizeof saved);
        c->r[0] = f, c->r[1] = x.id;
        in_fx = true;
        ntr_call(c, kFx | 1);
        in_fx = false;
        std::memcpy(c->r, saved, sizeof saved);
        for (int k = 0; k < 3; k++) ntr_wr32(c, f + fPos + k * 4, uint32_t(keep[k]));
    }
}

const bool registered = [] {
    coop_sync::listen(kMsgType, &on_message);
    return true;
}();

uint32_t new_epoch() {
    static uint32_t n = 0;
    return (uint32_t(std::rand()) << 8) ^ ++n;
}

}  // namespace

// ov001 0x020D2748 (manager, the student, the town time, ...): a field's fish are made. Ours start again.
extern "C" void mq_fish_init(NtrCpu* c) {
    st = State{};
    st.manager = c->r[0];
    st.epoch = new_epoch();
}

namespace {

// (research, MQ_FISH_TRACE: the lure, the trigger, the hooked fish and the source slot whenever they change, also
// outside a session)
void trace(NtrCpu* c, uint32_t m) {
    static uint32_t last[7] = {};
    static int n = 0;
    n++;
    const uint32_t now[7] = {ntr_rd32(c, m + mLure) != 0, ntr_rd8(c, m + mTrigger), ntr_rd16(c, m + mCount),
                             ntr_rd8(c, m + mSource), ntr_rd32(c, m + mHookActive), ntr_rd8(c, m + mHookState),
                             ntr_rd8(c, m + mHooked + 0x1F6)};
    if (std::memcmp(now, last, sizeof now) == 0) return;
    std::memcpy(last, now, sizeof now);
    const uint32_t lure = ntr_rd32(c, m + mLure);
    std::fprintf(stderr, "fish trace %d: lure %u trigger %u count %u source %d hooked %u state %d 1F6 %u at (%.2f, %.2f) "
                 "lure at (%.2f, %.2f) 1F7 %u 1CA %d\n",
                 n, now[0], now[1], now[2], int8_t(now[3]), now[4], int8_t(now[5]), now[6],
                 int32_t(ntr_rd32(c, m + mHooked + fPos)) / 4096.0, int32_t(ntr_rd32(c, m + mHooked + fPos + 8)) / 4096.0,
                 ram(lure) ? int32_t(ntr_rd32(c, lure)) / 4096.0 : 0.0, ram(lure) ? int32_t(ntr_rd32(c, lure + 8)) / 4096.0 : 0.0,
                 ntr_rd8(c, m + mHooked + 0x1F7), int16_t(ntr_rd16(c, m + mHooked + 0x1CA)));
}

}  // namespace

// ov001 0x020D24C4 (manager): the fish manager's update, once a frame (the fish think, the lure finds one).
extern "C" int mq_fish_update(NtrCpu* c) {
    if (in_update) return 0;
    static const bool tracing = std::getenv("MQ_FISH_TRACE") != nullptr;
    if (tracing) {
        const uint32_t field = ntr_rd32(c, kField);
        if (ram(field) && c->r[0] == field + kManager) trace(c, c->r[0]);
    }
    const uint32_t m = manager(c);
    if (!m || c->r[0] != m) return 0;
    if (m != st.manager) {
        st = State{};
        st.manager = m;
        st.epoch = new_epoch();
    }
    st.frames++;
    const bool host = coop_sync::hosting();
    if (host) {  // (a guest gone from this field: its slots are the host's again)
        for (int k = 0; k < kFish; k++)
            if (st.guest_owned[k] && st.frames - st.guest_seen[k] > kStale) {
                st.guest_owned[k] = false, st.copy[k] = Copy{}, st.was_sent[k] = false;
                if (log_on()) std::fprintf(stderr, "coop fish: fish %d is ours again (no word of the guest)\n", k);
            }
    } else if (!mirroring()) {
        st.mine = {}, st.asked = -1, st.force = -1;
    }
    const bool shared = host || mirroring();
    st.was_held = {};
    if (shared) {
        if (!host) guest_answers(c, m);
        // (the copies: skipped by the update, found by no lure)
        for (int i = 0; i < kFish; i++)
            if (!runs(i)) {
                const uint32_t f = fish(m, i);
                st.held[i] = ntr_rd32(c, f + fActive), st.was_held[i] = true;
                ntr_wr32(c, f + fActive, 0);
            }
        if (!host) guest_claims(c, m);
        // (anyone's lure in the water: this console's fish make their ripples too)
        const bool remote = st.remote_fishing && st.frames - st.remote_fishing_seen <= kStale;
        st.ripple_set = {};
        if (remote)
            for (int i = 0; i < kFish; i++)
                if (runs(i)) {
                    const uint32_t f = fish(m, i);
                    st.ripple_held[i] = ntr_rd8(c, f + fRipple), st.ripple_set[i] = true;
                    ntr_wr8(c, f + fRipple, 1);
                }
    }
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    in_update = true;
    ntr_call(c, kUpdate | 1);
    in_update = false;
    std::memcpy(c->r, saved, sizeof saved);
    if (shared) {
        for (int i = 0; i < kFish; i++) {
            const uint32_t f = fish(m, i);
            if (st.was_held[i]) ntr_wr32(c, f + fActive, st.held[i]);
            if (st.ripple_set[i]) ntr_wr8(c, f + fRipple, uint8_t(st.ripple_held[i]));
        }
        st.was_held = {};
        apply_copies(c, m);
        const int source = int8_t(ntr_rd8(c, m + mSource));
        if (log_on() && source >= 0 && source != st.last_source && ntr_rd32(c, m + mHookActive) != 0)
            std::fprintf(stderr, "coop fish: a fish from slot %d goes for our lure\n", source);
        st.last_source = ntr_rd32(c, m + mHookActive) != 0 ? source : -1;
        if (!host) guest_releases(c, m);
        if (st.frames % kSendEvery == 0 || (ntr_rd32(c, m + mLure) != 0) != st.last_fishing) send_state(c, m);
    }
    return 1;
}

// ov001 0x020D2EA0 (fish, effect): a ripple or a splash at a fish (27 a fish moving while a rod is out; 23 and others
// the hooked fish). One of this console's fish, or its hooked fish: the other console makes it at its copy too.
extern "C" void mq_fish_fx(NtrCpu* c) {
    if (in_fx) return;
    const uint32_t m = manager(c);
    if (!m || m != st.manager || !(coop_sync::hosting() || mirroring())) return;
    const uint32_t o = c->r[0];
    int slot = -1;
    if (o == m + mHooked)
        slot = int8_t(ntr_rd8(c, m + mSource));
    else if (o >= m + mFish && o < m + mFish + kFish * kFishSize && (o - m - mFish) % kFishSize == 0)
        slot = int((o - m - mFish) / kFishSize);
    if (slot < 0 || slot >= kFish || !runs(slot)) return;
    Fx x{kOpFx, uint8_t(slot), uint8_t(c->r[1]), 0, st.epoch, {}};
    for (int k = 0; k < 3; k++) x.pos[k] = int32_t(ntr_rd32(c, o + fPos + k * 4));
    ntr::coop_send(kMsgType, &x, sizeof x);
}
