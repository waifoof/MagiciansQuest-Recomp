// Magician's Quest co-op: the classmates and other townspeople do the same thing on every console of a session
// (docs/COOP.md, "The town's people").
//
// Each console runs its own copy of the town's people. In a session the game turns them into a small show for the
// visit (at most four classmates circling fixed spots, everyone else hidden): the town's placement, the brains, the
// director, the plan and the walkways all test the field's session byte. Here they run as in single player (see "Town
// life as in single player" below): the people come out and go about their day by the plan (/npc/NpcPlan.bin) and the
// town's time, which the guest takes from the host.
//
// What is not the same on every console the host decides:
// - where each person is and is going: a guest makes no plan moves of its own; twice a second the host sends each
//   NPC's place and its walk's target (entry +0x2C, +0x38), a guest's NPC walks to the same target and one more than
//   2 units away is put at the host's place;
// - the triggers (a classmate notices a student and walks up to them, invites them...): the host's evaluator sees
//   each student in turn (mq_npc_test) and the host sends each job it starts (the call of 0x020E1950 returning to
//   0x020E3CE4, with the NPC's trigger parameters at director+0x68C + i*16); the people and students the job names are
//   sent as an NPC id or a session slot, not as pointers. A guest drops the jobs of its own evaluator and starts the
//   host's through the same call, before its director's next update;
// - the full brains' wander places (mq_npc_wander);
// - the rolls inside a command are made from the job (the NPC, the host's count of its jobs, the place, the count of
//   rolls) instead of the game's random numbers, the same on every console;
// - a conversation (op 2) starts on the console of the student the job is about; the other consoles keep the NPC
//   waiting (as the game does while its own student talks) until that console says the conversation is over.
//
// Only in a co-op session (the field's session byte; the messages once the console stands in the host's town,
// coop_sync::own_slot()); otherwise every entry returns at once and the game runs as it always did.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
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
// In the director.
constexpr uint32_t kLocal = 0x27C;           // the local student
constexpr uint32_t kNpcs = 0x280, kNpcCount = 0x284;  // the NPC entries (0x2B0 each), their count (s8)
constexpr uint32_t kNpcSize = 0x2B0;
constexpr uint32_t kFrames = 0x274;          // the director's frame count
constexpr uint32_t kJobs = 0x300, kJobSize = 0x20, kJobFirst = 1, kJobEnd = 15;  // job slots: +0x11 NPC, +0x1C active
constexpr uint32_t kTrigParams = 0x68C, kTrigParamSize = 0x10;  // + i*16: written just before a trigger's job
constexpr uint32_t kCommand = 0x17A0, kCommandSize = 0x20;      // + i*32: an NPC's command state; +0xE op 2's status
constexpr uint32_t kOp2Status = 0x0E;
// In an NPC entry.
constexpr uint32_t kTarget = 0x38;
constexpr uint32_t kX = 0x2C, kZ = 0x34, kFacing = 0x68, kId = 0xA1, kState = 0xA2;
// Functions.
constexpr uint32_t kEnqueue = 0x020E1950;    // (director, request: r1-r3 + 2 stack words, priority): start a job
constexpr uint32_t kTriggerReturn = 0x020E3CE4;  // the trigger evaluator's call of it
constexpr uint32_t kCancel = 0x020E6040;     // (director, i, 0): what the evaluator does to an NPC's job first...
constexpr uint32_t kCancelReturn = 0x020E3C38;   // ... for some triggers, just before starting the new one
constexpr uint32_t kOp2 = 0x020E7918;        // command op 2 (director, i): start a conversation with the student
// The rolls inside NPC commands (returns of the calls of main 0x02005CF8, random in [r0, r1)).
constexpr uint32_t kRolls[] = {
    0x020C2F0E, 0x020C2F56,  // the actor resolver (0x020C26E0), codes 363/364: a random classmate
    0x020CA282,              // a one-liner balloon's variant (/talk/msg/onemsg%02d.ofs)
    0x020CFAD0,              // an item pick (0x020CFA60, category 0x1A), from command ops 26/52
};

constexpr int kMaxNpcs = 14;       // the director's NPC indexes (14-17 are the students)
constexpr uint32_t kOp2Timeout = 3600;  // director frames a console waits for another's conversation (a minute)
constexpr int kSnapPeriod = 30;    // frames between the host's snapshots of where its people are and are going
constexpr int32_t kSnapFar = 2 << 12;  // a guest's NPC this far (2 units, 20.12) from the host's moves there

enum Message : uint8_t {
    kJob = coop_sync::kFirstFree,  // host: a trigger's job
    kTalkDone,                     // the console of the student a job was about: its conversation (op 2) is over
    kSnap,                         // host: where its people stand
    kWander,                       // host: the place an NPC's brain picked to wander to
};

// An actor a job names, as every console can find it: none, a student by session slot, an NPC by id.
enum Ref : uint8_t { kNone = 0, kSlot = 1, kNpc = 2 };

