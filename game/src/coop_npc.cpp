// Magician's Quest co-op: the classmates and other townspeople go about their day as in single player in a session,
// and do the same thing on every console (docs/COOP.md, "The town's people").
//
// Each console runs its own copy of the people of the field it stands in. In a session the game turns them into a
// small show for the visit (at most four classmates circling fixed spots, everyone else hidden): the placement, the
// brains, the director, the plan and the walkways all test the field's session byte. Here they run as in single
// player (see "Life as in single player" below): the people come out and go about their day by the plan
// (/npc/NpcPlan.bin) and the time, which the guest takes from the host.
//
// One console decides for each NPC: its owner. The host makes itself the owner of every NPC, except one nearest to a
// guest's student, which that guest owns (an NPC keeps its owner while it is in a job, a conversation with its owner's
// student included). Only the owner moves it by the plan (the plan's kicks, ov001 0x020E6040), lets its triggers
// start jobs (a classmate notices the owner's student, walks up, talks: the game's own way, op 2 with the owner's own
// student) and rolls where its brain wanders. Twice a second each console sends the NPCs it owns (where each is, where
// its walk goes, entry +0x2C and +0x38, and whether it is busy); the host's message also carries who owns each one.
// The other consoles take the target (their copy walks there the game's way) and put a copy more than 3 units away at
// the owner's place. In a field the host does not stand in, each guest owns its people.
//
// Only in a co-op session (the field's session byte; the messages once the console stands in the host's town,
// coop_sync::own_slot()); otherwise every entry returns at once and the game runs as it always did.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/recomp.h"

