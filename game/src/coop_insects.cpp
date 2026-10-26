// Magician's Quest co-op: the insects of a field are the host's on every console (docs/COOP.md, "Insects together").
//
// Each console has its own insect manager in the field it stands in (field+0xA74C, ov001): when the field is made it
// rolls whether insects are out (70%), which four kinds can be (by the place, the month, the hour and the weather),
// and in its first 20 frames one spawn try per slot (20 slots, 5 per kind) with a rate, a place and a size from the
// RNG (ov001 0x020DBCC0). Each insect then thinks within 10 units of the student it watches (its +0x120): it rests,
// crawls, flies off, with the RNG. Left alone, the partner sees other insects in other places.
//
// In a session the host's console decides, as it does for the people (coop_npc.cpp):
// - The host runs its insects as the game does, with one change: each insect watches the student nearest to it, its
//   own or a guest's (the remote student objects), so that an insect flies off when a guest comes near too, and those
//   near a guest move at all. Every 6 frames it sends the field's insects (the manager's place, hour and kinds, and
//   per slot the kind, size, place, offset, turn and animation) that changed, all of them every 60 frames.
// - A guest standing in the same field mirrors them: its own spawn tries are left out; its manager is loaded again for
//   the host's place, hour and kinds when they differ (ov001 0x020DBB48 then 0x020DC648; the kinds set as the models
//   are loaded, 0x020DBBDC); each of the host's insects is made in the same slot by the game's own spawn try, run with
//   the slot, the place and the size the host's had; and its copies do not think (+0x184 set while the base update
//   runs, ov001 0x020DD280): they glide to the host's place (a quarter of the way each frame) and take its animation,
//   turn and offset. The animation itself plays on the guest's console.
// - A catch is decided once, by the host. The net's catch test (main 0x0202B4D0, from the net's state function main
//   0x02029F2C) runs on the console of the student who swings: the host's own catch stands at once (its copy goes on
//   the guests' consoles); a guest's test is run, then undone, and asked of the host. Meanwhile the guest's swing
//   holds at the miss it would have ended in (step 100). The host says yes when the insect is still free there (and
//   removes it: deleted, gone from the next state), no when it was caught already (by the host, or another guest
//   asked first). On yes the guest's console takes it as the game does (the catch test's writes, main 0x0202B3D4, the
//   catch animation, step 60): its bag, its book and its message are its own, as for a catch alone. On no the swing
//   ends as the miss it is and the copy cannot be caught there any more (the host's state removes it).
//
// In a field the host does not stand in (no state for 90 frames), a guest's insects are its own, as alone.
// MQ_COOP_INS_OFF: none of this. MQ_COOP_INS_LOG: what it does, on stderr.
#include <algorithm>
#include <cmath>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/recomp.h"

