// Magician's Quest co-op: the daily lessons taken together (docs/COOP.md, "Lessons together"; docs/COOP_GAME.md,
// item 2).
//
// What the game does (found with RAM dumps of both consoles and gdb on the recompiled functions):
// - A lesson starts when a student enters the classroom (map 0, the field's area +0x3EB4 = 0) and a start record of
//   /script/start/mc_ss.bin block 0 matches the lesson picked from Principal Sol's menu (g1017 = 1..96, kept on each
//   console in a session). Each console runs its own lesson. Every classroom scene (the lessons 011, 021, 031, 041,
//   042, homeroom 551 / 555, the festivals' announcements 500-503) seats the local student at (3.5, 0, 9.0) units
//   and its three classmates (actors t131-t133) at the other desks; in the lessons t131 sits beside the student at
//   (6.5, 9.0).
// - The other student is not hidden during a scene: its console seats it at (3.5, 9.0) too, and its packets (type
//   0x0F or 0x11: +2 area, +4 x, +8 z, 20.12 units) put it there on this console, inside the local student: the two
//   overlap.
// - In a session the classmates were missing from every classroom scene: the event registers them (actor_init, ov001
//   0x020D03C8: a list of four at field+0x3EA4) and moves them in, then the building's placement (ov003 0x020FEB54)
//   asks ov001 0x020C67F8 (field, n) to keep at most n of the people in the room, n being field+0x3EAC; in a
//   session n was 0 (alone: 3 or 4), so they were all sent home (their own room, area 28-35) and the homeroom's
//   farewell (551) waited forever for the classmate who leaves.
//
// What is built (only in a co-op session):
// - The classroom keeps the scene's classmates (mq_class_summon: n is at least the number the scene registered).
// - The two students start their lessons together: a console whose lesson is about to start when the student walks
//   into the classroom holds it (the record check reads g1017 as 0 for it: mq_class_hold, from mq_coop_var) while
//   the other student has a lesson picked and has not started yet ("<name> is on the way" is shown); as soon as the
//   other one arrives (its console sees this one waiting and starts at once) or starts, the held lesson is queued
//   (the event of that record, as the town's triggers start one). After 90 seconds, or when the other student leaves,
//   has no lesson to take or is gone, it starts alone. The two consoles decide the same way from each other's state
//   (message 42, twice a second and on each change), so no console has to be asked.
// - Each student keeps its own lesson (g1017 is each console's: if they picked different subjects, each has its own
//   teacher and questions in the same room at the same time) and is graded on its own record.
// - The other student sits beside: when a lesson of this console starts with the other student taking theirs, the
//   classmate beside (the one the scene put at (6.5, 9.0)) is left out of this console's room, and the other
//   student's packets from the classroom's own seat (3.5, 9.0) are shown at (6.5, 9.0) (mq_class_remote, from the
//   remote player's update hook). Packets from anywhere else (the board at the front, the hall) are left alone.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cfie_game.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

constexpr uint32_t kField = 0x020AD7EC;          // the field object
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the session)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+0x2254: flag bytes; +8 running;
                                                 // +0x23BC the start kind of a queued event)
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kSummon = 0x020C67F8;         // ov001 (field, n): keep at most n people in the room
constexpr uint32_t kStore = 0x020AF020 + 0x2D8;  // the town's variable store (g1000 + index; bytes below 0x100)
constexpr uint32_t kFieldArea = 0x3EB4, kWanted = 0x3EA4;
constexpr uint32_t kNpcs = 0x3E48, kNpcCount = 0x3EAD, kNpcSize = 0x2B0, kNpcId = 0xA1;
constexpr uint32_t kRemoteQueue = 0xE4;           // a remote player's packets (4 x 16 bytes)
constexpr int kClassroom = 0;
constexpr int32_t kSeatX = 0x3800, kSeatZ = 0x9000;  // the local student's seat in every classroom scene (3.5, 9.0)
constexpr int32_t kBesideX = 0x6800;                 // the desk beside it (6.5, 9.0)
constexpr int kHoldFrames = 90 * 60;                 // the longest a lesson waits for the other student
constexpr int kStale = 3 * 60;                       // the other's state counts this long after its last message
constexpr uint8_t kMsgType = 42;  // (40 the players' talk, 41 the new day's end of a visit)
enum : uint8_t { kPick = 1, kHeld = 2, kScene = 4 };
constexpr uint32_t kWaitEvent = 90014;   // our "<name> is on the way" (game/src/coop_content.cpp)
constexpr uint32_t kBeginEvent = 90015;  // ... and into the classroom again (warp_map 0, as Sol's event 010.99)
constexpr uint32_t kOtherFlag = 141;     // t141: the other student's actor (110 + slot), for $503'641;

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

bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0);
}

int field_area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? int(int8_t(ntr_rd8(c, field + kFieldArea))) : -1;
}

bool lesson_file(int file) { return file == 11 || file == 21 || file == 31 || file == 41 || file == 42; }

struct State {
    // this console
    int held = 0;          // the lesson (file * 100 + block) waiting for the other student, 0 none
    int held_frames = 0;
    bool begin = false;    // the waiting lesson begins: into the classroom again (retried while another event runs)
    bool again = false;    // ... and on arrival its record starts it, held no more
    bool wait_text = false;   // "<name> is on the way" still to show
    bool beside_free = false; // this console's scene left the desk beside free for the other student
    bool lesson = false;      // the classroom's scene is a lesson (not homeroom: its classmates all have parts)
    int idle = 0;             // frames out of a classroom scene
    bool shown_beside = false;  // (logged once a lesson)
    uint8_t sent = 0xFF;
    int frames = 0;
    // the other student (one: the first other console that tells us)
    uint8_t other = 0;
    int other_slot = -1;
    int other_seen = -1000000;
} st;

bool other_fresh() { return st.other_slot >= 0 && st.frames - st.other_seen <= kStale; }

uint8_t own_state(NtrCpu* c) {
    uint8_t f = 0;
    if (ntr_rd8(c, kStore + 0x11) != 0 && ntr_rd8(c, kStore + 0x0F) == 0) f |= kPick;  // g1017 picked, g1015 not done
    if (st.held) f |= kHeld;
    if (!st.held && field_area(c) == kClassroom && event_busy(c)) f |= kScene;
    return f;
}

void on_message(NtrCpu* /*c*/, uint32_t /*node*/, const uint8_t* d, size_t len) {
    if (len < 3 || d[0] != 1) return;
    st.other = d[1];
    st.other_slot = d[2];
    st.other_seen = st.frames;
}

bool queue(NtrCpu* c, int id) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || event_busy(c)) return false;
    if (call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) == 0) return false;
    ntr_wr8(c, ctx + 0x23BC, 2);
    return true;
}

// The NPC entry of the field with that classmate number, or 0.
uint32_t npc_of(NtrCpu* c, uint32_t field, uint8_t id) {
    const uint32_t npcs = ntr_rd32(c, field + kNpcs);
    if (!ram(npcs)) return 0;
    const int n = int8_t(ntr_rd8(c, field + kNpcCount));
    for (int i = 0; i < n; i++) {
        const uint32_t o = npcs + uint32_t(i) * kNpcSize;
        if (ntr_rd8(c, o + kNpcId) == id) return o;
    }
    return 0;
}

bool in_summon = false;

}  // namespace