#pragma pack(push, 1)
struct JobMsg {
    uint8_t npc;        // the NPC's id (entry +0xA1)
    uint8_t flags;      // bit 0: the evaluator called kCancel for it first
    uint16_t seq;       // the host's count of this NPC's trigger jobs
    uint8_t prio;
    int8_t target;      // the session slot of the student the job is about, or -1
    uint8_t ref4[2], ref8[2];  // request +4 and +8 (actor pointers), as Refs
    uint32_t w0;        // request +0
    uint32_t w12, w16;  // request +0x0C..+0x13 (with +0x11, the NPC's index, written again here)
    uint8_t params[kTrigParamSize];
};
struct TalkDoneMsg {
    uint8_t npc;
    uint16_t seq;
};
struct WanderMsg {
    uint8_t npc;
    int32_t to[3];      // brain +0x14 (x, y, z, 20.12)
};
struct SnapEntry {
    uint8_t npc, state;
    int32_t pos[3];     // entry +0x2C
    int32_t to[3];      // entry +0x38: where its walk goes (the actor walks while it differs from pos)
    uint32_t facing;
};
#pragma pack(pop)

struct Npc {
    uint16_t seq = 0;       // the last trigger job's number (host: sent, guest: started)
    int target = -1;        // the student slot its job is about, until op 2 has run
    bool cancel = false;    // host: the evaluator just called kCancel for it
    bool waiting = false;   // op 2 waits for the target's console
    uint32_t wait_from = 0;
    uint16_t done = 0;      // the last kTalkDone from another console (its seq)
    bool done_valid = false;
    uint32_t rolls = 0;     // rolls made in this job so far
    bool params_valid = false;  // guest: the trigger parameters the host last sent
    uint8_t params[kTrigParamSize] = {};
};

struct State {
    uint32_t director = 0;
    std::array<Npc, kMaxNpcs> npcs;
    std::deque<JobMsg> jobs;            // guest: the host's jobs to start
    std::vector<SnapEntry> snap;        // guest: the host's last snapshot (to apply)
    std::array<bool, 256> wander_valid{};  // guest: the host's last wander place for each NPC id, not used yet
    std::array<std::array<int32_t, 3>, 256> wander{};
    uint32_t rotation = 0;              // host: the trigger evaluator's student, in turn
    bool passthrough = false;           // a call of ours into a hooked function: let it run
    int cur = -1;                       // the NPC whose commands run now
    int frames = 0;
    bool logged = false;
} st;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

