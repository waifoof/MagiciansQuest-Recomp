// Magician's Quest co-op: the Gateway of the Beginning and End's destinations in a session (docs/COOP.md, "The
// Gateway's destinations together"; docs/COOP_GAME.md, item 6).
//
// What the game does (event 250 block 1, started by touching the Gateway upstairs in the dorm):
// - #18-#19: query 0x22 (t20 = 1 in a session); in a session the menu is "communications room / Exit" (999:5315)
//   and nothing else. Not a menu text tag: a script test.
// - Alone, by progress: s411 != 0 (a student the principal has not yet told about the Gateway) gets
//   "communications room / Save / Exit"; mystery 31 at stages 1-2 locks the room; query 0x96 (field+0x3EB0, the town's
//   g1262, a flag of the day) offers **Anywhere** (#166: a random warp, query 0x25's t55, to the outdoor areas 64 and
//   63, the school's hall 6, the haunted house 9, the books building 53, or map 67 - in a session the school's hall
//   instead, the game's own test at #207); query 0x45 offers **Mystery time** (#343, the Key of Mystery: query 0x48 asks
//   the field for change 2, whose end (ov001 0x020C9D8E -> 0x020C7AAC) starts the town's next mystery from
//   /script/start/obj.dat and reloads the field); mystery 32 offers **The other world** (#228: with g1010 = 0 the
//   warden's scene that moves the mystery on; then, with item 2106, the warp to map 38, where mystery 32's own scenes
//   run (event 102) and the only way back is its scene 102.3, a warp to the dorm). Every menu has Save (query 0x49,
//   field change 3: save and back to the title).
//
// What is built (only in a co-op session; message type 46 on the side channel):
// - The console whose student uses the Gateway decides. When the game's menu would offer Anywhere or The other world,
//   the session test at #19 ends the event instead (a jump to its last command), and our menu (event 90030) offers
//   "communications room / <the destination> / Exit": no Save (a visit never saves), no Mystery time (it moves the
//   town's time to the next mystery: the host's town alone, and its field change is the kind of reload a session
//   leaves out, as the day's; kept hidden). The pick runs event 250 block 1 again, its #19 now jumping to the
//   destination's own branch: the game's own walk into the Gateway, its random pick and warp.
// - The other student is asked "<name> is going through the Gateway...! Go with <name>?" (event 90031) once free; Yes
//   takes them to where the first student arrived, a step beside (event 90032: warp_map, filled in as it runs, as the
//   mysteries' joins do: events of ours, not calls between frames). A student who never left (the other world
//   without its key item) is told so.
// - Back: when the first student leaves the destination, the other one, still there, is asked "Go back with <name>?"
//   and taken to the place the first went (a town arrival point near them, warp_local 90033, or a numbered map). From
//   the other world, which has no way out of its own, the other one goes back without a question.
// - The other world is offered to a guest only past the warden's scene (g1010 != 0): that scene moves the host's
//   mystery, which a guest's writes cannot.
#include <cmath>
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

constexpr uint32_t kField = 0x020AD7EC;          // the field object (+0x3EB4 the area, +0x3EAE loading, +0x3EAF session)
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the session)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+8 running, +0x23BC start kind, +0x2254 bytes)
constexpr uint32_t kEventNote = 0x020B8AAC;      // +2 the event's block, +6 its file
constexpr uint32_t kEventCode = 0x020B8AB4;      // the event's commands
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF)
constexpr uint32_t kGetVar = 0x0201D960;         // (context, variable): a script variable's value
constexpr uint32_t kControlled = 0x020FE264;     // the actor the player controls
constexpr uint32_t kPlayerPos = 0x020B8B7C;      // x, y, z (20.12)
constexpr uint32_t kStore = 0x020AF020 + 0x2D8;  // the town's variable store (g1000 + index)
constexpr uint32_t kWarpTo = 0x020AD7F0;         // a warp's place (x, y, z, +12 the map) until the load takes it
constexpr uint32_t kDayFlags = 0x020AD7F0;       // +14: the day's Anywhere flag (g1262), copied to field+0x3EB0
constexpr uint32_t kOverlaySlot = 0x0209C988, kSlotOverlay = 0x020A309C;
constexpr uint32_t kFieldArea = 0x3EB4, kFieldLoading = 0x3EAE, kFieldInSession = 0x3EAF, kFieldAnywhere = 0x3EB0;
constexpr uint8_t kMsgType = 46;  // (40 talk, 41 new day, 42 lessons, 43 mysteries and festivals, 44 incantations)