namespace {

// The game's addresses (USA, YNNE).
constexpr uint32_t kField = 0x020AD7EC;      // the town/field object
constexpr uint32_t kDirector = 0x13600;      // field+: the NPC director
constexpr uint32_t kRemotes = 0x3E4C, kRemoteSize = 0x638, kRemoteCount = 3;  // field+: the other students' objects
constexpr uint32_t kRemoteShown = 0x20, kRemoteHere = 0x5A0, kRemoteSlot = 0x62C;  // ... their model, "in this area", slot
constexpr uint32_t kArea = 0x3EB4;           // field+: the area (map) it shows
// In the director.
constexpr uint32_t kLocal = 0x27C;           // the local student
constexpr uint32_t kNpcs = 0x280, kNpcCount = 0x284;  // the NPC entries (0x2B0 each), their count (s8)
constexpr uint32_t kNpcSize = 0x2B0;
constexpr uint32_t kJobs = 0x300, kJobSize = 0x20, kJobFirst = 1, kJobEnd = 15;  // job slots: +0x11 NPC, +0x1C active
// In an NPC entry (and a student object).
constexpr uint32_t kPos = 0x2C, kTarget = 0x38, kFacing = 0x68, kId = 0xA1, kState = 0xA2, kTaken = 0xAA;
// Functions.
constexpr uint32_t kTriggerReturn = 0x020E3CE4;  // the trigger evaluator's call of the job start (ov001 0x020E1950)
constexpr uint32_t kCancelReturn = 0x020E3C38;   // its call of 0x020E6040 just before

constexpr int kMaxNpcs = 14;       // the director's NPC indexes (14-17 are the students)
constexpr int kSnapPeriod = 30;    // frames between a console's messages about the NPCs it owns
constexpr int32_t kSnapFar = 3 << 12;  // a copy this far (3 units, 20.12) from the owner's place is put there
constexpr int32_t kOwnerMargin = 1 << 12;  // a nearer student takes an NPC over when nearer by this much
constexpr int kHostStale = 90;     // frames without the host's message for this area: the guests own their people

enum Message : uint8_t {
    kPeople = coop_sync::kFirstFree + 2,  // (16, 17: the job replication of earlier builds)
};

#pragma pack(push, 1)
struct PeopleHeader {
    uint8_t slot;       // the sender's session slot
    int8_t area;        // the area its field shows
    uint8_t count;
};
struct PeopleEntry {
    uint8_t npc, owner, busy, state;
    int32_t pos[3];     // entry +0x2C
    int32_t to[3];      // entry +0x38: where its walk goes (the actor walks while it differs from pos)
    uint32_t facing;
};
#pragma pack(pop)

struct State {
    uint32_t director = 0;
    std::array<uint8_t, 256> owner{};      // by NPC id: the owner's slot (the host's table)
    std::array<bool, 256> guest_busy{};    // host: a guest says its NPC is busy
    std::vector<std::pair<uint8_t, PeopleEntry>> in;  // entries received (sender slot), to apply
    int host_seen = -100000;               // guest: the frame of the host's last message about this area
    int frames = 0;
    bool logged = false;
} st;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

// The director while this console stands in the host's town in a session, else 0.
uint32_t director(NtrCpu* c) {
    static const bool off = std::getenv("MQ_COOP_NPC_OFF") != nullptr;
    if (off || coop_sync::own_slot() < 0) return 0;
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return 0;
    const uint32_t d = field + kDirector;
    if (d != st.director) {  // (another field: what we knew of its people is gone)
        st = State{};
        st.director = d;
    }
    return d;
}

int npc_count(NtrCpu* c, uint32_t d) {
    const int n = int8_t(ntr_rd8(c, d + kNpcCount));
    return n < 0 ? 0 : n > kMaxNpcs ? kMaxNpcs : n;
}
uint32_t npc_entry(NtrCpu* c, uint32_t d, int i) { return ntr_rd32(c, d + kNpcs) + uint32_t(i) * kNpcSize; }
int npc_index(NtrCpu* c, uint32_t d, uint8_t id) {
    for (int i = 0, n = npc_count(c, d); i < n; i++)
        if (ntr_rd8(c, npc_entry(c, d, i) + kId) == id) return i;
    return -1;
}

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0, uint32_t a1 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0, c->r[1] = a1;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

// Whether an NPC has a job queued or running (a job slot with its index, active), or is taken by a conversation.
bool busy(NtrCpu* c, uint32_t d, int i) {
    for (uint32_t k = kJobFirst; k < kJobEnd; k++) {
        const uint32_t s = d + kJobs + k * kJobSize;
        if (ntr_rd8(c, s + 0x1C) && int8_t(ntr_rd8(c, s + 0x11)) == i) return true;
    }
    return ntr_rd8(c, npc_entry(c, d, i) + kTaken) != 0;
}

int area(NtrCpu* c) { return int8_t(ntr_rd8(c, ntr_rd32(c, kField) + kArea)); }

// Whether this console decides NPC i (see the top).
bool owns(NtrCpu* c, uint32_t d, int i) {
    const int own = coop_sync::own_slot();
    if (own < 0) return true;
    if (own != 0 && st.frames - st.host_seen > kHostStale) return true;  // (no host here: each guest owns its people)
    return st.owner[ntr_rd8(c, npc_entry(c, d, i) + kId)] == uint8_t(own);
}

int32_t dist(NtrCpu* c, uint32_t a, uint32_t b) {  // (the larger of |dx| and |dz| between two objects' places)
    const int32_t dx = int32_t(ntr_rd32(c, a + kPos)) - int32_t(ntr_rd32(c, b + kPos));
    const int32_t dz = int32_t(ntr_rd32(c, a + kPos + 8)) - int32_t(ntr_rd32(c, b + kPos + 8));
    return std::max(std::abs(dx), std::abs(dz));
}

// The host: who owns each NPC now.
void decide_owners(NtrCpu* c, uint32_t d) {
    struct Student {
        uint8_t slot;
        uint32_t obj;
    };
    std::vector<Student> students{{0, ntr_rd32(c, d + kLocal)}};
    const uint32_t field = ntr_rd32(c, kField), list = ntr_rd32(c, field + kRemotes);
    if (ram(list))
        for (uint32_t k = 0; k < kRemoteCount; k++) {
            const uint32_t o = list + k * kRemoteSize;
            const uint8_t s = ntr_rd8(c, o + kRemoteSlot);
            if (s > 0 && s < 4 && ntr_rd32(c, o + kRemoteShown) && ntr_rd8(c, o + kRemoteHere)) students.push_back({s, o});
        }
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    for (int i = 0, n = npc_count(c, d); i < n; i++) {
        const uint32_t p = npc_entry(c, d, i);
        const uint8_t id = ntr_rd8(c, p + kId);
        uint8_t& owner = st.owner[id];
        const Student* cur = nullptr;
        for (const Student& s : students)
            if (s.slot == owner) cur = &s;
        if (cur && (owner == 0 ? busy(c, d, i) : st.guest_busy[id])) continue;  // (busy: stays with its owner)
        const Student* best = &students[0];
        for (const Student& s : students)
            if (ram(s.obj) && dist(c, p, s.obj) < dist(c, p, best->obj)) best = &s;
        if (cur && best->slot != owner && dist(c, p, best->obj) + kOwnerMargin > dist(c, p, cur->obj)) continue;
        if (best->slot != owner) {
            if (log) std::fprintf(stderr, "coop npc: NPC %u now decided by slot %u\n", id, best->slot);
            owner = best->slot;
            st.guest_busy[id] = false;
        }
    }
}

void send_people(NtrCpu* c, uint32_t d) {
    const int own = coop_sync::own_slot();
    std::vector<uint8_t> m(sizeof(PeopleHeader));
    PeopleHeader h{uint8_t(own), int8_t(area(c)), 0};
    for (int i = 0, n = npc_count(c, d); i < n; i++) {
        const uint32_t p = npc_entry(c, d, i);
        const bool mine = owns(c, d, i);
        if (own != 0 && !mine) continue;  // (a guest: only its own; the host: all, with the owners)
        PeopleEntry e{};
        e.npc = ntr_rd8(c, p + kId);
        e.owner = own == 0 ? st.owner[e.npc] : uint8_t(own);
        e.busy = mine && busy(c, d, i);
        e.state = ntr_rd8(c, p + kState);
        for (uint32_t k = 0; k < 3; k++) {
            e.pos[k] = int32_t(ntr_rd32(c, p + kPos + k * 4));
            e.to[k] = int32_t(ntr_rd32(c, p + kTarget + k * 4));
        }
        e.facing = ntr_rd32(c, p + kFacing);
        const size_t at = m.size();
        m.resize(at + sizeof e);
        std::memcpy(m.data() + at, &e, sizeof e);
        h.count++;
    }
    std::memcpy(m.data(), &h, sizeof h);
    if (h.count) ntr::coop_send(kPeople, m.data(), m.size());
}

// The copies of NPCs another console owns follow it.
void apply_people(NtrCpu* c, uint32_t d) {
    static const bool off = std::getenv("MQ_COOP_NPC_NO_SNAP") != nullptr;
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    const int own = coop_sync::own_slot();
    for (const auto& [from, e] : st.in) {
        if (e.owner != from || e.owner == uint8_t(own)) continue;  // (only the owner's word counts)
        const int i = npc_index(c, d, e.npc);
        if (i < 0 || owns(c, d, i) || off) continue;
        const uint32_t p = npc_entry(c, d, i);
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kTarget + k * 4, uint32_t(e.to[k]));
        const int32_t dx = e.pos[0] - int32_t(ntr_rd32(c, p + kPos)), dz = e.pos[2] - int32_t(ntr_rd32(c, p + kPos + 8));
        const int32_t far = std::max(std::abs(dx), std::abs(dz));
        if (far <= kSnapFar) continue;
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kPos + k * 4, uint32_t(e.pos[k]));
        ntr_wr32(c, p + kFacing, e.facing);
        if (log)
            std::fprintf(stderr, "coop npc: NPC %u put at slot %u's place (%.2f, %.2f), %.2f units away\n", e.npc, from,
                         e.pos[0] / 4096.0, e.pos[2] / 4096.0, far / 4096.0);
    }
    st.in.clear();
}