// The director while this console stands in the host's town in a session, else 0.
uint32_t director(NtrCpu* c) {
    static const bool off = std::getenv("MQ_COOP_NPC_OFF") != nullptr;  // (research: the game's own people)
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

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

// A remote student object's session slot (its vtable +0x1C, as the game asks it at 0x020E2CE0).
int remote_slot(NtrCpu* c, uint32_t obj) {
    const uint32_t vt = ntr_rd32(c, obj);
    if (!ram(vt)) return -1;
    return int(int8_t(call(c, ntr_rd32(c, vt + 0x1C), obj)));
}
uint32_t remote_of_slot(NtrCpu* c, int slot) {
    const uint32_t field = ntr_rd32(c, kField), list = ntr_rd32(c, field + kRemotes);
    if (!ram(list)) return 0;
    for (uint32_t k = 0; k < kRemoteCount; k++)
        if (remote_slot(c, list + k * kRemoteSize) == slot) return list + k * kRemoteSize;
    return 0;
}

// An actor pointer of a request, as a Ref; false when it is none of ours (a pointer the other consoles cannot find).
bool to_ref(NtrCpu* c, uint32_t d, uint32_t p, uint8_t out[2]) {
    out[0] = kNone, out[1] = 0;
    if (p == 0) return true;
    if (p == ntr_rd32(c, d + kLocal)) {
        out[0] = kSlot, out[1] = uint8_t(coop_sync::own_slot());
        return true;
    }
    const uint32_t field = ntr_rd32(c, kField), list = ntr_rd32(c, field + kRemotes);
    if (ram(list) && p >= list && p < list + kRemoteCount * kRemoteSize && (p - list) % kRemoteSize == 0) {
        const int s = remote_slot(c, p);
        if (s < 0 || s > 3) return false;
        out[0] = kSlot, out[1] = uint8_t(s);
        return true;
    }
    const uint32_t npcs = ntr_rd32(c, d + kNpcs);
    const int n = npc_count(c, d);
    if (p >= npcs && p < npcs + uint32_t(n) * kNpcSize && (p - npcs) % kNpcSize == 0) {
        out[0] = kNpc, out[1] = ntr_rd8(c, p + kId);
        return true;
    }
    return false;
}
// ... and back, on this console; false when it is not here.
bool from_ref(NtrCpu* c, uint32_t d, const uint8_t r[2], uint32_t& p) {
    p = 0;
    if (r[0] == kNone) return true;
    if (r[0] == kSlot) {
        p = r[1] == coop_sync::own_slot() ? ntr_rd32(c, d + kLocal) : remote_of_slot(c, r[1]);
        return p != 0;
    }
    if (r[0] == kNpc) {
        const int i = npc_index(c, d, r[1]);
        if (i >= 0) p = npc_entry(c, d, i);
        return p != 0;
    }
    return false;
}

uint64_t mix(uint64_t x) {  // (splitmix64's finalizer)
    x ^= x >> 30, x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 27, x *= 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

// Whether an NPC has a job queued or running (a job slot with its index, active).
bool has_job(NtrCpu* c, uint32_t d, int i) {
    for (uint32_t k = kJobFirst; k < kJobEnd; k++) {
        const uint32_t s = d + kJobs + k * kJobSize;
        if (ntr_rd8(c, s + 0x1C) && int8_t(ntr_rd8(c, s + 0x11)) == i) return true;
    }
    return false;
}

// The guest: the host's jobs, started through the game's own call, in the order the host started them.
void start_jobs(NtrCpu* c, uint32_t d) {
    while (!st.jobs.empty()) {
        const JobMsg m = st.jobs.front();
        st.jobs.pop_front();
        const int i = npc_index(c, d, m.npc);
        uint32_t p4 = 0, p8 = 0;
        if (i < 0 || !from_ref(c, d, m.ref4, p4) || !from_ref(c, d, m.ref8, p8)) {
            std::fprintf(stderr, "coop npc: the host's job %u for NPC %u cannot start here (NPC %d)\n", m.seq, m.npc, i);
            continue;
        }
        Npc& n = st.npcs[size_t(i)];
        for (uint32_t k = 0; k < kTrigParamSize; k++) ntr_wr8(c, d + kTrigParams + uint32_t(i) * kTrigParamSize + k, m.params[k]);
        std::memcpy(n.params, m.params, sizeof n.params);
        n.params_valid = true;
        st.passthrough = true;
        if (m.flags & 1) {
            uint32_t saved[16];
            std::memcpy(saved, c->r, sizeof saved);
            c->r[0] = d, c->r[1] = uint32_t(i), c->r[2] = 0;
            ntr_call(c, kCancel | 1);
            std::memcpy(c->r, saved, sizeof saved);
        }
        // The request as the evaluator builds it (r1-r3, then two words and the priority on the stack).
        const uint32_t w16 = (m.w16 & ~0xFF00u) | (uint32_t(i) & 0xFF) << 8;
        uint32_t saved[16];
        std::memcpy(saved, c->r, sizeof saved);
        const uint32_t sp = c->r[13] - 16;
        ntr_wr32(c, sp, m.w12), ntr_wr32(c, sp + 4, w16), ntr_wr32(c, sp + 8, m.prio);
        c->r[13] = sp;
        c->r[0] = d, c->r[1] = m.w0, c->r[2] = p4, c->r[3] = p8;
        ntr_call(c, kEnqueue | 1);
        const uint32_t ok = c->r[0];
        std::memcpy(c->r, saved, sizeof saved);
        st.passthrough = false;
        n.seq = m.seq, n.rolls = 0, n.target = m.target, n.waiting = false;
        static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
        if (log)
            std::fprintf(stderr, "coop npc: host's job %u for NPC %u (#%d) about slot %d: %s\n", m.seq, m.npc, i,
                         m.target, ok ? "started" : "not taken");
    }
}

// The guest: each NPC goes where the host's goes (its walk's target), and one more than 2 units from the host's place
// is put there (an NPC in a job of the host's, or waiting for a conversation, is left to it).
SnapEntry snap_of(NtrCpu* c, uint32_t p) {
    SnapEntry e{};
    e.npc = uint8_t(ntr_rd8(c, p + kId)), e.state = uint8_t(ntr_rd8(c, p + kState));
    for (uint32_t k = 0; k < 3; k++) {
        e.pos[k] = int32_t(ntr_rd32(c, p + kX + k * 4));
        e.to[k] = int32_t(ntr_rd32(c, p + kTarget + k * 4));
    }
    e.facing = ntr_rd32(c, p + kFacing);
    return e;
}
void apply_snap(NtrCpu* c, uint32_t d) {
    static const bool off = std::getenv("MQ_COOP_NPC_NO_SNAP") != nullptr;
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    if (off) {
        st.snap.clear();
        return;
    }
    for (const SnapEntry& e : st.snap) {
        const int i = npc_index(c, d, e.npc);
        if (i < 0 || has_job(c, d, i) || st.npcs[size_t(i)].waiting) continue;
        const uint32_t p = npc_entry(c, d, i);
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kTarget + k * 4, uint32_t(e.to[k]));
        const int32_t dx = e.pos[0] - int32_t(ntr_rd32(c, p + kX)), dz = e.pos[2] - int32_t(ntr_rd32(c, p + kZ));
        const int32_t far = std::max(std::abs(dx), std::abs(dz));
        if (far <= kSnapFar) continue;
        for (uint32_t k = 0; k < 3; k++) ntr_wr32(c, p + kX + k * 4, uint32_t(e.pos[k]));
        ntr_wr32(c, p + kFacing, e.facing);
        if (log)
            std::fprintf(stderr, "coop npc: NPC %u put at the host's place (%.2f, %.2f), %.2f units away\n", e.npc,
                         e.pos[0] / 4096.0, e.pos[2] / 4096.0, far / 4096.0);
    }
    st.snap.clear();
}

void on_job(NtrCpu*, uint32_t, const uint8_t* data, size_t len) {
    if (coop_sync::hosting() || len < sizeof(JobMsg)) return;
    JobMsg m;
    std::memcpy(&m, data, sizeof m);
    st.jobs.push_back(m);
}

void on_talk_done(NtrCpu* c, uint32_t, const uint8_t* data, size_t len) {
    if (len < sizeof(TalkDoneMsg)) return;
    TalkDoneMsg m;
    std::memcpy(&m, data, sizeof m);
    const uint32_t d = director(c);
    if (!d) return;
    const int i = npc_index(c, d, m.npc);
    if (i < 0) return;
    Npc& n = st.npcs[size_t(i)];
    n.done = m.seq, n.done_valid = true;
}

void on_snap(NtrCpu*, uint32_t, const uint8_t* data, size_t len) {
    if (coop_sync::hosting()) return;
    st.snap.clear();
    for (size_t k = 0; k + sizeof(SnapEntry) <= len; k += sizeof(SnapEntry)) {
        SnapEntry e;
        std::memcpy(&e, data + k, sizeof e);
        st.snap.push_back(e);
    }
}

void on_wander(NtrCpu*, uint32_t, const uint8_t* data, size_t len) {
    if (coop_sync::hosting() || len < sizeof(WanderMsg)) return;
    WanderMsg m;
    std::memcpy(&m, data, sizeof m);
    st.wander_valid[m.npc] = true;
    for (int k = 0; k < 3; k++) st.wander[m.npc][size_t(k)] = m.to[k];
}

const bool registered = [] {
    coop_sync::listen(kJob, &on_job);
    coop_sync::listen(kTalkDone, &on_talk_done);
    coop_sync::listen(kSnap, &on_snap);
    coop_sync::listen(kWander, &on_wander);
    return true;
}();

// Town life as in single player. In a session the game turns its people into a show for the visit. Each of these
// tests the field's session byte (+0x3EAF) and is made to run as in single player here:
// - the director's init (ov001 0x020E14C4) leaves out the day plan (/npc/NpcPlan.bin) and NpcParam (mq_coop_triggers,
//   game/src/cfie_hooks.cpp, clears its flag);
// - the town's construction (ov002 0x020FF828) leaves out the people's walkways (mq_npc_place makes them);
// - a field's end (ov001 0x020C9AD4) does not write down where its people were for the next field (mq_npc_keep);
// - the placement (ov002 0x020FF134) puts the visit's picks (ov003 0x021016C0: at most four classmates whose record
//   says they are out, at fixed spots in the table at 0x020FE2A0) there and hides the rest (mq_npc_place: the spots
//   are made the hidden place, as single player's placement hides everyone until the plan brings them out);
// - each NPC gets the online brain (no think) instead of the full one (ov002 0x020FFDB4, mq_npc_brain), the full
//   brain's think (ov002 0x02105F9C, mq_npc_think) and the plan's kicks (ov001 0x020E6040, 0x020E61F0, 0x020E6220:
//   mq_npc_cancel, mq_npc_plan_one, mq_npc_plan_all) do nothing, nor does much of the director's update (mq_npc_director);
// - a per-frame tether (main 0x02030424) keeps the visit's picks circling their spot (skipped, mq_npc_tether).
// MQ_COOP_NPC_GAME=1: the game's own session people (none of this). Buildings (ov003) are left as the game has them
// in a session (mq_npc_place_indoors).
constexpr uint32_t kSessionByte = 0x3EAF;  // field+: the field was loaded in a session
constexpr uint32_t kWalkways = 0x020B8AC8, kWalkwaysField = 0x31BD0;  // the town's walkways (mq_npc_place)
constexpr uint32_t kPeopleList = 0x020FE29C;  // where the people were (a field's end, mq_npc_keep)
constexpr uint32_t kListFirst = 0x18, kListSize = 0x3C, kListCount = 12;
constexpr uint32_t kHiddenX = 0x73000, kHiddenZ = 0x1000;  // where the placements keep their hidden people
constexpr uint32_t kSpotTable = 0x020FE2A0, kSpotSize = 0x10, kSpots = 14, kSpotUsed = 0x0C;  // the visit's spots

uint32_t people_field(NtrCpu* c) {
    static const bool game = std::getenv("MQ_COOP_NPC_GAME") != nullptr || std::getenv("MQ_COOP_NPC_OFF") != nullptr;
    if (game) return 0;
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field) || ntr_rd8(c, field + kSessionByte) == 0) return 0;
    return field;
}