// Event 250 block 1: the session test, and the branches it is sent to (command indexes, checked before each jump).
constexpr int kGateFile = 250, kGateBlock = 1;
constexpr int kTestIndex = 19, kMenusIndex = 24, kCommsIndex = 74, kAnywhereIndex = 166, kOtherWorldIndex = 228,
              kEndIndex = 381;
constexpr uint32_t kGateMenu = content::kGateKind, kGateAnswer = content::kGateAnswer;  // t193, t194
constexpr int kOtherWorldMap = 38;  // mystery 32's other world: no way out but its own scene
constexpr int32_t kBeside = 1 << 12;
constexpr int kFreeFrames = 30;
constexpr int kSettle = 20;             // a new area counts once the student has stood in it this long
constexpr int32_t kJump = 3 << 12;      // a move this long in a frame is being put somewhere (units)
constexpr int kOfferFrames = 60 * 60;   // an offer to go along lasts at most this long
constexpr int kTravelFrames = 40 * 60;  // the first student's walk through the Gateway, at most

enum Dest : uint8_t { kNone = 0, kAnywhere = 1, kOtherWorld = 2, kComms = 3 };
enum Msg : uint8_t {
    kLeaving = 1,  // u16 seq, u8 destination, u8 slot: the student goes through the Gateway
    kArrived = 2,  // u16 seq, u8 area, u8 building, s32 x, y, z, u8 slot: ... and is there
    kCancel = 3,   // u16 seq: ... did not go after all
    kBack = 4,     // u16 seq, u8 area, u8 building, s32 x, y, z, u8 slot: ... has left the destination for this place
    kDecline = 5,  // u16 seq: the other student won't come
};

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
bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0);
}
bool event_running(NtrCpu* c, int file, int block) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) && ntr_rd8(c, ctx + 8) != 0 && int16_t(ntr_rd16(c, kEventNote + 6)) == file &&
           int16_t(ntr_rd16(c, kEventNote + 2)) == block;
}
int area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? ntr_rd8(c, field + kFieldArea) : -1;
}
bool loaded(NtrCpu* c) {  // the field up, not loading, with the student in it
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) && ntr_rd8(c, field + kFieldLoading) == 0 && ram(ntr_rd32(c, kControlled));
}
bool building(NtrCpu* c) {
    const int8_t slot = int8_t(ntr_rd8(c, kOverlaySlot + 3));
    return slot >= 0 && slot < 8 && int8_t(ntr_rd8(c, kSlotOverlay + uint32_t(slot))) == 3;
}
bool free_to_move(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) && ntr_rd8(c, field + kFieldInSession) != 0 && ntr_rd8(c, field + kFieldLoading) == 0 &&
           ram(ntr_rd32(c, kControlled)) && !event_busy(c);
}
uint8_t local(NtrCpu* c, uint32_t var) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (var - 10000)) : 0;
}
void set_local(NtrCpu* c, uint32_t var, uint8_t v) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + (var - 10000), v);
}
bool queue(NtrCpu* c, int id) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || event_busy(c)) return false;
    if (call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) == 0) return false;
    ntr_wr8(c, ctx + 0x23BC, 2);
    return true;
}

void put16(std::vector<uint8_t>& m, uint32_t v) { m.push_back(uint8_t(v)), m.push_back(uint8_t(v >> 8)); }
void put32(std::vector<uint8_t>& m, uint32_t v) { put16(m, v & 0xFFFF), put16(m, v >> 16); }
uint32_t get16(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8; }
uint32_t get32(const uint8_t* p) { return get16(p) | get16(p + 2) << 16; }
void send(const std::vector<uint8_t>& m) { ntr::coop_send(kMsgType, m.data(), m.size(), 0); }

struct Place {
    int area = -1;
    bool building = false;
    int32_t x = 0, y = 0, z = 0;
};
// The last warp this console made: the place a warp asks for (ov001 0x020D15B4 writes it at 0x020AD7F0, x, y, z in
// 20.12 and the map at +12) is in the warp's own frame, which is not the student's position's (0x020B8B7C: in map 64,
// warp_map (5, 14) puts the student at (5, 25.85); in the dorm (15, 1) at (14.95, 10.74)). The load consumes it, so it
// is caught each frame while it is there.
struct Warp {
    int32_t x = 0, y = 0, z = 0;
    int map = -1;
    int frame = -100000;
} last_warp;
int frames = 0;

