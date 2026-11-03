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
#include <cmath>
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

bool mq_myst_in_battle(NtrCpu* c);  // (game/src/coop_myst.cpp: a mystery's battle in a session)
bool mq_myst_joined_scene();         // (game/src/coop_myst.cpp: this console plays the other student's scene, joined)
bool mq_class_give_model(NtrCpu* c, uint32_t npc);  // (game/src/coop_class.cpp: a model for a classmate in a building)

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
// A person the host's console moved in one go (an event's actor_set_pos: Halloween's ghost sent off the map to
// (120, 0) by 101.3 once it has played its trick, a scene's people put in their places) stays the host's for a while,
// so that the guests' copies are put where the host's event put it. Else the nearest student decided at once: a
// guest's student was nearer to the ghost's (120, 0) than the host's, so the guest's copy (still in town, the guest
// plays no 101.3) became the one to follow, and the ghost, gone alone until a classmate is spoken to, walked the
// host's town at stage 1 (seen 2026-10-28: talks to it there ran 101.3 again, "......").
constexpr int32_t kMovedFar = 2 << 12;  // a move this long in one frame (walking is ~0.03 units a frame)
constexpr int kMovedHold = 600;         // ... keeps the person the host's this many frames
// The same for a guest's own scene: a person its scene moved in one go is the guest's for a while (it tells the host,
// a "claim", the entry's busy byte 2), so that the host's copy follows the guest's and not the other way round. Else
// the guest's copy, put by its scene somewhere else, was put back at the host's copy's place as soon as the scene
// ended: mystery 19's runner, sent off to his far spot by the guest's 089.2 ("Catch me if ya can!"), stood at once
// beside the guest's student again (7.94 units back), and the guest's next A was a talk at the start of the game,
// 089.30 "Whoa! That was close! ... Ha ha ha! In your face!": the tag game lost at once (docs/COOP_PLAYTEST.md, part 6).
// A scene joined from the other console (coop_myst.cpp) moves people the same way, but where it puts them is the
// starter's: the joining console neither claims them (a guest) nor holds them as its own event's (the host). Else the
// guest that joined the host's 089.2 claimed the runner at its own copy's far spot (each console's 089.2 picks one of
// three by a roll) and the host's copy was put back there again and again while the host's game had him run
// (docs/COOP_PLAYTEST.md, part 7).
constexpr int kClaimHold = 3600;        // ... the guest's this many frames (renewed by each move of its scenes)
constexpr uint8_t kClaim = 2;           // (PeopleEntry::busy: claimed by its sender's scene)

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
    int host_away = -100000;               // guest: the frame of the host's last message about another area
    int frames = 0;
    bool logged = false;
    std::array<int32_t, 256> last_x{}, last_z{};  // host: each person's place at the last frame
    std::array<bool, 256> seen{};
    std::array<int, 256> moved_until{};           // host: moved in one go, the host's until this frame
    std::array<uint8_t, 256> moved_owner{};       // host: ... or a guest's that claimed it (its slot)
    std::array<int, 256> claim_until{};           // guest: moved by its own scene, its own until this frame
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