// From mq_coop_var (main 0x0201D960, instead): the start records' check (ov001 0x020F4214, the record in r4) reads
// g1017 for a lesson's record. True: hold this lesson (the check reads 0, so it does not start now).
bool mq_class_hold(NtrCpu* c) {
    if (!in_session(c)) return false;
    const uint32_t rec = c->r[4];
    if (!ram(rec) || !ram(rec + 0x50)) return false;
    const int file = int16_t(ntr_rd16(c, rec + 2)), block = ntr_rd8(c, rec + 1);
    if (!lesson_file(file) || int16_t(ntr_rd16(c, rec + 8)) != 1017) return false;
    const int value = ntr_rd8(c, kStore + 0x11);
    if (value == 0 || value < int(ntr_rd8(c, rec + 10)) || value > int(ntr_rd8(c, rec + 11))) return false;  // (not this one)
    if (st.held == file * 100 + block) {  // (the records checked again while it waits)
        static int again = 0;
        if (++again <= 3) std::fprintf(stderr, "coop class: lesson %d checked again, still held\n", st.held);
        return true;
    }
    if (st.held || st.begin) return false;
    st.lesson = true;
    if (st.again) {
        st.again = false;
        std::fprintf(stderr, "coop class: lesson %d starts\n", file * 100 + block);
        return false;
    }
    // The other student: about to take a lesson and not in one yet, and not waiting (then this one starts now and
    // the other one's starts with it).
    if (!other_fresh() || !(st.other & kPick) || (st.other & (kHeld | kScene))) {
        std::fprintf(stderr, "coop class: lesson %d starts now (the other student: %s)\n", file * 100 + block,
                     !other_fresh() ? "not here" : (st.other & kHeld) ? "waiting in class" :
                     (st.other & kScene) ? "in class" : "no lesson to take");
        return false;
    }
    st.held = file * 100 + block;
    st.held_frames = 0;
    st.wait_text = true;
    std::fprintf(stderr, "coop class: lesson %d held until slot %d's starts too\n", st.held, st.other_slot);
    return true;
}

// ov001 0x020C67F8 (field, n), instead: the building's placement keeps at most n people in the room (the scene's
// classmates first, in the order the scene registered them at field+0x3EA4; the rest go home). In the classroom in a
// session: n is at least the number the scene registered, so its classmates stay; and while the other student takes
// a lesson here too, the classmate the scene put beside the local student is left out (the other student sits there).
extern "C" int mq_class_summon(NtrCpu* c) {
    if (in_summon || !in_session(c)) return 0;
    const uint32_t field = c->r[0];
    if (!ram(field) || field != ntr_rd32(c, kField) || int8_t(ntr_rd8(c, field + kFieldArea)) != kClassroom) return 0;
    int wanted = 0;
    for (uint32_t i = 0; i < 4; i++) wanted += ntr_rd8(c, field + kWanted + i) != 0xFF;
    if (wanted == 0) return 0;
    int n = int(int32_t(c->r[1]));
    const bool other_sits = st.lesson && other_fresh() && (st.other & (kHeld | kScene)) && st.other_slot >= 0;
    int dropped = -1;
    if (other_sits) {
        for (uint32_t i = 0; i < 4 && dropped < 0; i++) {
            const uint8_t id = ntr_rd8(c, field + kWanted + i);
            const uint32_t o = id == 0xFF ? 0 : npc_of(c, field, id);
            if (!o) continue;
            const int32_t x = int32_t(ntr_rd32(c, o + 0x2C)), z = int32_t(ntr_rd32(c, o + 0x34));
            if (std::abs(x - kBesideX) < 0x400 && std::abs(z - kSeatZ) < 0x400) {
                ntr_wr8(c, field + kWanted + i, 0xFF);
                dropped = id;
                wanted--;
            }
        }
    }
    const int keep = dropped >= 0 ? wanted : (n > wanted ? n : wanted);
    std::fprintf(stderr, "coop class: the classroom keeps %d of its people (asked %d)%s\n", keep, n,
                 dropped >= 0 ? ", the desk beside is the other student's" : "");
    if (dropped >= 0) st.beside_free = true;
    in_summon = true;
    call(c, kSummon, field, uint32_t(keep));
    in_summon = false;
    return 1;
}