// Where this student is, as the other console's warp needs it: in a numbered map the place its last warp asked for
// (when it is that map's, and recent), else (the town: arrival points are picked by the position) the position.
Place here(NtrCpu* c) {
    Place p{area(c), building(c), int32_t(ntr_rd32(c, kPlayerPos)), int32_t(ntr_rd32(c, kPlayerPos + 4)),
            int32_t(ntr_rd32(c, kPlayerPos + 8))};
    if (p.area != 255 && last_warp.map == p.area && frames - last_warp.frame < 60 * 60)
        p.x = last_warp.x, p.y = last_warp.y, p.z = last_warp.z;
    return p;
}
void put_place(std::vector<uint8_t>& m, const Place& p) {
    m.push_back(uint8_t(p.area)), m.push_back(p.building ? 1 : 0);
    put32(m, uint32_t(p.x)), put32(m, uint32_t(p.y)), put32(m, uint32_t(p.z));
}
Place get_place(const uint8_t* d) {
    return {d[0], d[1] != 0, int32_t(get32(d + 2)), int32_t(get32(d + 6)), int32_t(get32(d + 10))};
}

// This console's student at the Gateway (U).
struct Trip {
    int phase = 0;  // 1 our menu to show, 2 menu up, 3 250.1 again (going), 4 walking through, 5 at the destination
    Dest kind = kNone;  // what our menu offers
    Dest dest = kNone;  // what was picked
    int go_index = -1;  // where #19 sends the second run
    uint16_t seq = 0;
    int origin = -1, there = -1;
    int frames = 0;
} trip;

// The other student, invited (P).
struct Follow {
    bool offer = false;     // asked to go along (kLeaving), or back (kBack)
    bool back = false;      // ... back
    uint16_t seq = 0;
    int slot = -1;
    bool asked = false, yes = false;
    bool arrived = false;   // the first student's place is known
    Place to;
    int frames = 0, answer_wait = 0;
    int there = -1;         // went along: the destination it stands in (for the way back)
    uint16_t there_seq = 0;
    int moves = 0;
} fol;

int free_frames = 0;
// Frames the student has stood in the new area since it was put there. A warp writes the field's area byte at once,
// while the old map is still up (the screen fading out) and the student still at the old place; the new map's load
// then puts the student at its arrival point: a jump of the position. So a new area counts from that jump on.
int settled = 0, settled_area = -1;
bool moved_in = false;
int32_t last_x = 0, last_z = 0;
uint16_t next_seq = 0;
bool listening = false;

// The warp to fill in (events 90032 / 90033: the command after the mark).
struct Target {
    bool on = false;
    int map = 0;  // the map (warp_map) or the arrival point (warp_local)
    int32_t x = 0, y = 0, z = 0;  // 1/128 units
} target;

struct Arrival {
    uint8_t id;
    float x, z;
};
constexpr Arrival kTownArrivals[] = {  // (the town's arrival points, as coop_myst.cpp)
    {240, 44.0f, 22.5f},  {221, 77.25f, 41.25f}, {243, 32.5f, 71.0f}, {235, 80.25f, 8.75f}, {233, 73.25f, 5.5f},
    {230, 52.5f, 8.0f},   {232, 61.25f, 5.25f},  {234, 81.75f, 15.25f}, {236, 82.0f, 22.5f}, {237, 70.0f, 20.5f},
    {238, 57.0f, 56.75f}, {239, 63.0f, 57.25f},  {206, 46.0f, 6.0f},
};

// The other student is taken to `p` (a step beside). False when it can't be reached (a building shown as the town).
bool move_to(NtrCpu* c, const Place& p) {
    target = Target{};
    target.x = (p.x + kBeside) >> 5, target.y = p.y >> 5, target.z = p.z >> 5;
    int kind = content::kEventGateWarp;
    if (p.area == 255 && p.building) return false;
    if (p.area == 255) {
        float best = 1e9f;
        for (const Arrival& a : kTownArrivals) {
            const float dx = a.x - p.x / 4096.0f, dz = a.z - p.z / 4096.0f, d = dx * dx + dz * dz;
            if (d < best) best = d, target.map = a.id;
        }
        kind = content::kEventGateEntry;
    } else {
        target.map = p.area;
    }
    target.on = true;
    if (!queue(c, kind)) {
        target.on = false;
        return false;
    }
    std::fprintf(stderr, "coop gate: to %s %d (%.1f, %.1f)\n", kind == content::kEventGateEntry ? "the town at arrival point" : "map",
                 target.map, target.x / 128.0, target.z / 128.0);
    return true;
}