void on_people(NtrCpu* c, uint32_t, const uint8_t* data, size_t len) {
    if (len < sizeof(PeopleHeader)) return;
    PeopleHeader h;
    std::memcpy(&h, data, sizeof h);
    if (!director(c) || h.area != int8_t(area(c))) return;  // (another field's people)
    const bool from_host = h.slot == 0, hosting = coop_sync::hosting();
    if (from_host) st.host_seen = st.frames;
    for (size_t k = 0; k < h.count && sizeof h + (k + 1) * sizeof(PeopleEntry) <= len; k++) {
        PeopleEntry e;
        std::memcpy(&e, data + sizeof h + k * sizeof e, sizeof e);
        if (from_host && !hosting) st.owner[e.npc] = e.owner;
        if (hosting && st.owner[e.npc] == h.slot) st.guest_busy[e.npc] = e.busy;
        st.in.emplace_back(h.slot, e);
    }
}

const bool registered = [] {
    coop_sync::listen(kPeople, &on_people);
    return true;
}();

// Life as in single player. In a session the game turns its people into a show for the visit. Each of these tests
// the field's session byte (+0x3EAF) and is made to run as in single player here:
// - the director's init (ov001 0x020E14C4) leaves out the day plan (/npc/NpcPlan.bin) and NpcParam (mq_coop_triggers,
//   game/src/cfie_hooks.cpp, clears its flag);
// - a field's construction (town ov002 0x020FF828, building ov003 0x020FF6C4) leaves out the people's walkways (made
//   by the placement hooks here);
// - a field's end (ov001 0x020C9AD4) does not write down where its people were for the next field (mq_npc_keep);
// - the placements (town ov002 0x020FF134, building ov003 0x020FEB54) put the visit's picks (ov003 0x021016C0: at most
//   four classmates whose record says they are out) at fixed spots, and in a building each person the list has there at
//   a fixed spot of the room; in single player they put people back near where the list has them, or hide them until
//   the plan brings them out (mq_npc_place, mq_npc_place_indoors: run with the byte cleared, except for the field's
//   player slots, ov001 0x020C5368 / 0x020C5AF8, which must stay a session's, and the relations' drift the placement
//   rolls at its end, ov001 0x020F5F08 / 0x020F6B0C, left out as in a session);
// - each NPC gets the online brain (no think) instead of the full one (town ov002 0x020FFDB4, building ov003
//   0x021000C8: mq_npc_brain), the full brain's think (ov002 0x02105F9C, ov003 0x02107D6C: mq_npc_think) and the
//   plan's kicks (ov001 0x020E6040, 0x020E61F0, 0x020E6220: mq_npc_cancel, mq_npc_plan_one, mq_npc_plan_all) do
//   nothing, nor does much of the director's update (mq_npc_director);
// - a per-frame tether (main 0x02030424) keeps the visit's picks circling their spot (skipped, mq_npc_tether).
// MQ_COOP_NPC_GAME=1: the game's own session people (none of this).
constexpr uint32_t kSessionByte = 0x3EAF;  // field+: the field was loaded in a session
constexpr uint32_t kTownWalkways = 0x020B8AC8, kTownWalkwaysField = 0x31BD0;      // ov002 0x02104C18 makes them
constexpr uint32_t kRoomWalkways = 0x020B8AC4, kRoomWalkwaysField = 0x451F4;      // ov003 0x021071F8 (field, area)
constexpr uint32_t kRegisterWalkways = 0x02048818;