// From mq_coop_remote (main 0x02031404, before: a remote player's update, r0 the player): the other student sitting
// at the classroom's own seat is shown at the desk beside, unless a classmate of this console's scene sits there.
void mq_class_remote(NtrCpu* c, uint32_t player) {
    if (!in_session(c) || field_area(c) != kClassroom) return;
    const uint32_t field = ntr_rd32(c, kField), npcs = ntr_rd32(c, field + kNpcs);
    const int n = ram(npcs) ? int8_t(ntr_rd8(c, field + kNpcCount)) : 0;
    for (int i = 0; i < n; i++) {  // (a classmate shown at the desk beside: the other student stays where it is)
        const uint32_t o = npcs + uint32_t(i) * kNpcSize;
        if (ntr_rd32(c, o + 0x20) != 0 && std::abs(int32_t(ntr_rd32(c, o + 0x2C)) - kBesideX) < 0x400 &&
            std::abs(int32_t(ntr_rd32(c, o + 0x34)) - kSeatZ) < 0x400)
            return;
    }
    for (uint32_t i = 0; i < 4; i++) {
        const uint32_t p = player + kRemoteQueue + i * 16;
        const uint8_t type = ntr_rd8(c, p);  // (0x0F and 0x11 both carry the place: the update's case 0)
        if (static const bool log = std::getenv("MQ_CLASS_LOG") != nullptr; log && type != 0x0E)
            std::fprintf(stderr, "coop class: packet %02x area %d (%d, %d)\n", type, ntr_rd8(c, p + 2),
                         int32_t(ntr_rd32(c, p + 4)), int32_t(ntr_rd32(c, p + 8)));
        if ((type != 0x0F && type != 0x11) || ntr_rd8(c, p + 2) != kClassroom) continue;
        if (int32_t(ntr_rd32(c, p + 4)) == kSeatX && int32_t(ntr_rd32(c, p + 8)) == kSeatZ) {
            ntr_wr32(c, p + 4, kBesideX);
            if (!st.shown_beside) std::fprintf(stderr, "coop class: the other student sits at the desk beside\n");
            st.shown_beside = true;
        }
    }
}

// Once a frame (from mq_frame_end).
void mq_class_frame(NtrCpu* c) {
    static bool listening = false;
    if (!listening) {
        coop_sync::listen(kMsgType, on_message);
        listening = true;
    }
    st.frames++;
    if (!in_session(c)) {
        if (st.held) {  // (the session ended while waiting: the lesson begins alone)
            std::fprintf(stderr, "coop class: the session ended: lesson %d begins alone\n", st.held);
            st.held = 0, st.begin = true;
        }
        st.other_slot = -1, st.beside_free = false, st.sent = 0xFF, st.wait_text = false;
        if (st.begin && field_area(c) == kClassroom && queue(c, kBeginEvent)) st.begin = false, st.again = true;
        return;
    }
    const int own = coop_sync::own_slot();
    if (own < 0) return;
    const uint8_t mine = own_state(c);
    if (mine != st.sent || st.frames % 30 == 0) {
        const uint8_t msg[3] = {1, mine, uint8_t(own)};
        ntr::coop_send(kMsgType, msg, sizeof msg);
        st.sent = mine;
    }
    const bool scene = field_area(c) == kClassroom && event_busy(c);
    // (out of class a while: no lesson here any more. A while: a scene starts some frames after its record matched,
    // the room still loading.)
    st.idle = (scene || st.held || st.begin) ? 0 : st.idle + 1;
    if (st.idle > 120) st.beside_free = false, st.lesson = false, st.again = false, st.shown_beside = false;
    if (st.held) {
        st.held_frames++;
        const char* why = nullptr;
        if (field_area(c) != kClassroom) {
            std::fprintf(stderr, "coop class: lesson %d no longer waits (the student left the classroom)\n", st.held);
            st.held = 0, st.wait_text = false;
        } else if (!other_fresh()) why = "the other student is gone";
        else if (st.other & (kHeld | kScene)) why = "the other student is here";
        else if (!(st.other & kPick)) why = "the other student has no lesson to take";
        else if (st.held_frames > kHoldFrames) why = "waited long enough";
        if (why) {
            std::fprintf(stderr, "coop class: lesson %d begins (%s)\n", st.held, why);
            st.held = 0, st.wait_text = false, st.begin = true;
        }
    }
    if (st.wait_text && st.other_slot >= 0 && !event_busy(c)) {
        const uint32_t ctx = ntr_rd32(c, kScriptContext);
        if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + kOtherFlag, uint8_t(110 + st.other_slot));
        if (queue(c, kWaitEvent)) st.wait_text = false;
    }
    // The waiting lesson begins: into the classroom again (our event, a warp as Principal Sol's), where its start
    // record starts it as the game does (the room's people placed for it).
    if (st.begin && queue(c, kBeginEvent)) {
        st.begin = false, st.again = true;
        std::fprintf(stderr, "coop class: into the classroom again for the lesson\n");
    }
}