namespace {

constexpr uint8_t kMsgType = 47;  // (40 talk, 41 new day, 42 lessons, 43 mysteries, 44 incantations, 46 the Gateway)
enum Op : uint8_t { kOpState = 1, kOpAsk = 2, kOpAnswer = 3 };

// The game's addresses (USA, YNNE).
constexpr uint32_t kField = 0x020AD7EC;
constexpr uint32_t kManager = 0xA74C;  // field+: the insect manager
constexpr uint32_t kArea = 0x3EB4;     // field+: the map it shows
constexpr uint32_t kRemotes = 0x3E4C, kRemoteSize = 0x638, kRemoteCount = 3;  // field+: the other students' objects
constexpr uint32_t kRemoteShown = 0x20, kRemoteHere = 0x5A0, kRemoteSlot = 0x62C;
// In the manager.
constexpr uint32_t mSlots = 0x90;  // 20 insect pointers
constexpr int kSlots = 20;
constexpr uint32_t mKinds = 0x50E0, mState = 0x50E4, mPlace = 0x50E5, mData = 0x5104, mPlaces = 0x5108,
                   mHour = 0x5110, mTry = 0x5111, mStudent = 0x511C;
// In an insect.
constexpr uint32_t oNear = 0xCD, oAnimArg = 0xED, oSizeIndex = 0xF0, oSize = 0xF4, oF8 = 0xF8,
                   oPos = 0xFC, oOffset = 0xB4, oTurn = 0xC0, oWatch = 0x120, oAnim = 0x125, oActive = 0x13C,
                   oKind = 0x140, oAsleep = 0x184, oCatchable = 0x189;
// In a student.
constexpr uint32_t sPos = 0x2C, sModel = 0x20, sStep = 0xA5, sLocal = 0xAA, sCaught = 0x5DC;
// Functions.
constexpr uint32_t kSpawnTry = 0x020DBCC0, kReset = 0x020DBB48, kLoad = 0x020DC648, kSetAnim = 0x020DD208,
                   kObjUpdate = 0x020DD280, kCatchTest = 0x0202B4D0, kTake = 0x0202B3D4, kStudentAnim = 0x02020134,
                   kModelAnim = 0x02034EC8;
constexpr uint8_t kStepMiss = 100, kStepCaught = 0x3C;

constexpr int kSendEvery = 6, kFullEvery = 60, kHostStale = 90, kAskTimeout = 90, kMineFrames = 1800;
constexpr int32_t kSnap = 4 << 12;  // a copy this far from the host's place is put there

#pragma pack(push, 1)
struct Header {
    uint8_t op;          // kOpState
    uint8_t slot;        // the sender's session slot
    int8_t area;         // the map its field shows
    uint8_t place, hour; // the manager's place (+0x50E5) and hour slot (+0x5110)
    int8_t kinds[4];     // +0x50E0
    uint8_t ready;       // the manager has its kinds (state 0): the slots below are the field's
    uint8_t count;       // entries that follow
    uint8_t pad;
    uint32_t present;    // bit per slot: an insect there, free
};
struct Entry {
    uint8_t slot, flags;  // flags: 1 catchable (+0x189)
    uint16_t gen;         // the host's count of insects made: the same gen is the same insect
    int8_t kind;
    uint8_t size_index, f8, anim, anim_arg, pad[3];
    uint32_t size;
    int32_t pos[3], offset[3], turn[3];
};
struct Ask {
    uint8_t op, slot, insect;  // kOpAsk: the asking student's slot, the insect's slot
    uint8_t pad;
    uint16_t gen;
};
struct Answer {
    uint8_t op, insect, yes, pad;  // kOpAnswer
    uint16_t gen;
};
#pragma pack(pop)

struct HostSlot {
    uint32_t obj = 0;
    int8_t kind = -1;
    uint32_t size = 0;
    uint16_t gen = 0;
    Entry sent{};
    bool was_sent = false;
};
struct GuestSlot {
    uint16_t gen = 0;    // the host's insect this slot's copy is (0: none)
    uint32_t obj = 0;
    bool mine = false;   // granted to this console's student: left to its catch
    int mine_frame = 0;
};
struct Request {
    uint32_t from;
    Ask ask;
};

struct State {
    uint32_t manager = 0;
    int frames = 0;
    // host
    std::array<HostSlot, kSlots> hs{};
    uint16_t next_gen = 1;
    std::vector<Request> asks;
    // guest
    Header host{};
    bool host_known = false;
    int host_seen = -100000;
    std::array<Entry, kSlots> target{};
    std::array<bool, kSlots> has_target{};
    std::array<GuestSlot, kSlots> gs{};
    uint8_t loaded_for[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // place, hour, kinds we last loaded for
    int reloads = 0;
    std::vector<Answer> answers;
    int asked = -1;      // the slot asked about (-1: none)
    uint16_t asked_gen = 0;
    int asked_frame = 0;
    uint32_t asker = 0;  // the student who asked
} st;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }
bool log_on() {
    static const bool on = std::getenv("MQ_COOP_INS_LOG") != nullptr;
    return on;
}

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0, uint32_t a1 = 0, uint32_t a2 = 0, uint32_t a3 = 0,
              bool stack0 = false) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0, c->r[1] = a1, c->r[2] = a2, c->r[3] = a3;
    if (stack0) {  // (a fifth argument, 0, on the stack)
        c->r[13] -= 8;
        ntr_wr32(c, c->r[13], 0);
    }
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

int area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? int8_t(ntr_rd8(c, field + kArea)) : -128;
}