// Calls the hooked function itself (the registers as the hook found them) with the field's session byte cleared.
// `guard`: set while it runs, so that the hook lets the call through.
void call_solo(NtrCpu* c, uint32_t field, uint32_t fn, bool& guard) {
    const uint8_t saved = ntr_rd8(c, field + kSessionByte);
    ntr_wr8(c, field + kSessionByte, 0);
    guard = true;
    ntr_call(c, fn | 1);
    guard = false;
    ntr_wr8(c, field + kSessionByte, saved);
}

// (research: MQ_COOP_NPC_SKIP=place,walkways,keep,plan,brain,think,director,tether,test,wander leaves those to the game)
bool skip(const char* what) {
    static const char* list = std::getenv("MQ_COOP_NPC_SKIP");
    return list && std::strstr(list, what);
}

bool in_keep = false, in_indoors = false, in_brain = false, in_think = false, in_director = false, in_test = false, in_wander = false;

}  // namespace

// ov001 0x020E1950 (director, request, priority): a job starts. From the trigger evaluator: the host sends it (once the
// game has taken it), a guest drops its own (the host's comes instead). Any other job for an NPC ends what we knew of
// its trigger job.
extern "C" int mq_npc_job(NtrCpu* c) {
    if (st.passthrough) return 0;
    const uint32_t d = director(c);
    if (!d || c->r[0] != d) return 0;
    const int i = int8_t(ntr_rd32(c, c->r[13] + 4) >> 8);  // (request +0x11)
    if ((c->r[14] & ~1u) != kTriggerReturn) {
        if (i >= 0 && i < kMaxNpcs) st.npcs[size_t(i)].target = -1;
        return 0;
    }
    if (i < 0 || i >= npc_count(c, d)) return 0;
    Npc& n = st.npcs[size_t(i)];
    if (!coop_sync::hosting()) {
        // (the parameters the evaluator just wrote for it go back to the host's)
        if (n.params_valid)
            for (uint32_t k = 0; k < kTrigParamSize; k++)
                ntr_wr8(c, d + kTrigParams + uint32_t(i) * kTrigParamSize + k, n.params[k]);
        c->r[0] = 0;
        return 1;
    }
    JobMsg m{};
    m.npc = ntr_rd8(c, npc_entry(c, d, i) + kId);
    m.flags = n.cancel ? 1 : 0;
    n.cancel = false;
    m.prio = uint8_t(ntr_rd32(c, c->r[13] + 8));
    m.w0 = c->r[1];
    m.w12 = ntr_rd32(c, c->r[13]);
    m.w16 = ntr_rd32(c, c->r[13] + 4);
    const bool refs = to_ref(c, d, c->r[2], m.ref4) && to_ref(c, d, c->r[3], m.ref8);
    m.target = m.ref4[0] == kSlot ? int8_t(m.ref4[1]) : int8_t(-1);
    for (uint32_t k = 0; k < kTrigParamSize; k++) m.params[k] = ntr_rd8(c, d + kTrigParams + uint32_t(i) * kTrigParamSize + k);
    st.passthrough = true;
    ntr_call(c, kEnqueue | 1);
    st.passthrough = false;
    if (c->r[0] == 0) return 1;  // (not taken: a job of its own goes first)
    n.seq++, n.rolls = 0, n.target = m.target, n.waiting = false;
    m.seq = n.seq;
    if (!refs) {
        std::fprintf(stderr, "coop npc: NPC %u's job names an actor the others cannot find (%08x %08x): not sent\n",
                     m.npc, m.w0, c->r[2]);
        return 1;
    }
    ntr::coop_send(kJob, &m, sizeof m);
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    if (log)
        std::fprintf(stderr, "coop npc: job %u for NPC %u (#%d) about slot %d sent (cmd %d/%d, refs %u:%u %u:%u)\n", m.seq,
                     m.npc, i, m.target, int16_t(m.w12), int16_t(m.w12 >> 16), m.ref4[0], m.ref4[1], m.ref8[0], m.ref8[1]);
    return 1;
}