uint32_t people_field(NtrCpu* c) {
    static const bool game = std::getenv("MQ_COOP_NPC_GAME") != nullptr || std::getenv("MQ_COOP_NPC_OFF") != nullptr;
    if (game) return 0;
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field) || ntr_rd8(c, field + kSessionByte) == 0) return 0;
    return field;
}

// (research: MQ_COOP_NPC_SKIP=place,walkways,keep,plan,brain,think,director,tether,owner leaves those to the game)
bool skip(const char* what) {
    static const char* list = std::getenv("MQ_COOP_NPC_SKIP");
    return list && std::strstr(list, what);
}

// Calls the hooked function itself (the registers as the hook found them) with the field's session byte cleared.
// `guard`: set while it runs, so that the hook lets the call through.
uint8_t solo_saved = 0;   // the session byte while a call of ours runs with it cleared
uint32_t solo_field = 0;
void call_solo(NtrCpu* c, uint32_t field, uint32_t fn, bool& guard) {
    const uint8_t saved = ntr_rd8(c, field + kSessionByte);
    const uint32_t outer_field = solo_field;
    const uint8_t outer_saved = solo_saved;
    solo_field = field, solo_saved = saved;
    ntr_wr8(c, field + kSessionByte, 0);
    guard = true;
    ntr_call(c, fn | 1);
    guard = false;
    ntr_wr8(c, field + kSessionByte, saved);
    solo_field = outer_field, solo_saved = outer_saved;
}

bool in_place = false, in_indoors = false, in_keep = false, in_brain = false, in_think = false, in_director = false,
     in_plan = false, in_part = false;

}  // namespace