// The manager while this console stands in the host's town in a session, else 0.
uint32_t manager(NtrCpu* c) {
    static const bool off = std::getenv("MQ_COOP_INS_OFF") != nullptr;
    if (off || coop_sync::own_slot() < 0) return 0;
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return 0;
    return field + kManager;
}

uint32_t slot_obj(NtrCpu* c, uint32_t m, int i) { return ntr_rd32(c, m + mSlots + uint32_t(i) * 4); }
int8_t kind_of(NtrCpu* c, uint32_t m, int g) { return int8_t(ntr_rd8(c, m + mKinds + uint32_t(g))); }

// Whether a guest mirrors the host here now.
bool mirroring() {
    return !coop_sync::hosting() && st.host_known && st.frames - st.host_seen <= kHostStale;
}

Entry entry_of(NtrCpu* c, int i, uint32_t o) {
    Entry e{};
    e.slot = uint8_t(i);
    e.flags = ntr_rd8(c, o + oCatchable) ? 1 : 0;
    e.kind = int8_t(ntr_rd32(c, o + oKind));
    e.size_index = ntr_rd8(c, o + oSizeIndex);
    e.f8 = ntr_rd8(c, o + oF8);
    e.anim = ntr_rd8(c, o + oAnim);
    e.anim_arg = ntr_rd8(c, o + oAnimArg);
    e.size = ntr_rd32(c, o + oSize);
    for (uint32_t k = 0; k < 3; k++) {
        e.pos[k] = int32_t(ntr_rd32(c, o + oPos + k * 4));
        e.offset[k] = int32_t(ntr_rd32(c, o + oOffset + k * 4));
        e.turn[k] = int32_t(ntr_rd32(c, o + oTurn + k * 4));
    }
    return e;
}

bool changed(const Entry& a, const Entry& b) {
    if (a.flags != b.flags || a.anim != b.anim || a.anim_arg != b.anim_arg || a.gen != b.gen) return true;
    for (int k = 0; k < 3; k++)
        if (std::abs(a.pos[k] - b.pos[k]) > 0x40 || a.turn[k] != b.turn[k] || std::abs(a.offset[k] - b.offset[k]) > 0x40)
            return true;
    return false;
}

void del(NtrCpu* c, uint32_t m, int i) {
    const uint32_t o = slot_obj(c, m, i);
    if (!ram(o)) return;
    const uint32_t vt = ntr_rd32(c, o);
    if (ram(vt)) call(c, ntr_rd32(c, vt + 8), o);
    ntr_wr32(c, m + mSlots + uint32_t(i) * 4, 0);
}

// ---- the host ----

// Each insect watches the student nearest to it (the local one or a guest's here).
void watch_nearest(NtrCpu* c, uint32_t m) {
    std::vector<uint32_t> students{ntr_rd32(c, m + mStudent)};
    const uint32_t field = ntr_rd32(c, kField), list = ntr_rd32(c, field + kRemotes);
    if (ram(list))
        for (uint32_t k = 0; k < kRemoteCount; k++) {
            const uint32_t o = list + k * kRemoteSize;
            const uint8_t s = ntr_rd8(c, o + kRemoteSlot);
            if (s > 0 && s < 4 && ntr_rd32(c, o + kRemoteShown) && ntr_rd8(c, o + kRemoteHere)) students.push_back(o);
        }
    if (students.size() < 2 || !ram(students[0])) return;
    for (int i = 0; i < kSlots; i++) {
        const uint32_t o = slot_obj(c, m, i);
        if (!ram(o)) continue;
        const int32_t x = int32_t(ntr_rd32(c, o + oPos)), z = int32_t(ntr_rd32(c, o + oPos + 8));
        // (another student takes over only when nearer by a unit: two students about as near must not trade places
        // every frame, which the insect's sensor, ov001 0x020DCFEC, reads as a sudden approach: seen, it flew off)
        const uint32_t cur = ntr_rd32(c, o + oWatch);
        auto dist = [&](uint32_t s) {
            const int64_t dx = int32_t(ntr_rd32(c, s + sPos)) - x, dz = int32_t(ntr_rd32(c, s + sPos + 8)) - z;
            return std::sqrt(double(dx * dx + dz * dz));
        };
        uint32_t best = students[0];
        double bd = 1e30, cd = 1e30;
        for (uint32_t s : students) {
            const double dd = dist(s);
            if (s == cur) cd = dd;
            if (dd < bd) bd = dd, best = s;
        }
        if (best != cur && cd < 1e29 && bd + 4096.0 > cd) continue;
        ntr_wr32(c, o + oWatch, best);
    }
}