// Hang-outs in a session (docs/COOP.md, "Hang-outs in a session"). Alone, a classmate who agrees to hang out
// follows the student (its follow job, NPC +0x220; the classmate at *0x020B8EF4) until its time is up, also into the
// next field (ov001 0x020C67F8 at a field's construction). The game refuses every hang-out while a session is set
// (main 0x0204A2C8) and leaves the next field's follower out (0x020C67F8 returns at once on the field's session byte).
// Here the asking console's hang-out runs as alone (mq_hang_start, mq_hang_place); that console decides the classmate
// (owns it) while it follows; each console tells the other which classmate follows its student and in which area
// (message kHang). The other console's copy of that classmate walks behind the other student's copy in the same area
// and is put out of sight (kParkX, kParkZ, where the game's own visit hides people) in another; a classmate already
// going about with the other student refuses. MQ_COOP_HANGOUT=0: the game's refusal, as before.
constexpr uint32_t kFollow = 0x020B8EEC;      // main: the hang-out's state; +8 the classmate going about with the student
constexpr uint32_t kSessionPtr = 0x020ACA58;  // *(*it + 0xA28): the session
constexpr uint32_t kHangStart = 0x0204A2C8, kHangPlace = 0x020C67F8, kHangEnd = 0x0204BD3C;
constexpr uint32_t kFollowJob = 0x220;        // NPC+: its follow job
constexpr int32_t kParkX = 115 << 12, kParkZ = 1 << 12;
constexpr int kHangFresh = 1800;  // frames a word about the other's hang-out counts (a console in a menu or a load sends none)
enum : uint8_t { kHang = 52 };                // (40 talk ... 51 shared battles)
#pragma pack(push, 1)
struct HangMessage {
    uint8_t slot, npc;  // npc: 255 none
    int8_t area;
};
#pragma pack(pop)
struct HangState {
    int frames = 0;                  // counted in every field's director update
    HangMessage partner{0, 255, -1};
    int partner_seen = -100000;
    uint8_t sent_npc = 255;
    int8_t sent_area = -1;
    int sent_at = -100000;
    // the other's classmate put out of sight here
    bool parked = false;
    uint8_t parked_npc = 255;
    uint32_t parked_director = 0;
    int32_t parked_pos[3]{}, parked_to[3]{};
    uint32_t model_tried = 0;        // the director (field) where the other's classmate was given a model
    int far_frames = 0;              // frames this student's classmate has stood still far from it
    int32_t far_x = 0, far_z = 0;
    int32_t owner_y = 0;             // the other's classmate's height on its owner's console (its message) ...
    uint8_t owner_y_npc = 255;       // ... for this classmate
} hang;

bool hang_on() {
    static const bool on = [] {
        const char* e = std::getenv("MQ_COOP_HANGOUT");
        return !(e && e[0] == '0');
    }();
    return on;
}
bool hang_log() {
    static const bool on = std::getenv("MQ_COOP_HANGOUT_LOG") != nullptr;
    return on;
}
// The classmate going about with this console's student (its id), or 255.
uint8_t local_follower(NtrCpu* c) {
    const uint32_t o = ntr_rd32(c, kFollow + 8);
    return ram(o) ? ntr_rd8(c, o + kId) : 255;
}
// The classmate going about with the other student, or 255.
uint8_t partner_follower() {
    if (!hang_on() || coop_sync::own_slot() < 0 || hang.frames - hang.partner_seen > kHangFresh) return 255;
    return hang.partner.npc;
}

// Whether this console decides NPC i (see the top). In a mystery's battle each console decides its own people: the
// battle is each console's own, its monster an NPC of the battle's field (actor 171 / 197 / 214) that only this
// console's student fights (coop_myst.cpp, Battles); the monster's reaction to a hit is one of its jobs (mq_npc_job):
// owned by the other console, a joined student's hits took no heart.
bool owns(NtrCpu* c, uint32_t d, int i) {
    const int own = coop_sync::own_slot();
    if (own < 0 || mq_myst_in_battle(c)) return true;
    if (hang_on()) {  // (a classmate going about with a student: that student's console decides it)
        const uint8_t id = ntr_rd8(c, npc_entry(c, d, i) + kId);
        if (id != 255 && id == local_follower(c)) return true;
        if (id != 255 && id == partner_follower()) return false;
    }
    if (own != 0 && st.frames - st.host_seen > kHostStale) return true;  // (no host here: each guest owns its people)
    if (own != 0 && st.claim_until[ntr_rd8(c, npc_entry(c, d, i) + kId)] > st.frames) return true;  // (its scene's)
    return st.owner[ntr_rd8(c, npc_entry(c, d, i) + kId)] == uint8_t(own);
}