// ov001 0x020E1950 (director, request, priority): a job starts. A trigger's job (from the evaluator) starts only on
// the console that owns the NPC.
extern "C" int mq_npc_job(NtrCpu* c) {
    if ((c->r[14] & ~1u) != kTriggerReturn || skip("owner")) return 0;
    const uint32_t d = director(c);
    if (!d || c->r[0] != d) return 0;
    const int i = int8_t(ntr_rd32(c, c->r[13] + 4) >> 8);  // (request +0x11)
    if (i < 0 || i >= npc_count(c, d) || owns(c, d, i)) {
        static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
        if (log && i >= 0 && i < npc_count(c, d))
            std::fprintf(stderr, "coop npc: NPC %u's trigger (command %d) starts here\n",
                         ntr_rd8(c, npc_entry(c, d, i) + kId), int(ntr_rd32(c, c->r[13]) & 0xFFF));
        return 0;
    }
    c->r[0] = 0;
    return 1;
}

// The plan's kicks: ov001 0x020E6040 (director, i, command; 0: the job of the plan for this hour; also the trigger
// evaluator's first step for some triggers), 0x020E61F0 (one NPC's, from its brain or the game) and 0x020E6220 (every
// NPC's, at a scene's start and the hour's change) do nothing in a session; here they run as in single player, for the
// NPCs this console owns.
static int plan_solo(NtrCpu* c, uint32_t fn) {
    if (in_plan || skip("plan")) return 0;
    const uint32_t field = people_field(c);
    if (!field) return 0;
    call_solo(c, field, fn, in_plan);
    return 1;
}
extern "C" int mq_npc_plan_one(NtrCpu* c) { return plan_solo(c, 0x020E61F0); }
extern "C" int mq_npc_plan_all(NtrCpu* c) { return plan_solo(c, 0x020E6220); }
extern "C" int mq_npc_cancel(NtrCpu* c) {
    const uint32_t d = director(c);
    if (d && c->r[0] == d && c->r[1] < uint32_t(npc_count(c, d)) && !owns(c, d, int(c->r[1])) && !skip("owner") &&
        (people_field(c) || in_plan || in_director))
        return 1;  // (another console's NPC)
    (void)kCancelReturn;
    return plan_solo(c, 0x020E6040);
}

// The director's update (ov001 0x020E2978), run as in single player. Before it: what the others said about their
// NPCs; now and then, what this console says about its own (the host: who owns which).
extern "C" int mq_npc_director(NtrCpu* c) {
    if (in_director || skip("director")) return 0;
    const uint32_t field = people_field(c);
    if (!field || c->r[0] != field + kDirector) return 0;
    const uint32_t d = director(c);
    if (d && d == c->r[0]) {
        st.frames++;
        if (!st.logged) {
            st.logged = true;
            std::fprintf(stderr, "coop npc: the people here live as in single player (%d of them, %s)\n",
                         npc_count(c, d), coop_sync::hosting() ? "this console decides them" : "each one's owner decides it");
        }
        if (!st.in.empty()) apply_people(c, d);
        if (st.frames % kSnapPeriod == 0) {
            if (coop_sync::hosting()) decide_owners(c, d);
            send_people(c, d);
        }
    }
    call_solo(c, field, 0x020E2978, in_director);
    return 1;
}

// The placements of a field's people: the town's (ov002 0x020FF134) and a building's (ov003 0x020FEB54), with the
// walkways made first (the field's construction leaves them out in a session).
static int place_solo(NtrCpu* c, uint32_t fn, bool town) {
    const uint32_t field = people_field(c);
    if (!field || skip("place")) return 0;
    const uint32_t ptr = town ? kTownWalkways : kRoomWalkways;
    if (ntr_rd32(c, ptr) == 0 && !skip("walkways")) {
        const uint32_t w = field + (town ? kTownWalkwaysField : kRoomWalkwaysField);
        if (town)
            call(c, 0x02104C18, w);
        else
            call(c, 0x021071F8, w, uint32_t(int32_t(int8_t(ntr_rd8(c, field + kArea)))));
        ntr_wr32(c, ptr, w);
        call(c, kRegisterWalkways, w);
        std::fprintf(stderr, "coop npc: the %s's walkways made for its people\n", town ? "town" : "building");
    }
    call_solo(c, field, fn, town ? in_place : in_indoors);
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    if (log) std::fprintf(stderr, "coop npc: the %s's people placed as in single player\n", town ? "town" : "building");
    return 1;
}
extern "C" int mq_npc_place(NtrCpu* c) { return in_place ? 0 : place_solo(c, 0x020FF134, true); }
extern "C" int mq_npc_place_indoors(NtrCpu* c) { return in_indoors ? 0 : place_solo(c, 0x020FEB54, false); }