void host_asks(NtrCpu* c, uint32_t m) {
    const uint32_t me = ntr_rd32(c, m + mStudent);
    for (const Request& r : st.asks) {
        const int i = r.ask.insect;
        bool yes = false;
        if (i >= 0 && i < kSlots) {
            const uint32_t o = slot_obj(c, m, i);
            yes = ram(o) && st.hs[i].obj == o && st.hs[i].gen == r.ask.gen && ntr_rd32(c, o + oActive) != 0 &&
                  ntr_rd8(c, o + oCatchable) != 0 && !(ram(me) && int8_t(ntr_rd8(c, me + sCaught)) == i);
            if (yes) {
                del(c, m, i);
                st.hs[i] = HostSlot{};
            }
        }
        if (log_on())
            std::fprintf(stderr, "coop insects: slot %u asks for insect %d (gen %u): %s\n", r.ask.slot, i, r.ask.gen,
                         yes ? "yes, it is theirs (removed here)" : "no");
        const Answer a{kOpAnswer, uint8_t(i), uint8_t(yes), 0, r.ask.gen};
        ntr::coop_send(kMsgType, &a, sizeof a, r.from);
    }
    st.asks.clear();
}

void host_send(NtrCpu* c, uint32_t m) {
    const bool full = st.frames % kFullEvery == 0;
    Header h{};
    h.op = kOpState;
    h.slot = uint8_t(coop_sync::own_slot());
    h.area = int8_t(area(c));
    h.place = ntr_rd8(c, m + mPlace);
    h.hour = ntr_rd8(c, m + mHour);
    for (uint32_t g = 0; g < 4; g++) h.kinds[g] = kind_of(c, m, int(g));
    h.ready = ntr_rd8(c, m + mState) == 0;
    std::vector<uint8_t> msg(sizeof h);
    for (int i = 0; i < kSlots; i++) {
        const uint32_t o = slot_obj(c, m, i);
        HostSlot& s = st.hs[i];
        if (!ram(o) || ntr_rd32(c, o + oActive) == 0) {  // (none, or caught: held by the host's student)
            if (!ram(o)) s = HostSlot{};
            continue;
        }
        const int8_t kind = int8_t(ntr_rd32(c, o + oKind));
        const uint32_t size = ntr_rd32(c, o + oSize);
        if (s.obj != o || s.kind != kind || s.size != size) {  // (a new insect in this slot)
            s = HostSlot{};
            s.obj = o, s.kind = kind, s.size = size, s.gen = st.next_gen++;
            if (st.next_gen == 0) st.next_gen = 1;
        }
        h.present |= 1u << i;
        Entry e = entry_of(c, i, o);
        e.gen = s.gen;
        if (!full && s.was_sent && !changed(e, s.sent)) continue;
        s.sent = e, s.was_sent = true;
        const size_t at = msg.size();
        msg.resize(at + sizeof e);
        std::memcpy(msg.data() + at, &e, sizeof e);
        h.count++;
    }
    std::memcpy(msg.data(), &h, sizeof h);
    ntr::coop_send(kMsgType, msg.data(), msg.size());
}

// ---- a guest ----

bool forcing = false;          // our spawn try runs
int32_t forced_pos[3] = {};