int32_t dist(NtrCpu* c, uint32_t a, uint32_t b) {  // (the larger of |dx| and |dz| between two objects' places)
    const int32_t dx = int32_t(ntr_rd32(c, a + kPos)) - int32_t(ntr_rd32(c, b + kPos));
    const int32_t dz = int32_t(ntr_rd32(c, a + kPos + 8)) - int32_t(ntr_rd32(c, b + kPos + 8));
    return std::max(std::abs(dx), std::abs(dz));
}

// The host, each frame: people moved in one go (kMovedFar) stay the host's for kMovedHold frames.
void note_moves(NtrCpu* c, uint32_t d) {
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    for (int i = 0, n = npc_count(c, d); i < n; i++) {
        const uint32_t p = npc_entry(c, d, i);
        const uint8_t id = ntr_rd8(c, p + kId);
        const int32_t x = int32_t(ntr_rd32(c, p + kPos)), z = int32_t(ntr_rd32(c, p + kPos + 8));
        if (st.seen[id] && std::max(std::abs(x - st.last_x[id]), std::abs(z - st.last_z[id])) > kMovedFar &&
            !mq_myst_joined_scene()) {  // (a guest's scene joined here: where it puts people is the guest's)
            if (log || st.owner[id] != 0)
                std::fprintf(stderr, "coop npc: NPC %u moved by an event here to (%.2f, %.2f): the host's for now\n", id,
                             x / 4096.0, z / 4096.0);
            st.moved_until[id] = st.frames + kMovedHold;
            st.moved_owner[id] = 0;
            st.owner[id] = 0, st.guest_busy[id] = false;
        }
        st.seen[id] = true, st.last_x[id] = x, st.last_z[id] = z;
    }
}

// A guest, each frame: people its own scene moved in one go (an event of this console runs, not one of ours, file 900)
// become its own for kClaimHold frames; the next message tells the host.
void note_own_moves(NtrCpu* c, uint32_t d) {
    const uint32_t ctx = ntr_rd32(c, 0x020B8AC0);
    const bool scene = ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0) &&
                       int16_t(ntr_rd16(c, 0x020B8AAC + 6)) != 900 && !mq_myst_joined_scene();
    for (int i = 0, n = npc_count(c, d); i < n; i++) {
        const uint32_t p = npc_entry(c, d, i);
        const uint8_t id = ntr_rd8(c, p + kId);
        const int32_t x = int32_t(ntr_rd32(c, p + kPos)), z = int32_t(ntr_rd32(c, p + kPos + 8));
        if (scene && st.seen[id] && std::max(std::abs(x - st.last_x[id]), std::abs(z - st.last_z[id])) > kMovedFar) {
            std::fprintf(stderr, "coop npc: NPC %u moved by this console's scene to (%.2f, %.2f): its own for now\n", id,
                         x / 4096.0, z / 4096.0);
            st.claim_until[id] = st.frames + kClaimHold;
            st.owner[id] = uint8_t(coop_sync::own_slot());
        }
        st.seen[id] = true, st.last_x[id] = x, st.last_z[id] = z;
    }
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
        if (hang_on() && id != 255 && (id == local_follower(c) || id == partner_follower())) {
            const uint8_t want = id == local_follower(c) ? 0 : hang.partner.slot;
            if (owner != want && log)
                std::fprintf(stderr, "coop npc: NPC %u now decided by slot %u (hanging out with its student)\n", id, want);
            owner = want;
            continue;
        }
        if (st.moved_until[id] > st.frames) {  // (moved by the host's event or a guest's scene: theirs for now)
            if (owner != st.moved_owner[id]) {
                if (log)
                    std::fprintf(stderr, "coop npc: NPC %u now decided by slot %u (moved by its scene)\n", id,
                                 st.moved_owner[id]);
                owner = st.moved_owner[id];
                st.guest_busy[id] = false;
            }
            continue;
        }
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
        if (own != 0 && st.claim_until[e.npc] > st.frames) e.busy = kClaim;
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
    // (the host's message goes also without people: a guest learns where the host stands, mq_npc_decides)
    if (h.count || own == 0) ntr::coop_send(kPeople, m.data(), m.size());
}

