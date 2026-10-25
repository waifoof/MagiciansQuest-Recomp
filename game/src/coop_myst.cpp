// Magician's Quest co-op: the weekly mysteries played together (docs/COOP.md, "Mysteries together"; docs/COOP_GAME.md,
// item 1).
//
// What the game does (event scripts 071-122, one file per mystery; the start tables of script/start; the matcher
// ov001 0x020F4A38 and its checks):
// - The week's mystery is g1000 (its number) and g1001 (its stage). A stage's scenes start from start records whose
//   date bytes (+4, +5) are the mystery and the stage (ov001 0x020F42C8 compares them with store fields 0 and 1): on
//   loading a map (mc_ss.bin, block = the map; mostly nowait set-ups that place the creature, some real scenes), on
//   walking into a trigger area (map_u_ss.bin, the selector = the area id of the map's event rects) or at an actor
//   (act_u_ss.bin, often after an incantation cast at the creature: t160/t161). The scenes advance the stage
//   themselves (set_town / add_town of g1001 and the mystery's own fields), and the day roll moves it on too (ov027
//   0x02118CD4 copies g1025 into g1001). Rewards (items, the student's own fields) are written to the student who
//   watches the scene.
// - An event, started or queued, is noted at 0x020B8AAC: +2 the block, +6 the file (main 0x0201D0CC, 0x0201CEF4); its
//   commands are loaded at *0x020B8AB4; the script context's +8 is set while it runs.
// - In a session each console ran a stage's scene only for its own student when that student walked in; the other saw
//   the student stand there. A guest's stage writes are the host's alone (coop_sync.cpp, kHostVars): not sent, undone
//   once no event runs; so a guest's scene could not advance the host's mystery, and could start again.
//
// What is built (only in a co-op session; message type 43 on the side channel):
// - A mystery scene (an event of files 071-122 that is not just a nowait set-up) starting on one console is told to
//   the other ("start": the event, the mystery and stage it started at, the area and the student's place). The other
//   student joins: at once if free and in the same area within kNear units; otherwise, once free, it is asked
//   "<name> has found something mysterious! Go and help?" (event 90016); Yes warps it beside the first student (into
//   the same map, or a step beside it in the same map) and the same scene starts there. A busy student (in an event,
//   a menu, loading) is never interrupted: the offer waits until it is free and goes when the first student's scene
//   ends (or after kOfferFrames). An offer of a stage the town has moved past is dropped, and so is a scene in a
//   student's own room (dorm rooms 22-25, apartment rooms 26-29: some mysteries visit the student there).
// - Each console runs the scene for its own student, so each gets the scene's rewards on its own record.
// - The host's console decides the stage: the host's own scenes write the town as usual. A guest's scene that it
//   started itself (not one it joined) asks the host to take its stage writes: the guest notes the writes of the
//   host's variables its scene makes (the store write, main 0x0201E070) and sends them when the scene ends
//   ("result"); the host writes them into its town (the same store write) if the town still is at the mystery and
//   stage the scene started at and the host did not play that stage's scene on its own; the town sync brings them
//   back to the guest. When the host joined the guest's scene, the host's own copy of the writes is undone first
//   (the student who started the scene decides, e.g. when an incantation cast by that student is what the scene
//   checks). A guest's writes from a scene it joined are dropped (the host's own run counts).
// - After a scene, its start record does not start it again on that console while the town is still at that stage
//   (until the host's stage arrives, or kVetoFrames): the start records' date check (ov001 0x020F42C8, mq_myst_date)
//   answers "no" for it.
// - While both students play the same scene, the other student is shown a step beside (its packets moved by
//   kBeside), as both scripts walk their students to the same spots.
//
// The school festivals and New Year's Eve (docs/COOP.md, "Festivals together"; events 500-504) use the same messages:
// a festival's scenes start by talking to its teacher (Miss Strawe's tea party 500.3, Mr. Vlad's test of courage 501.3,
// Mr. Oakley's hotpot cookoff 502.12-15, Principal Sol's rainbow snow ritual 503.3, his New Year's countdown at Lovers
// Point 504.2), and the game picks which block from the student's own progress. So the other student is not given
// the scene: it is asked "<name> is at the <festival>! Go along?" and, on Yes, taken beside the first student (the
// same moves as a mystery's join), where it talks to the teacher itself and plays its own part, judged on its own
// record. A student already there (the same place, within kNear) is not asked. A festival scene sends no result:
// the town's festival flags (g1387-1390, g1242, the test of courage's g1054 / g1070) are the host's alone.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

namespace content = mq::content;

constexpr uint32_t kField = 0x020AD7EC;          // the field object (+0x3EB4 the area shown, +0x3EAE loading)
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the session)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+8 running, +0x23BC the start kind,
                                                 // +0x2254 the flag bytes)