// The game's own spawn try for slot i, with the host's insect.
uint32_t spawn(NtrCpu* c, uint32_t m, int i, const Entry& e) {
    const uint32_t data = ntr_rd32(c, m + mData);
    if (!ram(data) || !ram(ntr_rd32(c, m + mPlaces)) || e.kind < 0) return 0;
    const uint32_t rec = data + uint32_t(uint8_t(e.kind)) * 0x48;
    const uint8_t count = ntr_rd8(c, rec + 1), rate = ntr_rd8(c, rec + 2);
    const uint8_t tries = ntr_rd8(c, m + mTry);
    ntr_wr8(c, rec + 1, std::max<uint8_t>(count, uint8_t(i % 5 + 1)));  // (the kind's count allows this slot)
    ntr_wr8(c, rec + 2, 255);                                           // (the rate passes)
    ntr_wr8(c, m + mTry, uint8_t(i));
    std::memcpy(forced_pos, e.pos, sizeof forced_pos);
    forcing = true;
    call(c, kSpawnTry, m);
    forcing = false;
    ntr_wr8(c, rec + 1, count);
    ntr_wr8(c, rec + 2, rate);
    ntr_wr8(c, m + mTry, tries);
    const uint32_t o = slot_obj(c, m, i);
    if (!ram(o)) return 0;
    ntr_wr8(c, o + oSizeIndex, e.size_index);
    ntr_wr32(c, o + oSize, e.size);
    ntr_wr8(c, o + oF8, e.f8);
    for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, o + oPos + k * 4, uint32_t(e.pos[k]));
    return o;
}

uint32_t local_student(NtrCpu* c, uint32_t m) { return ntr_rd32(c, m + mStudent); }

// The manager loaded for the host's place, hour and kinds (when they differ), or emptied when the host has none.
// Returns whether it stands as the host's (its slots can be made).
bool guest_manager(NtrCpu* c, uint32_t m) {
    const Header& h = st.host;
    if (ntr_rd8(c, m + mState) != 0) return false;  // (loading)
    const bool none = h.kinds[0] < 0 && h.kinds[1] < 0 && h.kinds[2] < 0 && h.kinds[3] < 0;
    bool same = true;
    for (uint32_t g = 0; g < 4; g++) same &= kind_of(c, m, int(g)) == h.kinds[g];
    const bool tables = ram(ntr_rd32(c, m + mData)) && ram(ntr_rd32(c, m + mPlaces));
    if (same && (none || tables)) {
        st.reloads = 0;
        return true;
    }
    if (!h.ready) return false;
    // (not while this console's student holds an insect of this manager: the reset deletes them all)
    const uint32_t me = local_student(c, m);
    if (st.asked >= 0 || (ram(me) && int8_t(ntr_rd8(c, me + sCaught)) >= 0)) return false;
    const uint8_t want[6] = {h.place, h.hour, uint8_t(h.kinds[0]), uint8_t(h.kinds[1]), uint8_t(h.kinds[2]),
                             uint8_t(h.kinds[3])};
    if (std::memcmp(want, st.loaded_for, 6) == 0 && st.reloads > 3) return false;  // (it does not take: give up)
    st.reloads = std::memcmp(want, st.loaded_for, 6) == 0 ? st.reloads + 1 : 1;
    std::memcpy(st.loaded_for, want, 6);
    if (log_on())
        std::fprintf(stderr, "coop insects: the manager made the host's (place %u, hour %u, kinds %d %d %d %d)\n", h.place,
                     h.hour, h.kinds[0], h.kinds[1], h.kinds[2], h.kinds[3]);
    call(c, kReset, m);  // (tables freed, models unloaded, insects deleted)
    st.gs = {};
    ntr_wr8(c, m + mTry, 0);
    if (none) {
        for (uint32_t g = 0; g < 4; g++) ntr_wr8(c, m + mKinds + g, 0xFF);
        return true;
    }
    ntr_wr8(c, m + mPlace, h.place);
    ntr_wr8(c, m + mHour, h.hour);
    call(c, kLoad, m);  // (state 1: the tables; then the models for the kinds, mq_ins_models; then state 0)
    return false;
}