// The copies of NPCs another console owns follow it.
void apply_people(NtrCpu* c, uint32_t d) {
    static const bool off = std::getenv("MQ_COOP_NPC_NO_SNAP") != nullptr;
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    const int own = coop_sync::own_slot();
    // (a scene on this console places its own people, each console its own scene: the owner's places would take them
    // from where the scene put them. Seen: classmates taken from their desks in a lesson, game/src/coop_class.cpp; the
    // tea party's guests (500.3) taken from the table in town on the guest's console (RAM: classmate 38 at (18.7,
    // 60.1), 76 at (57.9, 65.7) instead of (59.1, 75.0) and (58.0, 76.1)). Once the scene is over the owner's places
    // are taken again.)
    // (Our own events, file 900, place no people: "Go and help <name>?" may stay up a minute, and the host's places
    // were not taken meanwhile; Halloween's ghost sent off by the host's 101.3 stayed in the guest's town.)
    const uint32_t ctx = ntr_rd32(c, 0x020B8AC0);
    const bool ours = int16_t(ntr_rd16(c, 0x020B8AAC + 6)) == 900;  // (the running event's file, main 0x0201D0CC)
    if (ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0) && !ours) {
        st.in.clear();
        return;
    }
    for (const auto& [from, e] : st.in) {
        if (e.owner != from || e.owner == uint8_t(own)) continue;  // (only the owner's word counts)
        const int i = npc_index(c, d, e.npc);
        if (i < 0 || owns(c, d, i) || off) continue;
        if (e.npc == partner_follower()) {  // (walks behind the other student's copy: hang_frame; its height is the owner's)
            hang.owner_y = e.pos[1], hang.owner_y_npc = e.npc;
            continue;
        }
        if (st.moved_until[e.npc] > st.frames && coop_sync::hosting() && st.moved_owner[e.npc] == 0)
            continue;  // (the host's event put it there)
        const uint32_t p = npc_entry(c, d, i);
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kTarget + k * 4, uint32_t(e.to[k]));
        const int32_t dx = e.pos[0] - int32_t(ntr_rd32(c, p + kPos)), dz = e.pos[2] - int32_t(ntr_rd32(c, p + kPos + 8));
        const int32_t far = std::max(std::abs(dx), std::abs(dz));
        if (far <= kSnapFar) continue;
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kPos + k * 4, uint32_t(e.pos[k]));
        ntr_wr32(c, p + kFacing, e.facing);
        st.last_x[e.npc] = e.pos[0], st.last_z[e.npc] = e.pos[2];  // (put here by us: not an event's move)
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
    if (!director(c)) return;
    if (h.area != int8_t(area(c))) {  // (another field's people)
        if (h.slot == 0) st.host_away = st.frames;
        return;
    }
    const bool from_host = h.slot == 0, hosting = coop_sync::hosting();
    if (from_host) st.host_seen = st.frames;
    for (size_t k = 0; k < h.count && sizeof h + (k + 1) * sizeof(PeopleEntry) <= len; k++) {
        PeopleEntry e;
        std::memcpy(&e, data + sizeof h + k * sizeof e, sizeof e);
        if (from_host && !hosting && st.claim_until[e.npc] <= st.frames) st.owner[e.npc] = e.owner;
        if (hosting && e.busy == kClaim && e.owner == h.slot &&
            !(st.moved_until[e.npc] > st.frames && st.moved_owner[e.npc] == 0)) {  // (the host's own event first)
            if (st.owner[e.npc] != h.slot || st.moved_owner[e.npc] != h.slot || st.moved_until[e.npc] <= st.frames)
                std::fprintf(stderr, "coop npc: NPC %u claimed by slot %u's scene: its for now\n", e.npc, h.slot);
            st.owner[e.npc] = h.slot, st.moved_owner[e.npc] = h.slot;
            st.moved_until[e.npc] = st.frames + kMovedHold;
        }
        if (hosting && st.owner[e.npc] == h.slot) st.guest_busy[e.npc] = e.busy;
        st.in.emplace_back(h.slot, e);
    }
}