constexpr uint32_t kEventNote = 0x020B8AAC;      // +2 the event's block, +6 its file (main 0x0201D0CC)
constexpr uint32_t kEventCode = 0x020B8AB4;      // the event's commands (loaded by main 0x0201D158)
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kControlled = 0x020FE264;     // the actor the player controls (null outside the field)
constexpr uint32_t kPlayerPos = 0x020B8B7C;      // the local student's x, y, z (20.12)
constexpr uint32_t kTown = 0x020AF020, kTownSize = 0x993C;
constexpr uint32_t kStore = kTown + 0x2D8;       // the town's variable store (g1000 + index)
constexpr uint32_t kStoreWrite = 0x0201E070;     // (store, index, value)
constexpr uint32_t kChecksum = 0x02011D9C;       // (block, size)
constexpr uint32_t kOverlaySlot = 0x0209C988, kSlotOverlay = 0x020A309C;  // the overlay table (main 0x02009DD8)
constexpr uint32_t kFieldArea = 0x3EB4, kFieldLoading = 0x3EAE, kFieldInSession = 0x3EAF;
constexpr uint8_t kMsgType = 43;  // (40 the players' talk, 41 the new day, 42 the lessons)
constexpr int kFirstFile = 71, kLastFile = 122;  // the mysteries' event files
constexpr int kFestivalFirst = 500, kFestivalLast = 504;  // the festivals' event files (504 New Year's Eve)
constexpr uint8_t kShowFestival = content::kShowFestival;  // t155 = this + (file - 500) for "<name> is at the ..."
constexpr int kRoomsFirst = 22, kRoomsLast = 29;  // the students' own rooms (dorm 22-25, apartments 26-29)
constexpr uint32_t kAnswer = 10159;     // t159: the answer to "Go and help?" (1 yes, 2 no); no retail script reads it
constexpr uint32_t kOtherFlag = content::kTalkOther;  // t141: the other student's actor, for $503'641;
constexpr int32_t kNear = 12 << 12;     // the same area and this near: the other student joins without a question
constexpr int32_t kBeside = 1 << 12;    // the other student shown a step beside in a shared scene
constexpr int kFreeFrames = 30;         // free to move this long before a question or a join
constexpr int kOfferFrames = 60 * 60;   // an offer lasts at most this long
constexpr int kArriveFrames = 30 * 60;  // after the warp, the longest wait to stand in the scene's area
constexpr int kVetoFrames = 20 * 60;    // a scene's record does not start it again for this long (or until the stage moves)

enum Msg : uint8_t {
    kStart = 1,   // u16 seq, u16 file, u8 block, u8 mystery, u8 stage, u8 area, s32 x, y, z, u8 joined (0 started here),
                  // u8 slot, u8 in a building
    kEnd = 2,     // u16 seq, u16 file, u8 block
    kResult = 3,  // guest -> host: u16 seq, u16 file, u8 block, u8 mystery, u8 stage, u8 n, n x (u16 index, u8 value)
    kAnswerMsg = 4,  // host -> guest: u16 seq, u8 taken (1) or not (0)
    kDecline = 5, // u16 seq: the other student said no to joining
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
uint32_t session(NtrCpu* c) {
    const uint32_t game = ntr_rd32(c, kGame);
    return ram(game) ? ntr_rd32(c, game + 0xA28) : 0;
}

bool in_session(NtrCpu* c) { return cfie::coop_anytime && ntr::multiplayer_active() && ram(session(c)); }

bool log_all() {
    static const bool on = std::getenv("MQ_MYST_LOG") != nullptr;
    return on;
}

bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0);
}

int area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? ntr_rd8(c, field + kFieldArea) : -1;
}

// In a building (overlay 3 loaded, as coop_connect.cpp tests it) rather than the town or an outdoor map (overlay 2).
// The field's area byte is the map for most places (14 the dorm, 0 the classroom, 49 Lovers Point, 255 the town) but
// some buildings show 255 too (the café): the two together tell places apart.
bool building(NtrCpu* c) {
    const int8_t slot = int8_t(ntr_rd8(c, kOverlaySlot + 3));
    return slot >= 0 && slot < 8 && int8_t(ntr_rd8(c, kSlotOverlay + uint32_t(slot))) == 3;
}

bool mystery_file(int file) { return file >= kFirstFile && file <= kLastFile; }

// A festival's scenes with its teacher (the talks the game starts at the teacher, and what they lead to); not the
// announcements in class (block 1: coop_class.cpp holds them for both), the set-ups (2, 10, 11) or the resets (99).
bool festival_scene(int file, int block) {
    switch (file) {
    case 500: case 501: case 503: return block == 3;
    case 502: return block >= 12 && block <= 15;
    case 504: return block == 2;
    default: return false;
    }
}
bool festival_file(int file) { return file >= kFestivalFirst && file <= kFestivalLast; }

uint8_t mystery(NtrCpu* c) { return ntr_rd8(c, kStore + 0); }
uint8_t stage(NtrCpu* c) { return ntr_rd8(c, kStore + 1); }

void put16(std::vector<uint8_t>& m, uint32_t v) { m.push_back(uint8_t(v)), m.push_back(uint8_t(v >> 8)); }
void put32(std::vector<uint8_t>& m, uint32_t v) { put16(m, v & 0xFFFF), put16(m, v >> 16); }
uint32_t get16(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8; }
uint32_t get32(const uint8_t* p) { return get16(p) | get16(p + 2) << 16; }