// ov001 0x020E6040 (director, i, 0): also the evaluator's first step for some triggers. The host notes it for the job;
// a guest leaves it to the host's job (its own trigger does not start).
// The plan's kicks: ov001 0x020E6040 (director, i, command; 0: the job of the plan for this hour), 0x020E61F0 (one
// NPC's, from its brain or the game) and 0x020E6220 (every NPC's, at a scene's start and the hour's change) do
// nothing in a session (the people never go where their day takes them); here they run as in single player.
static bool in_plan = false;
// A guest makes none of its own: its people go where the host's go (the snapshot, apply_snap).
static int plan_solo(NtrCpu* c, uint32_t fn) {
    if (in_plan || skip("plan")) return 0;
    const uint32_t field = people_field(c);
    if (!field) return 0;
    if (coop_sync::own_slot() > 0) return 1;
    call_solo(c, field, fn, in_plan);
    return 1;
}
extern "C" int mq_npc_plan_one(NtrCpu* c) { return plan_solo(c, 0x020E61F0); }
extern "C" int mq_npc_plan_all(NtrCpu* c) { return plan_solo(c, 0x020E6220); }

extern "C" int mq_npc_cancel(NtrCpu* c) {
    if (st.passthrough) return 0;
    if ((c->r[14] & ~1u) == kCancelReturn) {
        const uint32_t d = director(c);
        if (d && c->r[0] == d && c->r[1] < uint32_t(kMaxNpcs)) {
            if (!coop_sync::hosting()) return 1;
            st.npcs[c->r[1]].cancel = true;
        }
    }
    return plan_solo(c, 0x020E6040);
}