// The menu of ours, if the game's would offer a destination: Anywhere (query 0x96: field+0x3EB0), else The other world
// (mystery 32; for a guest only past the warden's scene). None for a student the principal hasn't told yet (s411).
Dest menu_kind(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext), field = ntr_rd32(c, kField);
    if (!ram(ctx) || !ram(field)) return kNone;
    if (call(c, kGetVar, ctx, 411) != 0) return kNone;
    if (ntr_rd8(c, field + kFieldAnywhere) != 0) return kAnywhere;
    if (ntr_rd8(c, kStore + 0) == 32 && (coop_sync::hosting() || ntr_rd8(c, kStore + 10) != 0)) return kOtherWorld;
    return kNone;
}

// Command `n` of the running event (counting from its first; a command's size is its second byte).
uint32_t command(NtrCpu* c, int n) {
    uint32_t at = ntr_rd32(c, kEventCode);
    if (!ram(at)) return 0;
    for (int i = 0; i < n; i++) {
        const uint8_t size = ntr_rd8(c, at + 1);
        if (size < 4) return 0;
        at += size;
    }
    return at;
}
// The branches checked before a jump: their first command as the game has it.
bool branch_ok(NtrCpu* c, int n) {
    const uint32_t at = command(c, n);
    if (!at) return false;
    switch (n) {
    case kEndIndex: return ntr_rd8(c, at) == 0x42;
    case kCommsIndex: return ntr_rd8(c, at) == 0x53 && ntr_rd8(c, at + 2) == 0x26;                       // query 0x26
    case kAnywhereIndex: return ntr_rd8(c, at) == 0x1E && ntr_rd8(c, at + 4) == 2;                       // motion 2
    case kOtherWorldIndex: return ntr_rd8(c, at) == 0x38 && ntr_rd32(c, at + 4) == 1010;                 // if g1010
    case kMenusIndex: return ntr_rd8(c, at) == 0x38 && ntr_rd32(c, at + 4) == 411;                       // if s411
    default: return false;
    }
}

void reset_trip() { trip = Trip{}; }

void on_message(NtrCpu* c, uint32_t, const uint8_t* d, size_t len) {
    if (len < 3) return;
    const uint16_t seq = uint16_t(get16(d + 1));
    switch (d[0]) {
    case kLeaving:
        if (len < 5) return;
        fol = Follow{};
        fol.offer = true, fol.seq = seq, fol.slot = d[4];
        std::fprintf(stderr, "coop gate: the other student goes through the Gateway (%s)\n",
                     d[3] == kAnywhere ? "Anywhere" : "The other world");
        return;
    case kArrived:
        if (len < 18 || !fol.offer || fol.back || fol.seq != seq) return;
        fol.arrived = true, fol.to = get_place(d + 3);
        std::fprintf(stderr, "coop gate: the other student arrived in map %d (%.1f, %.1f)\n", fol.to.area,
                     fol.to.x / 4096.0, fol.to.z / 4096.0);
        return;
    case kCancel:
        if (!fol.offer || fol.seq != seq) return;
        std::fprintf(stderr, "coop gate: the other student did not go through after all\n");
        if (fol.yes) {  // (told, once free)
            set_local(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + fol.slot));
            set_local(c, kGateMenu, 0);
            queue(c, content::kEventGateNote);
        }
        fol.offer = false;
        return;
    case kBack: {
        if (len < 18 || fol.there < 0 || fol.there_seq != seq) return;
        const Place to = get_place(d + 3);
        if (area(c) != fol.there) {  // (gone already)
            fol.there = -1;
            return;
        }
        const int there = fol.there;
        const uint16_t tseq = fol.there_seq;
        fol = Follow{};
        fol.offer = true, fol.back = true, fol.seq = seq, fol.slot = d[17], fol.arrived = true, fol.to = to;
        fol.there = there, fol.there_seq = tseq;
        // (the other world has no way out of its own: back without a question)
        if (there == kOtherWorldMap) fol.asked = true, fol.yes = true;
        std::fprintf(stderr, "coop gate: the other student left map %d for map %d\n", there, to.area);
        return;
    }
    case kDecline:
        std::fprintf(stderr, "coop gate: the other student won't come along\n");
        return;
    }
}

}  // namespace