// The event's commands, just loaded: a set-up only (nowait_begin, its commands, nowait_end, end) is not a scene.
bool just_setup(NtrCpu* c) {
    const uint32_t p = ntr_rd32(c, kEventCode);
    if (!ram(p) || ntr_rd8(c, p) != 0x57) return false;
    uint32_t at = p;
    for (int n = 0; n < 400; n++) {
        const uint8_t op = ntr_rd8(c, at), size = ntr_rd8(c, at + 1);
        if (size < 2) return false;
        if (op == 0x58) return ntr_rd8(c, at + size) == 0x42;
        at += size;
    }
    return false;
}

struct Scene {
    bool on = false;
    int file = -1, block = -1;
    uint8_t mys = 0, stg = 0;
    uint16_t seq = 0;      // ours (started here), or the other's (joined)
    bool joined = false;   // started by us for the other student's scene
    bool guest_join = false;  // host: joined a guest's scene (its writes are the guest's to decide)
    int frames = 0;
    bool sent = false;
    std::map<uint16_t, uint8_t> writes;  // guest, its own scene: the host's variables it wrote (store index, value)
    std::vector<uint8_t> store_before;   // host, joining a guest's scene: the store as it was
};

struct Offer {
    bool on = false;
    uint16_t seq = 0;
    int file = -1, block = -1;
    uint8_t mys = 0, stg = 0;
    int area = -1;
    bool building = false;
    int32_t x = 0, y = 0, z = 0;
    int frames = 0;
    bool asked = false;   // "Go and help?" queued
    bool festival = false;  // a festival's scene: the student goes there and plays its own part (not given the scene)
    bool ended = false;   // the other's scene has ended (a late Yes still plays it, if the stage is still that one)
};

struct Join {
    int step = 0;  // 1 going to the scene first, 2 at once (near)
    int frames = 0;
    Offer o;
    int moves = 0;  // events queued to go there (a warp, then a step beside)
};

struct Veto {
    int file = -1, block = -1;
    uint8_t mys = 0, stg = 0;
    int until = 0;
};

struct Done {  // host: a scene it played (its own or joined), for a guest's result
    int file = -1, block = -1;
    uint8_t mys = 0, stg = 0;
    bool own = false;           // started on the host itself
    uint16_t guest_seq = 0;     // joined: the guest's scene
    std::vector<uint8_t> store_before;
    int frames = 0;
};

struct Pending {  // host: a guest's result waiting for the host's own run of that scene to end
    bool on = false;
    uint32_t from = 0;
    std::vector<uint8_t> msg;
};

struct State {
    int frames = 0;
    int free = 0;
    uint16_t seq = 0;
    Scene local;
    Offer offer;
    Join join;
    std::vector<Veto> vetoes;
    std::vector<Done> done;
    Pending pending;
    // the other student's scene, as it told us
    int other_slot = -1;
    int other_seen = -1000000;
    int other_file = -1, other_block = -1;
    bool other_on = false;
    uint8_t pre_mys = 0, pre_stg = 0;  // the town's mystery and stage while no event ran
    bool listening = false;
    int answer_wait = 0;  // frames since "Go and help?" was queued
    bool late = false;    // the offer went while its question was up
} st;

uint32_t host_node() {
    uint32_t nodes[8];
    const size_t n = ntr::coop_peers(nodes, 8);
    return n ? nodes[0] : 0;  // (two players: the other console)
}

void send(const std::vector<uint8_t>& m, uint32_t to = 0) { ntr::coop_send(kMsgType, m.data(), m.size(), to); }

void send_start(NtrCpu* c, const Scene& s) {
    std::vector<uint8_t> m = {kStart};
    put16(m, s.seq), put16(m, uint32_t(s.file));
    m.push_back(uint8_t(s.block)), m.push_back(s.mys), m.push_back(s.stg), m.push_back(uint8_t(area(c)));
    put32(m, ntr_rd32(c, kPlayerPos)), put32(m, ntr_rd32(c, kPlayerPos + 4)), put32(m, ntr_rd32(c, kPlayerPos + 8));
    m.push_back(s.joined ? 1 : 0);
    m.push_back(uint8_t(coop_sync::own_slot()));
    m.push_back(building(c) ? 1 : 0);
    send(m);
}

void send_end(const Scene& s) {
    std::vector<uint8_t> m = {kEnd};
    put16(m, s.seq), put16(m, uint32_t(s.file));
    m.push_back(uint8_t(s.block));
    send(m);
}

void veto(int file, int block, uint8_t mys, uint8_t stg) {
    st.vetoes.erase(std::remove_if(st.vetoes.begin(), st.vetoes.end(),
                                   [&](const Veto& v) { return v.file == file && v.block == block; }),
                    st.vetoes.end());
    st.vetoes.push_back({file, block, mys, stg, st.frames + kVetoFrames});
}

std::vector<uint8_t> read_store(NtrCpu* c) {
    std::vector<uint8_t> v(0x160);
    for (uint32_t i = 0; i < v.size(); i++) v[i] = ntr_rd8(c, kStore + i);
    return v;
}