// ov001 0x020C5368 and 0x020C5AF8 (field, ...): the field's player slots, set up by the placements; inside a
// placement of ours they run with the session byte as it is (they set a session's slots up).
static int as_session(NtrCpu* c, uint32_t fn) {
    if (in_part || !solo_field || !(in_place || in_indoors)) return 0;
    ntr_wr8(c, solo_field + kSessionByte, solo_saved);
    in_part = true;
    ntr_call(c, fn | 1);
    in_part = false;
    ntr_wr8(c, solo_field + kSessionByte, 0);
    return 1;
}
extern "C" int mq_npc_slots_a(NtrCpu* c) { return as_session(c, 0x020C5368); }
extern "C" int mq_npc_slots_b(NtrCpu* c) { return as_session(c, 0x020C5AF8); }
// ov001 0x020F5F08 / 0x020F6B0C: the relations' drift the placements roll at their end: left out inside ours.
extern "C" int mq_npc_drift(NtrCpu* c) {
    if (!(in_place || in_indoors)) return 0;
    c->r[0] = 0;
    return 1;
}

// ov001 0x020C9AD4 (field): a field's end. In single player it first writes down where each of its people is and
// what they are doing (the list at *0x020FE29C: 12 entries of 0x3C from +0x18), which the next field's placement reads
// to put them back (then frees it, ov001 0x020C6F1C); in a session it does not. Here it does in a session too.
extern "C" int mq_npc_keep(NtrCpu* c) {
    if (skip("keep") || in_keep) return 0;
    const uint32_t field = people_field(c);
    if (!field || c->r[0] != field) return 0;
    call_solo(c, field, 0x020C9AD4, in_keep);
    return 1;
}

// The NPCs' brains (ov002 0x020FFDB4, ov003 0x021000C8: the full one) and the full brain's think (ov002 0x02105F9C,
// ov003 0x02107D6C).
static bool in_brain_room = false, in_think_room = false;
static int solo(NtrCpu* c, uint32_t fn, bool& guard, const char* what) {
    if (guard || skip(what)) return 0;
    const uint32_t field = people_field(c);
    if (!field) return 0;
    call_solo(c, field, fn, guard);
    return 1;
}
extern "C" int mq_npc_brain(NtrCpu* c) { return solo(c, 0x020FFDB4, in_brain, "brain"); }
extern "C" int mq_npc_brain_room(NtrCpu* c) { return solo(c, 0x021000C8, in_brain_room, "brain"); }
// (brain, ?, ?, NPC): only the NPC's owner thinks for it; another console's copy walks where the owner's goes.
static bool copy_of_other(NtrCpu* c, uint32_t npc) {
    const uint32_t d = director(c);
    if (!d || skip("owner")) return false;
    const uint32_t npcs = ntr_rd32(c, d + kNpcs);
    const int n = npc_count(c, d);
    if (npc < npcs || npc >= npcs + uint32_t(n) * kNpcSize || (npc - npcs) % kNpcSize) return false;
    return !owns(c, d, int((npc - npcs) / kNpcSize));
}
extern "C" int mq_npc_think(NtrCpu* c) {
    if (!in_think && people_field(c) && copy_of_other(c, c->r[3])) return 1;
    return solo(c, 0x02105F9C, in_think, "think");
}
extern "C" int mq_npc_think_room(NtrCpu* c) {
    if (!in_think_room && people_field(c) && copy_of_other(c, c->r[3])) return 1;
    return solo(c, 0x02107D6C, in_think_room, "think");
}

// main 0x02030424 (an NPC, every frame): the visit's tether to a spot of the table. Not in a session of ours.
extern "C" int mq_npc_tether(NtrCpu* c) { return !skip("tether") && people_field(c) ? 1 : 0; }

// ov001 0x020F321C (brain, brain+0x14, NPC): a full brain picks a place to wander to (two random numbers, main
// 0x020214C0); 0 when it found none (it tries again). Only the NPC's owner picks; the others' copies walk where the
// owner's goes (its walk's target, sent with its place).
extern "C" int mq_npc_wander(NtrCpu* c) {
    if (!people_field(c) || !copy_of_other(c, c->r[2])) return 0;
    c->r[0] = 0;
    return 1;
}