void on_hang(NtrCpu*, uint32_t, const uint8_t* data, size_t len) {
    if (len < sizeof(HangMessage) || !hang_on()) return;
    HangMessage m;
    std::memcpy(&m, data, sizeof m);
    if (m.slot > 3 || int(m.slot) == coop_sync::own_slot()) return;
    if (hang_log() && (m.npc != hang.partner.npc || m.area != hang.partner.area))
        std::fprintf(stderr, "coop hang: slot %u's student goes about with %s%u (area %d)\n", m.slot,
                     m.npc == 255 ? "nobody, " : "NPC ", m.npc, m.area);
    hang.partner = m;
    hang.partner_seen = hang.frames;
}

const bool registered = [] {
    coop_sync::listen(kPeople, &on_people);
    coop_sync::listen(kHang, &on_hang);
    return true;
}();

// The other student's copy in this field (its object), or 0.
uint32_t partner_copy(NtrCpu* c, uint8_t slot) {
    const uint32_t field = ntr_rd32(c, kField), list = ntr_rd32(c, field + kRemotes);
    if (!ram(list)) return 0;
    for (uint32_t k = 0; k < kRemoteCount; k++) {
        const uint32_t o = list + k * kRemoteSize;
        if (ntr_rd8(c, o + kRemoteSlot) == slot && ntr_rd32(c, o + kRemoteShown) && ntr_rd8(c, o + kRemoteHere)) return o;
    }
    return 0;
}

void unpark(NtrCpu* c, uint32_t d) {
    if (!hang.parked) return;
    if (hang.parked_director == d) {
        const int i = npc_index(c, d, hang.parked_npc);
        if (i >= 0) {
            const uint32_t p = npc_entry(c, d, i);
            for (uint32_t k = 0; k < 3; k++) {
                ntr_wr32(c, p + kPos + k * 4, uint32_t(hang.parked_pos[k]));
                ntr_wr32(c, p + kTarget + k * 4, uint32_t(hang.parked_to[k]));
            }
            st.last_x[hang.parked_npc] = hang.parked_pos[0], st.last_z[hang.parked_npc] = hang.parked_pos[2];
        }
        if (hang_log()) std::fprintf(stderr, "coop hang: NPC %u back where it was\n", hang.parked_npc);
    }
    hang.parked = false;
}