void guest_slots(NtrCpu* c, uint32_t m) {
    const uint32_t me = local_student(c, m);
    for (int i = 0; i < kSlots; i++) {
        GuestSlot& g = st.gs[i];
        if (g.mine) {  // (this console's catch: left to it for a while)
            if (st.frames - g.mine_frame < kMineFrames || (ram(me) && int8_t(ntr_rd8(c, me + sCaught)) == i)) continue;
            g = GuestSlot{};
        }
        const uint32_t o = slot_obj(c, m, i);
        const bool present = (st.host.present >> i) & 1;
        if (!present) {
            if (st.asked == i) continue;  // (asked about: the host's answer decides; it removed it when it said yes)
            if (ram(o) && !(ram(me) && int8_t(ntr_rd8(c, me + sCaught)) == i)) {
                if (log_on() && g.gen) std::fprintf(stderr, "coop insects: insect %d (gen %u) gone at the host\n", i, g.gen);
                del(c, m, i);
            }
            g = GuestSlot{};
            continue;
        }
        if (!st.has_target[i]) continue;
        const Entry& e = st.target[i];
        if (g.gen == e.gen && g.obj == o && ram(o)) continue;
        if (st.asked == i) continue;
        if (e.kind != kind_of(c, m, i / 5)) continue;  // (the manager is not the host's yet)
        const uint32_t made = spawn(c, m, i, e);
        g.gen = made ? e.gen : 0, g.obj = made;
        if (log_on())
            std::fprintf(stderr, "coop insects: insect %d (gen %u, kind %d) made at (%.2f, %.2f, %.2f)%s\n", i, e.gen,
                         e.kind, e.pos[0] / 4096.0, e.pos[1] / 4096.0, e.pos[2] / 4096.0, made ? "" : ": failed");
    }
}

// The host's answer: on yes the swing becomes the catch the game makes.
void guest_answers(NtrCpu* c, uint32_t m) {
    for (const Answer& a : st.answers) {
        if (a.insect != st.asked || a.gen != st.asked_gen) continue;
        const int i = a.insect;
        const uint32_t s = st.asker, o = slot_obj(c, m, i);
        st.asked = -1;
        if (!ram(s) || !ram(o)) continue;
        if (!a.yes) {
            ntr_wr8(c, o + oCatchable, 0);  // (another's: not caught here either, until the host's state removes it)
            if (log_on()) std::fprintf(stderr, "coop insects: insect %d is not ours (the host says no)\n", i);
            continue;
        }
        st.gs[i].mine = true, st.gs[i].mine_frame = st.frames;
        const uint8_t step = ntr_rd8(c, s + sStep);
        const bool held = step == kStepMiss, sweeping = step >= 30 && step < 60;
        if (held || sweeping) {  // (the catch test's writes)
            ntr_wr8(c, s + sCaught, uint8_t(i));
            ntr_wr32(c, o + oActive, 0);
            ntr_wr8(c, o + oNear, 0);
        }
        if (held) {  // (the swing's success: main 0x0202A01E..0x0202A044)
            call(c, kTake, s);
            call(c, kStudentAnim, s, 0x30, 0, 0);
            call(c, kModelAnim, ntr_rd32(c, s + sModel), 3, 0x800, 0, true);
            ntr_wr8(c, s + sStep, kStepCaught);
        }
        if (log_on())
            std::fprintf(stderr, "coop insects: insect %d is ours (the host says yes)%s\n", i,
                         held ? ": caught" : sweeping ? ": caught at the swing's end" : ": too late, the swing is over");
    }
    st.answers.clear();
    if (st.asked >= 0 && st.frames - st.asked_frame > kAskTimeout) {
        if (log_on()) std::fprintf(stderr, "coop insects: no answer about insect %d\n", st.asked);
        st.asked = -1;
    }
}

void on_message(NtrCpu* c, uint32_t from, const uint8_t* data, size_t len) {
    if (len < 1 || !manager(c)) return;
    if (data[0] == kOpState && len >= sizeof(Header) && !coop_sync::hosting()) {
        Header h;
        std::memcpy(&h, data, sizeof h);
        if (h.slot != 0 || h.area != int8_t(area(c))) return;  // (the host's other field)
        if (!st.host_known || std::memcmp(h.kinds, st.host.kinds, 4) != 0 || h.place != st.host.place ||
            h.hour != st.host.hour)
            st.has_target = {};
        st.host = h, st.host_known = true, st.host_seen = st.frames;
        for (size_t k = 0; k < h.count && sizeof h + (k + 1) * sizeof(Entry) <= len; k++) {
            Entry e;
            std::memcpy(&e, data + sizeof h + k * sizeof e, sizeof e);
            if (e.slot < kSlots) st.target[e.slot] = e, st.has_target[e.slot] = true;
        }
    } else if (data[0] == kOpAsk && len >= sizeof(Ask) && coop_sync::hosting()) {
        Request r{from, {}};
        std::memcpy(&r.ask, data, sizeof r.ask);
        st.asks.push_back(r);
    } else if (data[0] == kOpAnswer && len >= sizeof(Answer) && !coop_sync::hosting()) {
        Answer a;
        std::memcpy(&a, data, sizeof a);
        st.answers.push_back(a);
    }
}