// The director's update (ov001 0x020E2978), in a session run as in single player (above). Before it a guest starts
// the host's jobs and takes its snapshot; the host sends one now and then.
extern "C" int mq_npc_director(NtrCpu* c) {
    if (skip("director")) return 0;
    if (in_director) return 0;
    const uint32_t field = people_field(c);
    if (!field || c->r[0] != field + kDirector) return 0;
    const uint8_t saved = ntr_rd8(c, field + kSessionByte);
    ntr_wr8(c, field + kSessionByte, 0);
    const uint32_t d = director(c);
    if (d && d == c->r[0]) {
        if (!st.logged) {
            st.logged = true;
            std::fprintf(stderr, "coop npc: the town's people follow %s (%d of them), as in single player\n",
                         coop_sync::hosting() ? "this console" : "the host", npc_count(c, d));
        }
        if (!coop_sync::hosting()) {
            start_jobs(c, d);
            if (!st.snap.empty()) apply_snap(c, d);
        } else if (++st.frames % kSnapPeriod == 0) {
            std::vector<SnapEntry> snap;
            for (int i = 0, n = npc_count(c, d); i < n; i++) snap.push_back(snap_of(c, npc_entry(c, d, i)));
            if (!snap.empty()) ntr::coop_send(kSnap, snap.data(), snap.size() * sizeof(SnapEntry));
        }
    }
    in_director = true;
    ntr_call(c, 0x020E2978 | 1);
    in_director = false;
    ntr_wr8(c, field + kSessionByte, saved);
    return 1;
}

// The town's placement of its people (ov002 0x020FF134, the scene's vtable), the NPCs' brains (ov002 0x020FFDB4) and
// the full brain's think (ov002 0x02105F9C): as in single player in a session. (The relations' drift the placement
// makes at its end stays left out, as the game does in a session.)
static bool in_place_log = false, in_place_dump = false;
extern "C" int mq_npc_place(NtrCpu* c) {
    static const bool dump = std::getenv("MQ_COOP_NPC_LIST") != nullptr;  // (research: the list and the result, logged)
    if (dump && !in_place_log && !in_place_dump) {
        const uint32_t l = ntr_rd32(c, kPeopleList);
        std::fprintf(stderr, "npc list %08x (session %d)\n", l, ntr_rd8(c, ntr_rd32(c, kField) + kSessionByte));
        for (uint32_t i = 0; ram(l) && i < kListCount; i++) {
            std::fprintf(stderr, "  L%2u ", i);
            for (uint32_t k = 0; k < kListSize; k++) std::fprintf(stderr, "%02x", ntr_rd8(c, l + kListFirst + i * kListSize + k));
            std::fprintf(stderr, "\n");
        }
        in_place_dump = true;
        mq_npc_place(c);
        in_place_dump = false;
        in_place_log = true;
        ntr_call(c, 0x020FF134 | 1);
        in_place_log = false;
        const uint32_t field = ntr_rd32(c, kField), arr = ntr_rd32(c, field + 0x3E48);
        for (int i = 0, n = int8_t(ntr_rd8(c, field + 0x3EAD)); i < n && i < 14; i++) {
            const uint32_t e = arr + uint32_t(i) * kNpcSize;
            std::fprintf(stderr, "  N%2d id %u state %u at (%.2f, %.2f)\n", i, ntr_rd8(c, e + kId), ntr_rd8(c, e + kState),
                         int32_t(ntr_rd32(c, e + kX)) / 4096.0, int32_t(ntr_rd32(c, e + kZ)) / 4096.0);
        }
        return 1;
    }
    if (in_place_log || skip("place")) return 0;
    const uint32_t field = people_field(c);
    if (!field) return 0;
    // The town's walkways for its people (field+0x31BD0: ov002 0x02104C18 makes them, main 0x02048818 registers them,
    // *0x020B8AC8 points at them): the town's construction (ov002 0x020FF828) leaves them out in a session, and
    // without them no plan's walk starts (main 0x0201F81C answers "not here"). Made here, before the placement.
    if (ntr_rd32(c, kWalkways) == 0 && !skip("walkways")) {
        const uint32_t w = field + kWalkwaysField;
        call(c, 0x02104C18, w);
        ntr_wr32(c, kWalkways, w);
        call(c, 0x02048818, w);
        std::fprintf(stderr, "coop npc: the town's walkways made for its people\n");
    }
    // (not with the session byte cleared: the placement also sets up the field's player slots for a session, ov001
    // 0x020C5AF8. The session's placement differs in what it does with the visit's spots, the table at 0x020FE2A0
    // that the visit's set-up fills (ov003 0x021016C0: at most four classmates whose record says they are out, at
    // fixed spots): with no list of where the people were (mq_npc_keep) it shows those four there and hides the rest;
    // with one, it puts each person the list has in this area at its spot. So every spot is made the place where the
    // game keeps its hidden people (as single player's placement leaves everyone), and the plan brings them out.)
    const uint32_t list = ntr_rd32(c, kPeopleList);
    for (uint32_t i = 0; i < kSpots; i++) {
        const uint32_t spot = kSpotTable + i * kSpotSize;
        ntr_wr8(c, spot + kSpotUsed, 0);
        ntr_wr32(c, spot + 0, kHiddenX), ntr_wr32(c, spot + 4, 0), ntr_wr32(c, spot + 8, kHiddenZ);
    }
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    if (log) std::fprintf(stderr, "coop npc: the town's placement with %s\n", ram(list) ? "the list of where the people were" : "no list");
    return 0;
}