// Host: a guest's result written into the town (with the host's own copy of a joined run undone first).
void apply_result(NtrCpu* c, uint32_t from, const uint8_t* d, size_t len) {
    const uint16_t seq = uint16_t(get16(d + 1));
    const int file = int(get16(d + 3)), block = d[5];
    const uint8_t mys = d[6], stg = d[7], n = d[8];
    if (len < 9u + 3u * n) return;
    bool taken = false;
    const char* why = nullptr;
    const Done* joined = nullptr;
    for (const Done& dn : st.done) {
        if (dn.file != file || dn.block != block || dn.mys != mys || dn.stg != stg) continue;
        if (dn.own) why = "the host played this stage's scene itself";
        else if (dn.guest_seq == seq) joined = &dn;
    }
    // (the host's own run joined the guest's scene: its copy of the writes undone, so the guest's decide)
    std::string undone;
    if (!why && joined && !joined->store_before.empty()) {
        const std::vector<uint8_t> now = read_store(c);
        for (uint32_t i = 0; i < now.size(); i++) {
            const uint8_t m = coop_sync::host_var_bits(i);
            if (!m || !((now[i] ^ joined->store_before[i]) & m)) continue;
            ntr_wr8(c, kStore + i, uint8_t((now[i] & ~m) | (joined->store_before[i] & m)));
            undone += " +" + std::to_string(i);
        }
    }
    if (!why && (mystery(c) != mys || stage(c) != stg)) why = "the town has moved on";
    if (!why) {
        std::string w;
        for (int i = 0; i < n; i++) {
            const uint16_t index = uint16_t(get16(d + 9 + 3 * i));
            const uint8_t value = d[9 + 3 * i + 2];
            call(c, kStoreWrite, kStore, index, value);
            w += " g" + std::to_string(1000 + index) + "=" + std::to_string(value);
        }
        call(c, kChecksum, kTown, kTownSize);
        taken = true;
        std::fprintf(stderr, "coop myst: the guest's scene %d (mystery %d stage %d) moves the town:%s%s%s (stage now %d)\n",
                     file * 100 + block, mys, stg, n ? w.c_str() : " (nothing)",
                     undone.empty() ? "" : "; the host's joined copy undone at", undone.c_str(), stage(c));
    } else {
        if (!undone.empty()) call(c, kChecksum, kTown, kTownSize);
        std::fprintf(stderr, "coop myst: the guest's scene %d (mystery %d stage %d) not taken: %s\n", file * 100 + block,
                     mys, stg, why);
    }
    std::vector<uint8_t> m = {kAnswerMsg};
    put16(m, seq);
    m.push_back(taken ? 1 : 0);
    send(m, from);
}

void on_message(NtrCpu* c, uint32_t from, const uint8_t* d, size_t len) {
    if (len < 3) return;
    const uint16_t seq = uint16_t(get16(d + 1));
    switch (d[0]) {
    case kStart: {
        if (len < 24) return;  // (+21 joined, +22 slot, +23 building)
        const int file = int(get16(d + 3)), block = d[5];
        st.other_file = file, st.other_block = block, st.other_on = true;
        st.other_seen = st.frames;
        st.other_slot = d[22];
        if (d[21]) {  // (the other joined: our scene, or one of its own)
            std::fprintf(stderr, "coop myst: the other student joined scene %d\n", file * 100 + block);
            return;
        }
        if (d[8] >= kRoomsFirst && d[8] <= kRoomsLast) {  // (a scene in that student's own room: theirs alone)
            std::fprintf(stderr, "coop myst: the other student's scene %d is in their room: not offered\n", file * 100 + block);
            return;
        }
        Offer& o = st.offer;
        o = Offer{};
        o.on = true, o.seq = seq, o.file = file, o.block = block, o.mys = d[6], o.stg = d[7], o.area = d[8];
        o.x = int32_t(get32(d + 9)), o.y = int32_t(get32(d + 13)), o.z = int32_t(get32(d + 17));
        o.building = d[23] != 0;
        o.festival = festival_file(file);
        o.frames = 0;
        std::fprintf(stderr, "coop myst: the other student's scene %d started (mystery %d stage %d, area %d, at %.1f, %.1f)\n",
                     file * 100 + block, o.mys, o.stg, o.area, o.x / 4096.0, o.z / 4096.0);
        return;
    }
    case kEnd:
        st.other_on = false;
        st.other_seen = st.frames;
        if (st.offer.on && st.offer.seq == seq) st.offer.ended = true;
        if (log_all()) std::fprintf(stderr, "coop myst: the other student's scene %d ended\n", int(get16(d + 3)) * 100 + d[5]);
        return;
    case kResult:
        if (!coop_sync::hosting() || len < 9) return;
        // (the host still runs its joined copy of this scene: taken when it ends)
        if (st.local.on && st.local.guest_join && st.local.seq == seq) {
            st.pending.on = true, st.pending.from = from, st.pending.msg.assign(d, d + len);
            std::fprintf(stderr, "coop myst: the guest's result waits for the host's copy of the scene to end\n");
            return;
        }
        apply_result(c, from, d, len);
        return;
    case kAnswerMsg:
        if (len >= 4) std::fprintf(stderr, "coop myst: the host %s our scene's stage\n", d[3] ? "took" : "did not take");
        return;
    case kDecline:
        std::fprintf(stderr, "coop myst: the other student won't join this time\n");
        return;
    }
}

bool queue(NtrCpu* c, int id, uint8_t kind) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || event_busy(c)) return false;
    if (call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) == 0) return false;
    ntr_wr8(c, ctx + 0x23BC, kind);
    return true;
}