const bool registered = [] {
    coop_sync::listen(kMsgType, &on_message);
    return true;
}();

bool in_spawn = false, in_obj = false, in_catch = false, in_net = false;

}  // namespace

// ov001 0x020DC954 (manager): the insect manager's update, once a frame.
extern "C" int mq_ins_update(NtrCpu* c) {
    const uint32_t m = manager(c);
    if (!m || c->r[0] != m) return 0;
    if (m != st.manager) {  // (another field)
        st = State{};
        st.manager = m;
    }
    st.frames++;
    if (coop_sync::hosting()) {
        host_asks(c, m);
        watch_nearest(c, m);
        if (st.frames % kSendEvery == 0) host_send(c, m);
    } else if (mirroring()) {
        guest_answers(c, m);  // (first: an insect the host gave this console is not taken away as gone there)
        if (guest_manager(c, m)) guest_slots(c, m);
    } else if (st.asked >= 0) {
        st.asked = -1;  // (no host here any more)
    }
    return 0;
}

// ov001 0x020DBCC0 (manager): a spawn try. A guest mirroring the host makes only the host's insects (spawn()).
extern "C" int mq_ins_spawn(NtrCpu* c) {
    if (forcing || in_spawn) return 0;
    const uint32_t m = manager(c);
    if (!m || c->r[0] != m || !mirroring()) return 0;
    return 1;
}

// ov001 0x020DC418 / 0x020DC140 (manager, the kind's place kind, out, slot): a spawn's place; in our try, the host's.
extern "C" int mq_ins_place(NtrCpu* c) {
    if (!forcing) return 0;
    for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, c->r[2] + k * 4, uint32_t(forced_pos[k]));
    c->r[0] = 1;
    return 1;
}

// ov001 0x020DBBDC (manager): the models of the manager's four kinds are loaded; a guest's are the host's kinds.
extern "C" void mq_ins_models(NtrCpu* c) {
    const uint32_t m = manager(c);
    if (!m || c->r[0] != m || !mirroring()) return;
    for (uint32_t g = 0; g < 4; g++) ntr_wr8(c, m + mKinds + g, uint8_t(st.host.kinds[g]));
}

// ov001 0x020DD280 (insect): an insect's update. A guest's copy of the host's does not think; it glides to the host's.
extern "C" int mq_ins_obj(NtrCpu* c) {
    if (in_obj || !mirroring()) return 0;
    const uint32_t m = manager(c), o = c->r[0];
    if (!m || m != st.manager) return 0;
    int i = -1;
    for (int k = 0; k < kSlots; k++)
        if (st.gs[k].obj == o && st.gs[k].gen && !st.gs[k].mine) i = k;
    if (i < 0 || !st.has_target[i] || st.target[i].gen != st.gs[i].gen) return 0;
    const Entry& e = st.target[i];
    int32_t far = 0;
    for (uint32_t k = 0; k < 3; k++) far = std::max(far, std::abs(e.pos[k] - int32_t(ntr_rd32(c, o + oPos + k * 4))));
    for (uint32_t k = 0; k < 3; k++) {
        const int32_t cur = int32_t(ntr_rd32(c, o + oPos + k * 4));
        ntr_wr32(c, o + oPos + k * 4, uint32_t(far > kSnap ? e.pos[k] : cur + (e.pos[k] - cur) / 4));
        ntr_wr32(c, o + oOffset + k * 4, uint32_t(e.offset[k]));
        ntr_wr32(c, o + oTurn + k * 4, uint32_t(e.turn[k]));
    }
    if (ntr_rd8(c, o + oAnim) != e.anim) call(c, kSetAnim, o, e.anim, e.anim_arg, 0);
    if (!(e.flags & 1)) ntr_wr8(c, o + oCatchable, 0);
    const uint8_t asleep = ntr_rd8(c, o + oAsleep);
    ntr_wr8(c, o + oAsleep, 1);  // (no think)
    in_obj = true;
    ntr_call(c, kObjUpdate | 1);
    in_obj = false;
    ntr_wr8(c, o + oAsleep, asleep);
    return 1;
}