// ov001 0x020C9AD4 (field): a field's end. In single player it first writes down where each of its people is and
// what they are doing (the list at *0x020FE29C: 12 entries of 0x3C from +0x18, +0x26 set for one that is there, +8 its
// place), which the next field's placement reads to put them back (then frees it, ov001 0x020C6F1C); in a session it
// does not, so a session's town starts with nobody out. Here it does in a session too.
extern "C" int mq_npc_keep(NtrCpu* c) {
    if (skip("keep") || in_keep) return 0;
    const uint32_t field = people_field(c);
    if (!field || c->r[0] != field) return 0;
    call_solo(c, field, 0x020C9AD4, in_keep);
    return 1;
}

// ov003 0x020FEB54: a building's placement of its people. Left as the game has it in a session: without the list.
extern "C" int mq_npc_place_indoors(NtrCpu* c) {
    if (in_indoors || !people_field(c)) return 0;
    const uint32_t list = ntr_rd32(c, kPeopleList);
    ntr_wr32(c, kPeopleList, 0);
    in_indoors = true;
    ntr_call(c, 0x020FEB54 | 1);
    in_indoors = false;
    ntr_wr32(c, kPeopleList, list);
    return 1;
}
extern "C" int mq_npc_brain(NtrCpu* c) {
    if (skip("brain")) return 0;
    if (in_brain) return 0;
    const uint32_t field = people_field(c);
    if (!field) return 0;
    call_solo(c, field, 0x020FFDB4, in_brain);
    return 1;
}
extern "C" int mq_npc_think(NtrCpu* c) {
    if (skip("think")) return 0;
    if (in_think) return 0;
    const uint32_t field = people_field(c);
    if (!field) return 0;
    call_solo(c, field, 0x02105F9C, in_think);
    return 1;
}
// main 0x02030424 (an NPC, every frame): the visit's tether to a spot of the table. Not in a session of ours.
extern "C" int mq_npc_tether(NtrCpu* c) { return !skip("tether") && people_field(c) ? 1 : 0; }

// ov001 0x020E2C08 (director, +0x2A8, NPC index, player): the trigger evaluator's test of a student (player -1: the
// director's own student, D+0x27C, as in single player). On the host each call tests the next student of the session
// in turn (the host's own, then each other student standing in this area), so that a classmate can notice a guest too:
// for a guest's student the call runs with D+0x27C pointing at that student's object (field+0x3E4C); the job it starts
// names that object, which the job message carries as the student's session slot (to_ref).
extern "C" int mq_npc_test(NtrCpu* c) {
    if (skip("test")) return 0;
    if (in_test || (c->r[14] & ~1u) != 0x020E3A08 || int32_t(c->r[3]) != -1) return 0;
    const uint32_t d = director(c);
    if (!d || c->r[0] != d || !coop_sync::hosting()) return 0;
    const uint32_t field = ntr_rd32(c, kField), list = ntr_rd32(c, field + kRemotes);
    uint32_t students[1 + kRemoteCount];
    uint32_t n = 0;
    students[n++] = ntr_rd32(c, d + kLocal);
    if (ram(list))
        for (uint32_t k = 0; k < kRemoteCount; k++) {
            const uint32_t o = list + k * kRemoteSize;
            if (ntr_rd32(c, o + 0x20) != 0 && ntr_rd8(c, o + 0x5A0) != 0) students[n++] = o;  // (shown, in this area)
        }
    const uint32_t pick = students[st.rotation++ % n];
    if (pick == students[0]) return 0;
    const uint32_t own = ntr_rd32(c, d + kLocal);
    ntr_wr32(c, d + kLocal, pick);
    in_test = true;
    ntr_call(c, 0x020E2C08 | 1);
    in_test = false;
    ntr_wr32(c, d + kLocal, own);
    return 1;
}