// Before each event command 0x38 (from mq_coop_gate, the command's address): the Gateway's session test (event 250
// block 1, #19 "if t20 != 1 goto #24") sends the event to our menu's end or to the picked destination; our warp
// events' mark is filled in with the place.
void mq_gate_command(NtrCpu* c, uint32_t cmd) {
    if (ntr_rd8(c, cmd) != 0x38) return;
    // our warps (events 90032 / 90033)
    if (target.on && ntr_rd32(c, cmd + 4) == content::kQueryResult && int32_t(ntr_rd32(c, cmd + 8)) == content::kGateMark) {
        const uint32_t next = cmd + ntr_rd8(c, cmd + 1);
        if (ntr_rd8(c, next) == 0x2D || ntr_rd8(c, next) == 0x2E) {
            ntr_wr8(c, next + 2, uint8_t(target.map));
            ntr_wr16(c, next + 4, uint16_t(int16_t(target.x)));
            ntr_wr16(c, next + 6, uint16_t(int16_t(target.y)));
            ntr_wr16(c, next + 8, uint16_t(int16_t(target.z)));
        }
        target.on = false;
        return;
    }
    // (test driver, MQ_GATE_KEY=<path>: while <path> exists, the other world's item test (#230 "if t0 == 0 goto #236"
    // after query 0x28 (2106), "I shouldn't touch it") passes, as for a student who has the item)
    if (static const char* key = std::getenv("MQ_GATE_KEY"); key && ntr_rd32(c, cmd + 4) == content::kQueryResult &&
        ntr_rd8(c, cmd + 2) == 0 && ntr_rd32(c, cmd + 8) == 0 && ntr_rd32(c, cmd + 12) == 236 &&
        event_running(c, kGateFile, kGateBlock)) {
        if (FILE* f = std::fopen(key, "rb")) {
            std::fclose(f);
            ntr_wr32(c, cmd + 8, 0x7FFF);
            std::fprintf(stderr, "coop gate: (test) the other world's item test passes\n");
        } else {
            ntr_wr32(c, cmd + 8, 0);
        }
        return;
    }
    // the Gateway's test: t20 (10020) != 1, then sysmsg 5315 (the session's menu)
    if (ntr_rd8(c, cmd + 2) != 1 || ntr_rd32(c, cmd + 4) != 10020 || !event_running(c, kGateFile, kGateBlock)) return;
    const uint32_t next = cmd + ntr_rd8(c, cmd + 1);
    if (ntr_rd8(c, next) != 0x67 || ntr_rd16(c, next + 2) != 5315 || command(c, kTestIndex) != cmd || !in_session(c) ||
        coop_sync::own_slot() < 0)
        return;
    int to = -1;
    if (trip.phase == 3 && trip.go_index >= 0 && branch_ok(c, trip.go_index)) {
        to = trip.go_index;
        if (trip.dest == kComms) {
            reset_trip();
        } else {
            trip.phase = 4, trip.frames = 0, trip.origin = area(c);
            // (the other world before the warden's scene is that scene, here: no one travels)
            if (trip.dest == kOtherWorld && ntr_rd8(c, kStore + 10) == 0) {
                std::fprintf(stderr, "coop gate: the other world: the warden's scene (mystery 32), no travel\n");
                reset_trip();
            } else {
                trip.seq = ++next_seq;
                std::vector<uint8_t> m = {kLeaving};
                put16(m, trip.seq);
                m.push_back(trip.dest), m.push_back(uint8_t(coop_sync::own_slot()));
                send(m);
            }
        }
        std::fprintf(stderr, "coop gate: the Gateway goes to its branch #%d\n", to);
    } else if (trip.phase == 0) {
        const Dest kind = menu_kind(c);
        if (kind != kNone && branch_ok(c, kEndIndex) && branch_ok(c, kCommsIndex) &&
            branch_ok(c, kind == kAnywhere ? kAnywhereIndex : kOtherWorldIndex)) {
            to = kEndIndex;
            trip = Trip{};
            trip.phase = 1, trip.kind = kind;
            std::fprintf(stderr, "coop gate: the Gateway offers %s: our menu\n", kind == kAnywhere ? "Anywhere" : "The other world");
        }
    }
    if (to < 0) {  // (the game's own session menu; the command as the card has it, in case an earlier run changed it)
        ntr_wr32(c, cmd + 8, 1);
        ntr_wr32(c, cmd + 12, kMenusIndex);
        return;
    }
    ntr_wr32(c, cmd + 8, 0x7FFF);  // (t20 != 0x7FFF: always)
    ntr_wr32(c, cmd + 12, uint32_t(to));
}