// main 0x0202B4D0 (student, reach, kinds): the net's catch test; on a guest, a catch of the host's insect is asked.
extern "C" int mq_ins_catch(NtrCpu* c) {
    if (in_catch || !mirroring()) return 0;
    const uint32_t m = manager(c), s = c->r[0];
    if (!m || m != st.manager || ntr_rd8(c, s + sLocal) != 1 || int8_t(ntr_rd8(c, s + sCaught)) != -1) return 0;
    if (st.asked >= 0) {  // (waiting for the host's word)
        c->r[0] = 0;
        return 1;
    }
    uint32_t active[kSlots];
    uint8_t near[kSlots];
    for (int i = 0; i < kSlots; i++) {
        const uint32_t o = slot_obj(c, m, i);
        active[i] = ram(o) ? ntr_rd32(c, o + oActive) : 0;
        near[i] = ram(o) ? ntr_rd8(c, o + oNear) : 0;
    }
    in_catch = true;
    ntr_call(c, kCatchTest | 1);
    in_catch = false;
    const int i = int8_t(ntr_rd8(c, s + sCaught));
    if (c->r[0] == 0 || i < 0 || i >= kSlots || !st.gs[i].gen || st.gs[i].mine) return 1;  // (as the game has it)
    const uint32_t o = slot_obj(c, m, i);
    if (ram(o)) {
        ntr_wr32(c, o + oActive, active[i]);
        ntr_wr8(c, o + oNear, near[i]);
    }
    ntr_wr8(c, s + sCaught, 0xFF);
    st.asked = i, st.asked_gen = st.gs[i].gen, st.asked_frame = st.frames, st.asker = s;
    const Ask a{kOpAsk, uint8_t(coop_sync::own_slot()), uint8_t(i), 0, st.gs[i].gen};
    uint32_t nodes[8];
    const size_t n = ntr::coop_peers(nodes, 8);
    ntr::coop_send(kMsgType, &a, sizeof a, n ? nodes[0] : 0);
    if (log_on()) std::fprintf(stderr, "coop insects: our net caught insect %d (gen %u): asked the host\n", i, a.gen);
    c->r[0] = 0;
    return 1;
}

// main 0x02029F2C (student): the net's state function. While the host is asked, a guest's swing holds at its miss.
extern "C" int mq_ins_net(NtrCpu* c) {
    if (in_net || st.asked < 0 || c->r[0] != st.asker || !mirroring()) return 0;
    return ntr_rd8(c, c->r[0] + sStep) == kStepMiss ? 1 : 0;
}

// ov001 0x020D4490 / 0x020D41FC (mushrooms): when a field is made, its mushroom manager (field+0x135F0) clears the
// mushrooms it placed before and places the day's anew by trees, with the RNG. In town (+0x19 clear) they are
// ground objects of the town's grid (+0x438 of the town block, mushrooms +4), which the town sync keeps the host's: a
// guest's own clearing and placing are left out, so it keeps the host's mushrooms (the host's placing, when it enters
// town, reaches the guests as changed grid bytes). Elsewhere (+0x19 set: the field's own objects) each console's.
extern "C" int mq_mush_place(NtrCpu* c) {
    static const bool off = std::getenv("MQ_COOP_INS_OFF") != nullptr;
    if (off || coop_sync::own_slot() <= 0 || ntr_rd8(c, c->r[0] + 0x19) != 0) return 0;
    if (log_on()) std::fprintf(stderr, "coop insects: the mushrooms here are the host's (ours not placed)\n");
    return 1;
}