// Each frame of a session field's director: tell the other console about this student's hang-out (on a change and
// every kSnapPeriod frames); the other student's classmate here walks behind its copy, or is out of sight while that
// student is in another area.
void hang_frame(NtrCpu* c, uint32_t d) {
    const int own = coop_sync::own_slot();
    const uint8_t mine = local_follower(c);
    const int8_t here = int8_t(area(c));
    if (mine != hang.sent_npc || here != hang.sent_area || hang.frames - hang.sent_at >= kSnapPeriod) {
        if (hang_log() && (mine != hang.sent_npc))
            std::fprintf(stderr, "coop hang: this student goes about with %s%u\n", mine == 255 ? "nobody, " : "NPC ", mine);
        const HangMessage m{uint8_t(own), mine, here};
        ntr::coop_send(kHang, &m, sizeof m);
        hang.sent_npc = mine, hang.sent_area = here, hang.sent_at = hang.frames;
    }
    // (this student moved in one go, as our own steps do: a joined mystery's warp_local, a test step: the classmate
    // stood where it was, out of its follow (seen: action 33, 50 units away, for good), and the other console's copy
    // walked beside the student: standing still more than 12 units away for 1.5 s, it is put beside the student, as
    // a field's change brings it)
    if (mine != 255) {
        const int i = npc_index(c, d, mine);
        const uint32_t me = ntr_rd32(c, kField) + 0x383C;
        const uint32_t q = i >= 0 ? npc_entry(c, d, i) : 0;
        const int32_t qx = q ? int32_t(ntr_rd32(c, q + kPos)) : 0, qz = q ? int32_t(ntr_rd32(c, q + kPos + 8)) : 0;
        const bool still = q && qx == hang.far_x && qz == hang.far_z;  // (a classmate walking to the student moves)
        hang.far_x = qx, hang.far_z = qz;
        if (q && still && dist(c, q, me) > (12 << 12)) {
            if (++hang.far_frames >= 90) {
                const uint32_t p = npc_entry(c, d, i);
                ntr_wr32(c, p + kPos, ntr_rd32(c, me + kPos) + (1 << 12));
                ntr_wr32(c, p + kPos + 4, ntr_rd32(c, me + kPos + 4));
                ntr_wr32(c, p + kPos + 8, ntr_rd32(c, me + kPos + 8));
                for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kTarget + k * 4, ntr_rd32(c, p + kPos + k * 4));
                st.last_x[mine] = int32_t(ntr_rd32(c, p + kPos)), st.last_z[mine] = int32_t(ntr_rd32(c, p + kPos + 8));
                std::fprintf(stderr, "coop hang: NPC %u left far behind (the student moved in one go): put beside it\n", mine);
                hang.far_frames = 0;
            }
        } else {
            hang.far_frames = 0;
        }
    }
    const uint8_t theirs = partner_follower();
    // (both asked the same classmate at once: the host's student keeps it)
    if (mine != 255 && mine == theirs && own != 0 && hang.partner.slot == 0) {
        const uint32_t o = ntr_rd32(c, kFollow + 8);
        std::fprintf(stderr, "coop hang: NPC %u goes about with the host's student: this one's hang-out ends\n", mine);
        call(c, kHangEnd, o + kFollowJob);
        return;
    }
    if (hang.parked && (hang.parked_director != d || hang.parked_npc != theirs || hang.partner.area == here))
        unpark(c, d);
    if (theirs == 255 || theirs == mine) return;
    const int i = npc_index(c, d, theirs);
    if (i < 0) return;
    const uint32_t p = npc_entry(c, d, i);
    if (hang.partner.area != here) {  // (with the other student in another area: out of sight here)
        if (!hang.parked) {
            hang.parked = true, hang.parked_npc = theirs, hang.parked_director = d;
            for (uint32_t k = 0; k < 3; k++) {
                hang.parked_pos[k] = int32_t(ntr_rd32(c, p + kPos + k * 4));
                hang.parked_to[k] = int32_t(ntr_rd32(c, p + kTarget + k * 4));
            }
            if (hang_log())
                std::fprintf(stderr, "coop hang: NPC %u is with the other student in area %d: out of sight here\n", theirs,
                             hang.partner.area);
        }
        ntr_wr32(c, p + kPos, uint32_t(kParkX)), ntr_wr32(c, p + kPos + 8, uint32_t(kParkZ));
        ntr_wr32(c, p + kTarget, uint32_t(kParkX)), ntr_wr32(c, p + kTarget + 8, uint32_t(kParkZ));
        st.last_x[theirs] = kParkX, st.last_z[theirs] = kParkZ;
        return;
    }
    const uint32_t o = partner_copy(c, hang.partner.slot);
    if (!o) return;
    // (in a building the people kept get no models in a session: the classmate's own, once)
    if (here >= 0 && ntr_rd32(c, p + 0x20) == 0 && hang.model_tried != d) {
        hang.model_tried = d;
        mq_class_give_model(c, p);
    }
    const int32_t ox = int32_t(ntr_rd32(c, o + kPos)), oz = int32_t(ntr_rd32(c, o + kPos + 8));
    const int32_t px = int32_t(ntr_rd32(c, p + kPos)), pz = int32_t(ntr_rd32(c, p + kPos + 8));
    const double dx = (px - ox) / 4096.0, dz = (pz - oz) / 4096.0, far = std::sqrt(dx * dx + dz * dz);
    constexpr double kBehind = 1.2, kPut = 8.0;
    // (the height: the owner's word for it (twice a second), else its own; a remote copy's own height is not the
    // ground's: put at it, the classmate stood 3 units under the ground and could not be talked to)
    int32_t y = int32_t(ntr_rd32(c, p + kPos + 4));
    if (hang.owner_y_npc == theirs) {
        if (std::abs(y - hang.owner_y) > (1 << 12)) ntr_wr32(c, p + kPos + 4, uint32_t(hang.owner_y));
        y = hang.owner_y;
    }
    if (far > kPut) {  // (far behind, or just come in: put beside the other student's copy)
        ntr_wr32(c, p + kPos, uint32_t(ox + (1 << 12))), ntr_wr32(c, p + kPos + 4, uint32_t(y));
        ntr_wr32(c, p + kPos + 8, uint32_t(oz));
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kTarget + k * 4, ntr_rd32(c, p + kPos + k * 4));
        st.last_x[theirs] = ox + (1 << 12), st.last_z[theirs] = oz;
        if (hang_log()) std::fprintf(stderr, "coop hang: NPC %u put beside the other student (%.1f units away)\n", theirs, far);
        return;
    }
    if (far > kBehind + 0.3) {  // (walks to a spot behind the other student)
        const double k = kBehind / far;
        ntr_wr32(c, p + kTarget, uint32_t(ox + int32_t(dx * k * 4096.0)));
        ntr_wr32(c, p + kTarget + 4, uint32_t(y));
        ntr_wr32(c, p + kTarget + 8, uint32_t(oz + int32_t(dz * k * 4096.0)));
    } else {
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kTarget + k * 4, ntr_rd32(c, p + kPos + k * 4));
    }
}

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