// Once a frame (from mq_frame_end).
void mq_gate_frame(NtrCpu* c) {
    if (!listening) {
        coop_sync::listen(kMsgType, on_message);
        listening = true;
    }
    frames++;
    if (!in_session(c) || coop_sync::own_slot() < 0) {
        if (trip.phase || fol.offer || fol.there >= 0) std::fprintf(stderr, "coop gate: out of the session\n");
        reset_trip();
        fol = Follow{};
        target.on = false;
        free_frames = 0;
        return;
    }
    // (test driver, MQ_GATE_ANYWHERE=1: the day's Anywhere flag set, as on a day the game gives it)
    if (static const bool anywhere = std::getenv("MQ_GATE_ANYWHERE") != nullptr; anywhere) {
        const uint32_t field = ntr_rd32(c, kField);
        if (ram(field)) ntr_wr8(c, field + kFieldAnywhere, 1);
        ntr_wr8(c, kDayFlags + 14, 1);
    }
    free_frames = free_to_move(c) ? free_frames + 1 : 0;
    if (const int32_t wx = int32_t(ntr_rd32(c, kWarpTo)), wy = int32_t(ntr_rd32(c, kWarpTo + 4)),
        wz = int32_t(ntr_rd32(c, kWarpTo + 8));
        wx | wy | wz)
        last_warp = {wx, wy, wz, int(ntr_rd8(c, kWarpTo + 12)), frames};
    {
        const int32_t x = int32_t(ntr_rd32(c, kPlayerPos)), z = int32_t(ntr_rd32(c, kPlayerPos + 8));
        const bool jumped = std::abs(double(x) - last_x) + std::abs(double(z) - last_z) > double(kJump);
        last_x = x, last_z = z;
        if (area(c) != settled_area) settled = 0, settled_area = area(c), moved_in = jumped;
        else if (jumped) moved_in = true, settled = 0;
        else if (moved_in && loaded(c)) settled++;
    }

    // This console's student at the Gateway.
    switch (trip.phase) {
    case 1:  // the game's event has ended at our jump: our menu
        if (event_running(c, kGateFile, kGateBlock)) break;
        if (++trip.frames > 300) {
            std::fprintf(stderr, "coop gate: our menu could not start\n");
            reset_trip();
        } else if (!event_busy(c)) {
            set_local(c, kGateMenu, trip.kind);
            set_local(c, kGateAnswer, 0);
            if (queue(c, content::kEventGateMenu)) trip.phase = 2, trip.frames = 0;
        }
        break;
    case 2:  // our menu: the answer once it has ended
        if (++trip.frames < 10 || event_busy(c)) break;
        switch (local(c, kGateAnswer)) {
        case 1: trip.dest = kComms, trip.go_index = kCommsIndex; break;
        case 2: trip.dest = trip.kind, trip.go_index = trip.kind == kAnywhere ? kAnywhereIndex : kOtherWorldIndex; break;
        default: trip.dest = kNone; break;
        }
        if (trip.dest == kNone) {
            reset_trip();
        } else if (queue(c, kGateFile * 100 + kGateBlock)) {
            trip.phase = 3, trip.frames = 0;
            std::fprintf(stderr, "coop gate: picked %s\n", trip.dest == kComms ? "the communications room" : "the destination");
        } else {
            reset_trip();
        }
        break;
    case 3:  // the Gateway again, until its test
        if (++trip.frames > 300) reset_trip();
        break;
    case 4:  // through the Gateway: arrived, or stayed
        trip.frames++;
        if (settled >= kSettle && area(c) != trip.origin && area(c) >= 0 && !event_running(c, kGateFile, kGateBlock)) {
            trip.there = area(c), trip.phase = 5;
            std::vector<uint8_t> m = {kArrived};
            put16(m, trip.seq);
            put_place(m, here(c));
            m.push_back(uint8_t(coop_sync::own_slot()));
            send(m);
            std::fprintf(stderr, "coop gate: arrived in map %d (%.1f, %.1f)\n", trip.there,
                         int32_t(ntr_rd32(c, kPlayerPos)) / 4096.0, int32_t(ntr_rd32(c, kPlayerPos + 8)) / 4096.0);
        } else if ((free_frames > kFreeFrames && area(c) == trip.origin && !event_running(c, kGateFile, kGateBlock)) ||
                   trip.frames > kTravelFrames) {
            std::vector<uint8_t> m = {kCancel};
            put16(m, trip.seq);
            send(m);
            std::fprintf(stderr, "coop gate: did not go through the Gateway\n");
            reset_trip();
        }
        break;
    case 5:  // at the destination: leaving it
        if (settled >= kSettle && area(c) != trip.there && area(c) >= 0) {
            std::vector<uint8_t> m = {kBack};
            put16(m, trip.seq);
            put_place(m, here(c));
            m.push_back(uint8_t(coop_sync::own_slot()));
            send(m);
            std::fprintf(stderr, "coop gate: left map %d for map %d\n", trip.there, area(c));
            reset_trip();
        }
        break;
    }

    // Went along: leaving the destination by oneself ends it.
    if (fol.there >= 0 && !fol.offer && settled >= kSettle && area(c) != fol.there) fol.there = -1;

    // The other student's trip: asked, then taken there.
    if (!fol.offer) return;
    fol.frames++;
    if (fol.frames > kOfferFrames) {
        std::fprintf(stderr, "coop gate: the offer to go along is gone (too late)\n");
        fol.offer = false;
        return;
    }
    if (!fol.asked) {
        if (free_frames >= kFreeFrames && trip.phase == 0) {
            set_local(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + fol.slot));
            set_local(c, kGateMenu, fol.back ? 1 : 0);
            set_local(c, kGateAnswer, 0);
            if (queue(c, content::kEventGateAsk)) {
                fol.asked = true, fol.answer_wait = 0;
                std::fprintf(stderr, "coop gate: asking to go %s\n", fol.back ? "back" : "along");
            }
        }
        return;
    }
    if (!fol.yes) {
        if (++fol.answer_wait < 20 || event_busy(c)) return;
        const uint8_t a = local(c, kGateAnswer);
        if (a == 1) {
            fol.yes = true;
            std::fprintf(stderr, "coop gate: yes, going %s\n", fol.back ? "back" : "along");
        } else {
            std::vector<uint8_t> m = {kDecline};
            put16(m, fol.seq);
            send(m);
            std::fprintf(stderr, "coop gate: no, staying\n");
            const bool back = fol.back;
            const int there = fol.there;
            const uint16_t tseq = fol.there_seq;
            fol = Follow{};
            if (back) fol.there = there, fol.there_seq = tseq;  // (asked again if they leave once more: no)
        }
        return;
    }
    // Yes: once the place is known and the student free, the warp.
    if (!fol.arrived || free_frames < kFreeFrames / 2) return;
    if (fol.moves >= 3) {
        std::fprintf(stderr, "coop gate: could not get to the other student\n");
        fol = Follow{};
        return;
    }
    if (fol.back && fol.there == kOtherWorldMap && fol.moves == 0) {
        set_local(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + fol.slot));
        set_local(c, kGateMenu, 1);
        if (!queue(c, content::kEventGateNote)) return;  // ("You go back with <name>.")
        fol.moves++;
        return;
    }
    if (area(c) == fol.to.area && (fol.to.area != 255 || building(c) == fol.to.building)) {
        // there: went along (remembered for the way back), or back
        if (!fol.back) fol.there = fol.to.area, fol.there_seq = fol.seq;
        else fol.there = -1;
        std::fprintf(stderr, "coop gate: %s in map %d\n", fol.back ? "back" : "along", fol.to.area);
        const int there = fol.there;
        const uint16_t tseq = fol.there_seq;
        fol = Follow{};
        fol.there = there, fol.there_seq = tseq;
        return;
    }
    if (!move_to(c, fol.to)) {
        std::fprintf(stderr, "coop gate: the other student's place can't be reached\n");
        fol = Follow{};
        return;
    }
    fol.moves++;
}