bool free_to_move(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) && ntr_rd8(c, field + kFieldInSession) != 0 && ntr_rd8(c, field + kFieldLoading) == 0 &&
           ram(ntr_rd32(c, kControlled)) && !event_busy(c);
}

bool same_place(NtrCpu* c, const Offer& o) { return area(c) == o.area && building(c) == o.building; }
// Another place can be warped to when its map is known: the town (255, not a building) or a numbered area.
bool reachable(const Offer& o) { return o.area != 255 || !o.building; }

int32_t distance(NtrCpu* c, const Offer& o) {
    const double dx = double(int32_t(ntr_rd32(c, kPlayerPos)) - o.x), dz = double(int32_t(ntr_rd32(c, kPlayerPos + 8)) - o.z);
    return int32_t(std::sqrt(dx * dx + dz * dz));
}

// The joining student goes to the other: into its map, then a step beside it. Done as events of ours, as the game's
// own scripts move students, not by calls between frames: a map warp made between frames into the town ended the
// game in a session (the touch handling ran into the town's overlay before it was loaded, ov002 0x02102564), and so
// did event command 0x2D (warp_map) to the town; no retail script warps to the town by map, they arrive at one of its
// numbered arrival points (command 0x2E, warp_local, as doors do). So:
// - into the town (map 255): event 90019, warp_local to the town's arrival point nearest the other student;
// - into another numbered map (Lovers Point 49, the outdoor areas, the school's rooms, the dorm 14): event 90017,
//   warp_map to that map at the other student's place;
// - within the same map, too far: event 90018, actor_set_pos of the student a step beside the other (as scenes do).
// The command after each event's mark is filled in from the target when it runs (mq_myst_command).
struct Target {
    bool on = false;
    int kind = 0;  // kEventMystWarp, kEventMystEntry or kEventMystPlace
    int map = 0;   // the map (warp_map) or the arrival point (warp_local)
    int32_t x = 0, y = 0, z = 0;  // 1/128 units
} target;

// The town's arrival points (start/event rects of kind 253 on the town's layout 0, ov001 0x020F53E4) used to reach a
// place in it: id, x, z (units).
struct Arrival {
    uint8_t id;
    float x, z;
};
constexpr Arrival kTownArrivals[] = {
    {240, 44.0f, 22.5f},  {221, 77.25f, 41.25f}, {243, 32.5f, 71.0f}, {235, 80.25f, 8.75f}, {233, 73.25f, 5.5f},
    {230, 52.5f, 8.0f},   {232, 61.25f, 5.25f},  {234, 81.75f, 15.25f}, {236, 82.0f, 22.5f}, {237, 70.0f, 20.5f},
    {238, 57.0f, 56.75f}, {239, 63.0f, 57.25f},  {206, 46.0f, 6.0f},
};

bool move_toward(NtrCpu* c, const Offer& o) {
    target = Target{};
    target.x = (o.x + kBeside) >> 5, target.y = o.y >> 5, target.z = o.z >> 5;
    const char* how = nullptr;
    if (same_place(c, o)) {
        target.kind = content::kEventMystPlace;
        how = "a step beside the other student";
    } else if (o.area == 255 && !o.building) {
        float best = 1e9f;
        for (const Arrival& a : kTownArrivals) {
            const float dx = a.x - o.x / 4096.0f, dz = a.z - o.z / 4096.0f, d = dx * dx + dz * dz;
            if (d < best) best = d, target.map = a.id;
        }
        target.kind = content::kEventMystEntry;
        how = "into the town, at the arrival point nearest the other student";
    } else {
        target.kind = content::kEventMystWarp;
        target.map = o.area;
        how = "to the other student's map";
    }
    target.on = true;
    if (!queue(c, target.kind, 2)) {
        target.on = false;
        return false;
    }
    std::fprintf(stderr, "coop myst: %s (map %d, %.1f, %.1f)\n", how, target.kind == content::kEventMystPlace ? area(c) : target.map,
                 target.x / 128.0, target.z / 128.0);
    return true;
}

void local_started(NtrCpu* c, int file, int block) {
    Scene& s = st.local;
    s = Scene{};
    s.on = true, s.file = file, s.block = block;
    s.mys = st.pre_mys, s.stg = st.pre_stg;
    // (our join of the other's scene: the event we queued, or the same scene started here by the map's own record)
    if (st.join.step && st.join.o.file == file) {
        s.joined = true;
        s.seq = st.join.o.seq;
        s.mys = st.join.o.mys, s.stg = st.join.o.stg;
        st.join = Join{};
        st.offer.on = false;
    } else if (st.offer.on && st.offer.file == file && st.offer.block == block && st.offer.mys == s.mys &&
               st.offer.stg == s.stg && !st.offer.ended) {
        s.joined = true;  // (walked into the same scene while the other's runs: one scene, played together)
        s.seq = st.offer.seq;
        st.offer.on = false;
    } else {
        s.seq = ++st.seq;
    }
    s.guest_join = s.joined && coop_sync::hosting();
    if (s.guest_join) s.store_before = read_store(c);
    send_start(c, s);
    std::fprintf(stderr, "coop myst: scene %d (mystery %d stage %d) %s here, area %d\n", file * 100 + block, s.mys, s.stg,
                 s.joined ? "joined" : "started", area(c));
}