// The field's session byte as the session has it, also while a call of ours runs with it cleared (call_solo): the
// town's model manager (ov002 0x02100E30, mq_town_models in coop_class.cpp) is reached from inside those calls too
// (the director's or a brain's), and as alone it gave the other student's model back (main 0x020348DC (model, 0)), so
// its copy had none when it next came in: the host's hang leaving the school (docs/COOP_PLAYTEST.md).
uint8_t mq_npc_session_byte(NtrCpu* c, uint32_t field) {
    const uint8_t b = ntr_rd8(c, field + kSessionByte);
    return b == 0 && solo_field == field ? solo_saved : b;
}

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
void mq_class_models_made(NtrCpu* c);  // (game/src/coop_class.cpp)
extern "C" int mq_npc_plan_all(NtrCpu* c) {
    mq_class_models_made(c);  // (the classroom's people have their models: its session byte back)
    return plan_solo(c, 0x020E6220);
}
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
    if (!in_director) hang.frames++;
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
        if (mq_myst_in_battle(c)) {
            st.in.clear();  // (a battle's people are each console's own: none taken, none sent)
        } else {
            if (!st.in.empty()) apply_people(c, d);
            if (hang_on()) hang_frame(c, d);
            if (coop_sync::hosting()) note_moves(c, d);
            else note_own_moves(c, d);
            if (st.frames % kSnapPeriod == 0) {
                if (coop_sync::hosting()) decide_owners(c, d);
                send_people(c, d);
            }
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
// Whether this console decides the NPC with this id (a classmate's record +0x96) in the field it stands in, for the
// town sync (coop_sync.cpp: a guest's changes to a classmate's needs, docs/COOP.md "The classmates' needs"): 1 yes, 0
// no (another console does, or it is not among this field's people), -1 not known yet (a guest just after a field's
// load: no word from the host about this area or another yet). Unlike owns(), "no host here" counts only once the host
// has said it stands elsewhere, not after a silence.
int mq_npc_decides(NtrCpu* c, uint8_t id) {
    const int own = coop_sync::own_slot();
    if (own < 0) return -1;
    if (own == 0) return 1;
    const uint32_t d = director(c);
    if (!d) return -1;
    const int i = npc_index(c, d, id);
    if (st.frames - st.host_seen <= kHostStale) return i >= 0 && st.owner[id] == uint8_t(own) ? 1 : 0;
    if (st.frames - st.host_away <= kHostStale && st.host_away > st.host_seen) return i >= 0 ? 1 : 0;
    return -1;
}

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

// main 0x0204A2C8 (follow job, classmate): a classmate starts going about with the student (a hang-out agreed to, or
// the classmate taken along into the next field). It answers 0 at once while a session is set
// (*(*0x020ACA58 + 0xA28)); here it runs as alone in a session (the session pointer cleared for the call), except for a
// classmate already going about with the other student (0: the game's own answer for a classmate who cannot come).
extern "C" int mq_hang_start(NtrCpu* c) {
    static bool guard = false;
    if (guard || !hang_on()) return 0;
    const uint32_t sp = ntr_rd32(c, kSessionPtr);
    if (!ram(sp)) return 0;
    const uint32_t session = ntr_rd32(c, sp + 0xA28);
    if (!session) return 0;
    const uint32_t npc = c->r[1];
    const uint8_t id = ram(npc) ? ntr_rd8(c, npc + kId) : 255;
    if (id != 255 && id == partner_follower()) {
        std::fprintf(stderr, "coop hang: NPC %u already goes about with the other student: not with this one\n", id);
        c->r[0] = 0;
        return 1;
    }
    ntr_wr32(c, sp + 0xA28, 0);
    guard = true;
    ntr_call(c, kHangStart | 1);
    guard = false;
    ntr_wr32(c, sp + 0xA28, session);
    std::fprintf(stderr, "coop hang: NPC %u %s\n", id, c->r[0] ? "goes about with this student" : "does not come");
    return 1;
}

// main 0x020306E0 (classmate): a classmate's own update each frame (its follow job, main 0x02049B08 on +0x220; the
// time it asks to go about with the student, 0x0204A234). It returns at once while a session is set; here it runs as
// alone for the classmate going about with this console's student (the session pointer cleared for the call), so it
// follows the student and its time runs out as alone. The others' stay as in a session.
extern "C" int mq_hang_update(NtrCpu* c) {
    static bool guard = false;
    if (guard || !hang_on()) return 0;
    const uint32_t npc = c->r[0];
    if (!ram(npc) || npc != ntr_rd32(c, kFollow + 8)) return 0;
    const uint32_t sp = ntr_rd32(c, kSessionPtr);
    if (!ram(sp)) return 0;
    const uint32_t session = ntr_rd32(c, sp + 0xA28);
    if (!session) return 0;
    ntr_wr32(c, sp + 0xA28, 0);
    guard = true;
    ntr_call(c, 0x020306E0 | 1);
    guard = false;
    ntr_wr32(c, sp + 0xA28, session);
    return 1;
}

// ov001 0x020C67F8 (field, n; from a field's construction, the town's and a building's): it first takes the classmate
// going about with the student along into the new field (the kept list's +0x330: 0x0204A25C), then keeps at most n
// people; in a session it leaves the first part out (the field's session byte). Here it runs as alone (the byte
// cleared). Called by mq_class_summon (coop_class.cpp), the hook there, when that one leaves the call to the game.
static bool in_hang_place = false;
int mq_hang_place(NtrCpu* c) {
    if (in_hang_place || !hang_on()) return 0;
    const uint32_t field = people_field(c);
    if (!field || c->r[0] != field) return 0;
    // (a building in a session keeps no people, n = field+0x3EAC = 0, and the first part wants n > 0: at least 1 when
    // the kept list, *0x020FE29C, has a classmate going about with the student, +0x330)
    const uint32_t kept = ntr_rd32(c, 0x020FE29C);
    if (int32_t(c->r[1]) <= 0 && ram(kept) && ntr_rd8(c, kept + 0x330) != 0xFF) c->r[1] = 1;
    call_solo(c, field, kHangPlace, in_hang_place);
    return 1;
}