// ov001 0x020F321C (brain, brain+0x14, NPC): a full brain picks a place to wander to (two random numbers, main
// 0x020214C0) and copies it to brain+0x08, its timer brain+0x34 = 100; 0 when it found none (it tries again). The
// host's pick goes to the others; a guest takes the host's last pick for that NPC instead of rolling, or none yet.
extern "C" int mq_npc_wander(NtrCpu* c) {
    if (skip("wander")) return 0;
    if (in_wander) return 0;
    const uint32_t d = director(c);
    if (!d || !people_field(c)) return 0;
    const uint32_t brain = c->r[0], npc = c->r[2];
    const uint32_t npcs = ntr_rd32(c, d + kNpcs);
    const int n = npc_count(c, d);
    if (npc < npcs || npc >= npcs + uint32_t(n) * kNpcSize || (npc - npcs) % kNpcSize) return 0;
    const uint8_t id = ntr_rd8(c, npc + kId);
    static const bool log = std::getenv("MQ_COOP_NPC_LOG") != nullptr;
    if (coop_sync::hosting()) {
        in_wander = true;
        ntr_call(c, 0x020F321C | 1);
        in_wander = false;
        if (c->r[0]) {
            WanderMsg m{id, {}};
            for (int k = 0; k < 3; k++) m.to[k] = int32_t(ntr_rd32(c, brain + 0x14 + uint32_t(k) * 4));
            ntr::coop_send(kWander, &m, sizeof m);
            if (log) std::fprintf(stderr, "coop npc: NPC %u wanders to (%.2f, %.2f)\n", id, m.to[0] / 4096.0, m.to[2] / 4096.0);
        }
        return 1;
    }
    if (!st.wander_valid[id]) {
        c->r[0] = 0;
        return 1;
    }
    st.wander_valid[id] = false;
    for (uint32_t k = 0; k < 3; k++) {
        const uint32_t v = uint32_t(st.wander[id][k]);
        ntr_wr32(c, brain + 0x14 + k * 4, v);
        ntr_wr32(c, brain + 0x08 + k * 4, v);
    }
    ntr_wr16(c, brain + 0x34, 100);
    if (log) std::fprintf(stderr, "coop npc: NPC %u wanders to the host's (%.2f, %.2f)\n", id, st.wander[id][0] / 4096.0,
                          st.wander[id][2] / 4096.0);
    c->r[0] = 1;
    return 1;
}

// The executors (ov001 0x020E1E48, director, i) and what follows them (0x020E18F0): which NPC's commands run.
extern "C" void mq_npc_cmd(NtrCpu* c) {
    st.cur = st.director && c->r[0] == st.director && c->r[1] < uint32_t(kMaxNpcs) ? int(c->r[1]) : -1;
}
extern "C" void mq_npc_cmd_end(NtrCpu*) { st.cur = -1; }

// main 0x02005CF8 (lo, hi): a random number in [lo, hi). Called from an NPC command's roll, it comes from the job.
extern "C" int mq_npc_rng(NtrCpu* c) {
    if (st.cur < 0 || st.passthrough) return 0;
    const uint32_t lr = c->r[14] & ~1u;
    int site = -1;
    for (size_t k = 0; k < sizeof kRolls / sizeof kRolls[0]; k++)
        if (kRolls[k] == lr) site = int(k);
    if (site < 0 || !director(c)) return 0;
    const uint32_t d = st.director;
    Npc& n = st.npcs[size_t(st.cur)];
    const uint8_t id = st.cur < npc_count(c, d) ? ntr_rd8(c, npc_entry(c, d, st.cur) + kId) : 0xFF;
    int32_t lo = int32_t(c->r[0]), hi = int32_t(c->r[1]);
    if (hi < lo) std::swap(lo, hi);
    const uint32_t span = uint32_t(hi - lo);
    const uint64_t h = mix(uint64_t(id) << 48 ^ uint64_t(n.seq) << 32 ^ uint64_t(site) << 24 ^ n.rolls++);
    c->r[0] = span ? uint32_t(lo + int32_t(h % span)) : uint32_t(lo);
    return 1;
}

// Command op 2 (ov001 0x020E7918, director, i): an NPC starts a conversation with the console's own student. A job
// about another console's student waits here (op 2's "talking" status) until that console's conversation is over;
// the console of the student it is about runs it and says when it is over.
extern "C" int mq_npc_op2(NtrCpu* c) {
    if (st.passthrough) return 0;
    const uint32_t d = director(c);
    if (!d || c->r[0] != d || c->r[1] >= uint32_t(npc_count(c, d))) return 0;
    const int i = int(c->r[1]);
    Npc& n = st.npcs[size_t(i)];
    const int own = coop_sync::own_slot();
    const uint8_t id = ntr_rd8(c, npc_entry(c, d, i) + kId);
    if (n.target < 0 || n.target == own) {
        st.passthrough = true;
        ntr_call(c, kOp2 | 1);
        st.passthrough = false;
        if (c->r[0] && n.target == own) {
            const TalkDoneMsg m{id, n.seq};
            ntr::coop_send(kTalkDone, &m, sizeof m);
            n.target = -1;
        }
        return 1;
    }
    const uint32_t status = d + kCommand + uint32_t(i) * kCommandSize + kOp2Status;
    const uint32_t now = ntr_rd32(c, d + kFrames);
    if (!n.waiting) {
        n.waiting = true;
        n.wait_from = now;
        ntr_wr8(c, status, 1);
        std::fprintf(stderr, "coop npc: NPC %u waits for slot %d's conversation (job %u)\n", id, n.target, n.seq);
    }
    const bool done = n.done_valid && int16_t(n.done - n.seq) >= 0;
    if (!done && now - n.wait_from < kOp2Timeout) {
        c->r[0] = 0;
        return 1;
    }
    std::fprintf(stderr, "coop npc: NPC %u: slot %d's conversation %s\n", id, n.target, done ? "is over" : "timed out");
    ntr_wr8(c, status, 2);
    n.waiting = false;
    n.target = -1;
    c->r[0] = 1;
    return 1;
}