void local_ended(NtrCpu* c) {
    Scene& s = st.local;
    send_end(s);
    veto(s.file, s.block, s.mys, s.stg);
    if (coop_sync::hosting()) {
        Done d;
        d.file = s.file, d.block = s.block, d.mys = s.mys, d.stg = s.stg, d.own = !s.joined, d.guest_seq = s.joined ? s.seq : 0;
        d.store_before = s.store_before;
        d.frames = st.frames;
        st.done.push_back(d);
        if (st.done.size() > 8) st.done.erase(st.done.begin());
    } else if (!s.joined && mystery_file(s.file)) {  // (a guest's own scene: its stage writes go to the host)
        std::vector<uint8_t> m = {kResult};
        put16(m, s.seq), put16(m, uint32_t(s.file));
        m.push_back(uint8_t(s.block)), m.push_back(s.mys), m.push_back(s.stg);
        m.push_back(uint8_t(std::min<size_t>(s.writes.size(), 255)));
        std::string w;
        int k = 0;
        for (const auto& [index, value] : s.writes) {
            if (k++ == 255) break;
            put16(m, index), m.push_back(value);
            w += " g" + std::to_string(1000 + index) + "=" + std::to_string(value);
        }
        send(m, host_node());
        std::fprintf(stderr, "coop myst: our scene %d ended: its stage writes to the host:%s\n", s.file * 100 + s.block,
                     w.empty() ? " (none)" : w.c_str());
    }
    std::fprintf(stderr, "coop myst: scene %d ended here\n", s.file * 100 + s.block);
    s = Scene{};
    // (the host: a guest's result that waited for this run)
    if (st.pending.on && coop_sync::hosting()) {
        st.pending.on = false;
        apply_result(c, st.pending.from, st.pending.msg.data(), st.pending.msg.size());
    }
}

}  // namespace

// Before a store write (main 0x0201E070; r0 the store, r1 the index, r2 the value), from mq_var_store: a guest's own
// scene's writes of the host's variables, noted for its result.
void mq_myst_store(NtrCpu* c) {
    if (!st.local.on || st.local.joined || !mystery_file(st.local.file) || c->r[0] != kStore || coop_sync::hosting() ||
        coop_sync::own_slot() < 0)
        return;
    if (!coop_sync::host_var(1000 + c->r[1])) return;
    st.local.writes[uint16_t(c->r[1])] = uint8_t(c->r[2]);
}

// ov001 0x020F42C8 (the start records' date check, the matcher in r0, its record at +0x1C), instead: a scene this
// console has just played, at the stage it is still at, does not start again (until the host's stage arrives).
extern "C" int mq_myst_date(NtrCpu* c) {
    if (log_all()) {  // (research: the hook is reached, the records checked)
        static int calls = 0;
        if (++calls == 1 || calls % 5000 == 0) std::fprintf(stderr, "coop myst: start records' date checks: %d\n", calls);
    }
    if (st.vetoes.empty() || !in_session(c)) return 0;
    const uint32_t rec = ntr_rd32(c, c->r[0] + 0x1C);
    if (!ram(rec)) return 0;
    const int file = int16_t(ntr_rd16(c, rec + 2)), block = ntr_rd8(c, rec + 1);
    for (const Veto& v : st.vetoes) {
        if (v.file != file || v.block != block || st.frames > v.until) continue;
        if (mystery(c) != v.mys || stage(c) != v.stg) continue;
        if (log_all()) std::fprintf(stderr, "coop myst: scene %d not started again (just played at this stage)\n", file * 100 + block);
        c->r[0] = 0;
        return 1;
    }
    return 0;
}

// From mq_coop_remote (main 0x02031404, a remote player's update): in a scene both students play, the other student a
// step beside (both scripts walk their students to the same places).
void mq_myst_remote(NtrCpu* c, uint32_t player) {
    if (!st.local.on || !st.other_on || st.other_file != st.local.file || st.other_block != st.local.block) return;
    if (area(c) == 0) return;  // (the classroom has its own seats: coop_class.cpp)
    for (uint32_t i = 0; i < 4; i++) {
        const uint32_t p = player + 0xE4 + i * 16;
        const uint8_t type = ntr_rd8(c, p);
        if ((type != 0x0F && type != 0x11) || ntr_rd8(c, p + 2) != uint8_t(area(c))) continue;
        ntr_wr32(c, p + 4, ntr_rd32(c, p + 4) + uint32_t(kBeside));
    }
}

// Once a frame (from mq_frame_end).
void mq_myst_frame(NtrCpu* c) {
    if (!st.listening) {
        coop_sync::listen(kMsgType, on_message);
        st.listening = true;
    }
    st.frames++;
    if (!in_session(c) || coop_sync::own_slot() < 0) {
        if (st.local.on || st.offer.on || st.join.step) std::fprintf(stderr, "coop myst: out of the session\n");
        const int frames = st.frames;
        const uint16_t seq = st.seq;
        st = State{};
        st.frames = frames, st.seq = seq, st.listening = true;
        return;
    }
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    // (test driver, MQ_MYST_GOTO=<path>: "<map> <x> <z> <y>" (units) written there sends the student to that place
    // the way a join does (event 90017, a warp_map), and the file is removed. Map warps made between frames into the
    // town end the game in a session; this one is an event's.)
    if (static const char* go = std::getenv("MQ_MYST_GOTO"); go && !event_busy(c)) {
        if (FILE* f = std::fopen(go, "rb")) {
            int map = 0;
            double x = 0, z = 0, y = 0;
            // ("p <x> <z> [<y>]": a step within this map, actor_set_pos as a join's 90018)
            char kind = 0;
            if (std::fscanf(f, " %c", &kind) == 1 && kind == 'p') {
                const int got = std::fscanf(f, "%lf %lf %lf", &x, &z, &y);
                std::fclose(f);
                std::remove(go);
                if (got >= 2) {
                    target = Target{true, content::kEventMystPlace, 0, int32_t(x * 128), int32_t(y * 128), int32_t(z * 128)};
                    if (!queue(c, content::kEventMystPlace, 2)) target.on = false;
                    std::fprintf(stderr, "coop myst: test step to (%.1f, %.1f, %.1f)%s\n", x, y, z, target.on ? "" : " refused");
                }
                f = nullptr;
            } else {
                std::rewind(f);
            }
            const int got = f ? std::fscanf(f, "%d %lf %lf %lf", &map, &x, &z, &y) : 0;
            if (f) std::fclose(f), std::remove(go);
            if (f && got >= 3) {
                // ("e <arrival point>": warp_local, as into the town)
                const int kind = got == 3 && x == 0 && z == 0 ? content::kEventMystEntry : content::kEventMystWarp;
                target = Target{true, kind, map, int32_t(x * 128), int32_t(y * 128), int32_t(z * 128)};
                if (!queue(c, kind, 2)) target.on = false;
                std::fprintf(stderr, "coop myst: test warp to map %d (%.1f, %.1f, %.1f)%s\n", map, x, y, z,
                             target.on ? "" : " refused");
            }
        }
    }
    const bool running = ram(ctx) && ntr_rd8(c, ctx + 8) != 0;
    const int file = int16_t(ntr_rd16(c, kEventNote + 6)), block = int16_t(ntr_rd16(c, kEventNote + 2));
    if (!running && !event_busy(c)) st.pre_mys = mystery(c), st.pre_stg = stage(c);
    if (log_all()) {  // (research: every event start, as the note says it)
        static int seen_file = -1, seen_block = -1;
        static bool seen_running = false;
        if (running && (!seen_running || file != seen_file || block != seen_block)) {
            const uint32_t code = ntr_rd32(c, kEventCode);
            std::fprintf(stderr, "coop myst: event %d starts (first command %02x, mystery %d stage %d, area %d)\n",
                         file * 100 + block, ram(code) ? ntr_rd8(c, code) : 0xFFu, mystery(c), stage(c), area(c));
        }
        seen_running = running, seen_file = file, seen_block = block;
    }

    // This console's scene: started, or ended (or another event in its place).
    if (st.local.on && (!running || file != st.local.file || block != st.local.block)) local_ended(c);
    if (running && !st.local.on && block >= 0 &&
        ((mystery_file(file) && block != 99) || festival_scene(file, block))) {
        static int last_file = -1, last_block = -1, last_frame = -100;
        const bool same = file == last_file && block == last_block && st.frames - last_frame <= 1;
        last_file = file, last_block = block, last_frame = st.frames;
        if (!same) {
            if (just_setup(c)) {
                if (log_all()) std::fprintf(stderr, "coop myst: set-up %d (not a scene)\n", file * 100 + block);
            } else {
                local_started(c, file, block);
            }
        }
    }
    if (st.local.on) st.local.frames++;

    // The vetoes end when the stage moves or their time is up.
    st.vetoes.erase(std::remove_if(st.vetoes.begin(), st.vetoes.end(),
                                   [&](const Veto& v) {
                                       return st.frames > v.until || mystery(c) != v.mys || stage(c) != v.stg;
                                   }),
                    st.vetoes.end());
    // (the other student's scene still runs: its veto lasts)
    for (Veto& v : st.vetoes)
        if (st.other_on && st.other_file == v.file && st.other_block == v.block) v.until = std::max(v.until, st.frames + 60);

    st.free = free_to_move(c) ? st.free + 1 : 0;

    // A join under way: warped, waiting to stand in the scene's area, then its scene.
    if (st.join.step) {
        Join& j = st.join;
        j.frames++;
        if (mystery(c) != j.o.mys || stage(c) != j.o.stg) {
            std::fprintf(stderr, "coop myst: the town has moved past the scene we were joining\n");
            j = Join{};
        } else if (j.frames > kArriveFrames) {
            std::fprintf(stderr, "coop myst: could not reach the other student's scene\n");
            j = Join{};
        } else if (j.step == 1 && st.free >= kFreeFrames / 2 && (!same_place(c, j.o) || distance(c, j.o) > kNear) &&
                   !(j.o.festival && same_place(c, j.o) && distance(c, j.o) <= 2 * kNear)) {
            if (j.moves >= 3) {
                std::fprintf(stderr, "coop myst: could not get beside the other student\n");
                j = Join{};
            } else if (move_toward(c, j.o)) {
                j.moves++;
            }
        } else if (j.o.festival && same_place(c, j.o) && distance(c, j.o) <= 2 * kNear && st.free >= kFreeFrames / 2) {
            // (a festival: the student talks to the teacher itself; the game picks its own part)
            std::fprintf(stderr, "coop myst: beside the other student at festival scene %d\n", j.o.file * 100 + j.o.block);
            j = Join{};
        } else if (!j.o.festival && same_place(c, j.o) && st.free >= kFreeFrames / 2) {
            if (queue(c, j.o.file * 100 + j.o.block, 4))
                std::fprintf(stderr, "coop myst: scene %d queued (joining)\n", j.o.file * 100 + j.o.block);
        }
    }

    // A Yes to an offer that has gone meanwhile (the scene is over, the stage moved): "<name> has already worked it out."
    if (st.late && !event_busy(c) && ++st.answer_wait > 20) {
        const uint8_t a = ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (kAnswer - 10000)) : 0;
        if (a != 1 || queue(c, content::kEventMystOver, 2)) st.late = false;
    }

    // An offer of the other's scene.
    Offer& o = st.offer;
    if (o.on) {
        o.frames++;
        const bool stale = mystery(c) != o.mys || stage(c) != o.stg;
        if (stale || o.frames > kOfferFrames || (o.ended && !o.asked) || st.local.on) {
            if (log_all() || stale) std::fprintf(stderr, "coop myst: the offer of scene %d is gone (%s)\n", o.file * 100 + o.block,
                                                 stale ? "the stage moved" : st.local.on ? "a scene here" : o.ended ? "it ended" : "too late");
            if (o.asked) st.late = true;  // (the question may still be up: a Yes then hears it is over)
            o.on = false;
        } else if (o.asked) {
            // "Go and help?": the answer once the question's event has ended
            if (++st.answer_wait > 20 && !event_busy(c)) {
                const uint8_t a = ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (kAnswer - 10000)) : 0;
                if (a == 1) {
                    std::fprintf(stderr, "coop myst: yes, joining scene %d\n", o.file * 100 + o.block);
                    st.join = Join{1, 0, o};
                    o.on = false;
                } else if (a == 2 || st.answer_wait > 600) {
                    std::vector<uint8_t> m = {kDecline};
                    put16(m, o.seq);
                    send(m);
                    std::fprintf(stderr, "coop myst: no, not joining scene %d\n", o.file * 100 + o.block);
                    o.on = false;
                }
            }
        } else if (st.free >= kFreeFrames && !st.join.step) {
            if (o.festival && same_place(c, o) && distance(c, o) <= kNear) {
                std::fprintf(stderr, "coop myst: already beside the other student at festival scene %d\n", o.file * 100 + o.block);
                o.on = false;
            } else if (same_place(c, o) && distance(c, o) <= kNear) {
                std::fprintf(stderr, "coop myst: near the other student: joining scene %d at once\n", o.file * 100 + o.block);
                st.join = Join{2, 0, o};
                o.on = false;
            } else if (!same_place(c, o) && !reachable(o)) {
                std::fprintf(stderr, "coop myst: the other student's scene %d is in a building this console can't warp to: not offered\n",
                             o.file * 100 + o.block);
                o.on = false;
            } else if (!o.ended) {
                if (ram(ctx)) {
                    ntr_wr8(c, ctx + 0x2254 + (kAnswer - 10000), 0);
                    if (st.other_slot >= 0) ntr_wr8(c, ctx + 0x2254 + (kOtherFlag - 10000), uint8_t(content::kOtherPlayer0 + st.other_slot));
                    // (which question: a mystery's, or the festival's)
                    ntr_wr8(c, ctx + 0x2254 + (content::kTalkShow - 10000),
                            o.festival ? uint8_t(kShowFestival + (o.file - kFestivalFirst)) : 0);
                }
                if (queue(c, content::kEventMystAsk, 2)) {
                    o.asked = true;
                    st.answer_wait = 0;
                    std::fprintf(stderr, "coop myst: asking to join scene %d\n", o.file * 100 + o.block);
                }
            }
        }
    }
}

// Before each event command 0x38 (from mq_coop_gate, the command's address): our events 90017 / 90018 mark the command
// after the test with kMystMark; it is filled in with the place to join (a warp_map's map and place, or
// actor_set_pos's place for the student).
void mq_myst_command(NtrCpu* c, uint32_t cmd) {
    if (!target.on || ntr_rd8(c, cmd) != 0x38 || ntr_rd32(c, cmd + 4) != content::kQueryResult ||
        int32_t(ntr_rd32(c, cmd + 8)) != content::kMystMark)
        return;
    const uint32_t next = cmd + ntr_rd8(c, cmd + 1);
    if (ntr_rd8(c, next) == 0x2D || ntr_rd8(c, next) == 0x2E) {
        ntr_wr8(c, next + 2, uint8_t(target.map));
    } else if (ntr_rd8(c, next) == 0x02) {
        ntr_wr16(c, next + 2, uint16_t(content::kPlayer));
    } else {
        return;
    }
    ntr_wr16(c, next + 4, uint16_t(int16_t(target.x)));
    ntr_wr16(c, next + 6, uint16_t(int16_t(target.y)));
    ntr_wr16(c, next + 8, uint16_t(int16_t(target.z)));
    target.on = false;
}
