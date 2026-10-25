// Magician's Quest co-op: "Connect now" (docs/COOP.md, "Connecting at once").
//
// On the DS a visit starts at the Gateway of the Beginning and End upstairs in the dorm: touching it warps the
// student to the communications room (map 16), whose event (271, block 1) asks query 0x2A. That query (ov028
// 0x021158AC -> ov001 0x020F224C) runs the whole room in one call: ov001 0x020F17EC shows the system messages
// (bank 999: 20 "Connecting...", 22 "What would you like to do?", the menus 40 (visit / invite / friend code /
// exit), 42 (Multi-Card Play / Nintendo WFC), 45 (start? yes / no), then 46 "Saving.", and 47 "Preparing
// connection..." and 48 "Preparation of connection complete." for a host, or 49 "Searching for a town to visit...",
// 50 the towns found, 54/55 "Connect to <town>?" and 47/48 for a guest; 51-53 when nothing was found), acts on each
// menu's answer (the session object's methods, ov023 0x02111114 to start Multi-Card Play), and returns 1 once
// connected. The query then sets the session's flags up (0x020F224C) and the event warps the student to the dorm
// (map 14, by the Gateway), loading the host's town for a guest, with the session's rules for everyone.
//
// Connect now does the same without the walk and the room: our event (90005, game/src/coop_content.cpp) asks query
// 0x2A (in the dorm, see below) and warps to the dorm as event 271 does, and while the room's process runs, its
// questions are answered here, as a player would, with the buttons of the game's pad (main 0x020036D8 updates it
// once a frame; mq_coop_pad adds ours after it has read the DS's): the role's option in menu 40, Multi-Card Play,
// Yes, the first town found (a guest searches again until a host is there), and A for each message. Everything the
// game does on the way (saving, the session's set-up, the guest's arrival) is its own.
//
// The room's process runs only in the dorm. Its code calls into overlay 3 (e.g. 0x021016C0, from ov001 0x020F1E28
// while it prepares the connection), the overlay the dorm (and the room) load into the slot that the town fills with
// overlay 2; run in town, that call landed in the middle of overlay 2's code and the game died ("Saving." then
// "interpreter left guest code at ffff0010, started at 021016c1"). So away from the dorm, Connect now first takes
// the student to the dorm by the Gateway (event 90011, the arrival 271 uses), as a player walks there on the DS, and
// asks the room once they are free to move there, with the game's overlay table (main 0x02009DD8) showing overlay 3.
//
// Started by the setting "Connect automatically" (once after loading, when the student is free to move), the hotkey
// (Connect now), or MQ_COOP_AUTO=host|join (tests). Without any of them nothing here does anything.
#include "coop_connect.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "coop_content.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

// The game's addresses (USA, YNNE).
constexpr uint32_t kField = 0x020AD7EC;          // the town/field object (+0x383C the local player)
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the wireless session, null when alone)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context
constexpr uint32_t kControlled = 0x020FE264;    // the actor the player controls (null outside the field)
constexpr uint32_t kPad = 0x020ACAA4;            // the game's pad (+4 held, +8 pressed this frame, +0xA repeated)
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kEventRunning = 0x0201DF58;   // (2): an event runs or is queued
constexpr uint32_t kFieldMap = 0x3EB4;           // the field's byte: the area (map) it shows
constexpr int kDormMap = 14;                     // the dorm, by the Gateway (where 271 and our events arrive)
// The game's overlay table (main 0x02009DD8 loads an overlay): the slot of each overlay (s8 per overlay id) and the
// overlay loaded in each slot (s8 per slot, -1 none).
constexpr uint32_t kOverlaySlot = 0x0209C988, kSlotOverlay = 0x020A309C;
constexpr int kRoomOverlay = 3;                  // the dorm's overlay, which the room's code calls into
constexpr int kDormWarps = 3;                    // tries to reach the dorm before giving up
// The script context's text box (+0x30): the message it shows is +0x1470 + 100 * +0x146C (main 0x02017C38). Its
// menu (+0x1CE4): the cursor, s16 at +0x294 (main 0x02039420).
constexpr uint32_t kTextBox = 0x30, kMessage = 0x1470, kMessageBlock = 0x146C, kMenu = 0x1CE4, kCursor = 0x294;
// DS buttons (KEYINPUT order).
constexpr uint16_t kA = 1u << 0, kDown = 1u << 7, kUp = 1u << 6;

// The communications room's messages (bank 999) whose menus are answered.
constexpr uint32_t kMenuMain = 40, kMenuKind = 42, kMenuStart = 45, kTownList = 50, kMenuSearchAgain = 53,
                   kMenuConnect = 55;
constexpr int kMainVisit = 0, kMainInvite = 1, kMainExit = 3;
constexpr int kSearches = 30;      // a guest searches this many times for a host (a few minutes) before it gives up
constexpr int kFreeFrames = 90;    // the student is free to move this long before connecting
constexpr int kStartFrames = 300;  // a queued event that has not started by then is queued again
constexpr int kPressEvery = 4;     // frames from one press to the next (one frame down)
constexpr int kGiveUpFrames = 60 * 60 * 10;  // the room's process is left to the player after this long

std::atomic<int> auto_role{0};     // the setting (Role)
std::atomic<int> lan_role{0};      // Co-op between PCs (0 off, 1 host, 2 join)
std::atomic<int> requested{0};     // a Role asked for (the hotkey, the setting once, MQ_COOP_AUTO), 0 none
std::atomic<bool> cancel{false};   // the hotkey again

enum class State { Idle, Waiting, Queued, Running };
struct Run {
    State state = State::Idle;
    coop_connect::Role role = coop_connect::Role::None;
    int free_frames = 0;      // frames in a row the field was up with no event
    int frames = 0;           // since the room's process started
    int searches = 0;         // a guest's searches
    int main_menus = 0;       // times menu 40 came up (a second time: something failed, so Exit)
    int dorm_warps = 0;       // times the student was sent to the dorm first
    uint32_t last_message = 0;
    bool auto_done = false;   // the setting (or MQ_COOP_AUTO) has connected once this run
} run;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0, uint32_t a1 = 0, uint32_t a2 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0, c->r[1] = a1, c->r[2] = a2;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

// The communications room's code can run: the dorm, with its overlay loaded.
bool room_can_run(NtrCpu* c, uint32_t field) {
    const int8_t slot = int8_t(ntr_rd8(c, kOverlaySlot + kRoomOverlay));
    const bool overlay = slot >= 0 && slot < 8 && int8_t(ntr_rd8(c, kSlotOverlay + uint32_t(slot))) == kRoomOverlay;
    return overlay && ntr_rd8(c, field + kFieldMap) == kDormMap;
}

bool in_session(NtrCpu* c) {
    const uint32_t game = ntr_rd32(c, kGame);
    return ntr::multiplayer_active() || (ram(game) && ntr_rd32(c, game + 0xA28) != 0);
}

// The role MQ_COOP_AUTO asks for (host, join), once.
int env_role() {
    static const int role = [] {
        const char* e = std::getenv("MQ_COOP_AUTO");
        if (!e) return 0;
        if (std::strcmp(e, "host") == 0) return int(coop_connect::Role::Host);
        if (std::strcmp(e, "join") == 0 || std::strcmp(e, "guest") == 0) return int(coop_connect::Role::Join);
        return 0;
    }();
    return role;
}

void log(const char* what) { std::fprintf(stderr, "coop connect: %s\n", what); }

}  // namespace

namespace coop_connect {

void configure(int auto_r, int lan_mode) {
    auto_role.store(auto_r == 1 || auto_r == 2 ? auto_r : 0, std::memory_order_relaxed);
    lan_role.store(lan_mode == 1 || lan_mode == 2 ? lan_mode : 0, std::memory_order_relaxed);
}

void request_now() {
    if (requested.load(std::memory_order_relaxed) != 0) {
        cancel.store(true, std::memory_order_relaxed);
        return;
    }
    int role = auto_role.load(std::memory_order_relaxed);
    if (role == 0) role = env_role();
    if (role == 0) role = lan_role.load(std::memory_order_relaxed);
    if (role == 0) role = int(Role::Host);
    requested.store(role, std::memory_order_relaxed);
}

void request(Role role) {
    int none = 0;
    requested.compare_exchange_strong(none, int(role), std::memory_order_relaxed);
}

}  // namespace coop_connect

// Once a frame (from mq_frame_end): starts our event once the student is free to move.
void mq_coop_connect_frame(NtrCpu* c) {
    using coop_connect::Role;
    if (cancel.exchange(false, std::memory_order_relaxed)) {
        if (run.state != State::Idle) log("called off");
        run.state = State::Idle;
        requested.store(0, std::memory_order_relaxed);
    }
    // (test drivers, MQ_COOP_CONNECT_FILE=<path>: "host" or "join" written to it asks for Connect now, as the hotkey)
    if (static const char* path = std::getenv("MQ_COOP_CONNECT_FILE"); path) {
        static unsigned n = 0;
        if (++n % 30 == 0)
            if (FILE* f = std::fopen(path, "rb")) {
                char word[16] = {};
                const int got = std::fscanf(f, "%15s", word);
                std::fclose(f);
                std::remove(path);
                if (got == 1 && requested.load(std::memory_order_relaxed) == 0)
                    requested.store(std::strcmp(word, "join") == 0 ? int(Role::Join) : int(Role::Host),
                                    std::memory_order_relaxed);
            }
    }
    const uint32_t field = ntr_rd32(c, kField), ctx = ntr_rd32(c, kScriptContext);
    const bool field_up = ram(field) && ram(ctx) && ram(ntr_rd32(c, kPad));
    // The setting (or MQ_COOP_AUTO), once per run of the game: as soon as the field is up.
    if (!run.auto_done && field_up && requested.load(std::memory_order_relaxed) == 0) {
        int role = env_role();
        if (role == 0) role = auto_role.load(std::memory_order_relaxed);
        if (role != 0) {
            run.auto_done = true;
            requested.store(role, std::memory_order_relaxed);
        }
    }
    if (run.state == State::Idle) {
        const int role = requested.load(std::memory_order_relaxed);
        if (role == 0) return;
        if (in_session(c)) {  // (already connected: nothing to do)
            requested.store(0, std::memory_order_relaxed);
            return;
        }
        run = Run{State::Waiting, Role(role), 0, 0, 0, 0, 0, 0, run.auto_done};
        log(role == int(Role::Host) ? "as host, once free to move" : "as guest, once free to move");
    }
    if (run.state == State::Waiting) {
        // Free to move: the field is up with the student in it (its controlled actor, ov001's 0x020FE264), and no
        // event runs or is about to start (the context's +8 while one runs, +0x23BC its start kind once one is
        // asked for: a map's start records, e.g. the dorm's first-day intro, start some 30 frames after the student
        // appears). Then a while longer, so a map's own events go first.
        if (!field_up || !ram(ntr_rd32(c, kControlled)) || ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0 ||
            call(c, kEventRunning, 2) != 0) {
            run.free_frames = 0;
            return;
        }
        if (++run.free_frames < kFreeFrames) return;
        if (!room_can_run(c, field)) {  // (away from the dorm: there first)
            if (run.dorm_warps >= kDormWarps) {
                log("the dorm was not reached: called off");
                run.state = State::Idle;
                requested.store(0, std::memory_order_relaxed);
                return;
            }
            if (call(c, kQueueEvent, uint32_t(mq::content::kEventToDorm / 100),
                     uint32_t(mq::content::kEventToDorm % 100), 0xFF) == 0)
                return;
            ntr_wr8(c, ctx + 0x23BC, 2);
            ++run.dorm_warps;
            run.free_frames = 0;
            std::fprintf(stderr, "coop connect: to the dorm first (area %u)\n", ntr_rd8(c, field + kFieldMap));
            return;
        }
        if (call(c, kQueueEvent, uint32_t(mq::content::kEventConnect / 100), uint32_t(mq::content::kEventConnect % 100),
                 0xFF) == 0)
            return;
        ntr_wr8(c, ctx + 0x23BC, 2);
        run.state = State::Queued;
        run.frames = 0;
        log("event queued");
        return;
    }
    if (run.state == State::Queued && ++run.frames > kStartFrames) {  // (the event never started: a map change)
        run.state = State::Waiting;
        run.free_frames = 0;
    }
    if (run.state == State::Running && ++run.frames > kGiveUpFrames) {
        log("the room is left to the player (no answer for too long)");
        run.state = State::Idle;
        requested.store(0, std::memory_order_relaxed);
    }
}

// From mq_coop_gate (every command 0x38 run): our event marks where the room's process starts and ends with a
// test no variable passes (kConnectBegin, kConnectEnd).
void mq_coop_connect_command(NtrCpu* c, uint32_t cmd) {
    if (run.state == State::Idle) return;
    if (ntr_rd32(c, cmd + 4) != mq::content::kQueryResult) return;
    const uint32_t value = ntr_rd32(c, cmd + 8);
    if (value == mq::content::kConnectBegin) {
        run.state = State::Running;
        run.frames = 0;
        log("the communications room's process runs");
    } else if (value == mq::content::kConnectEnd) {
        // The query leaves non-zero in t0 also when the room was left without a visit (Exit), and event 271 then
        // takes the student back to the dorm; here the student stays where they were unless connected.
        const uint32_t ctx = ntr_rd32(c, kScriptContext);
        const bool ok = in_session(c);
        if (!ok && ram(ctx)) ntr_wr8(c, ctx + 0x2254, 0);
        log(ok ? "connected" : "not connected (no host found, or called off)");
        run.state = State::Idle;
        requested.store(0, std::memory_order_relaxed);
    }
}

// main 0x02003628 (r0: the pad), right after the game has read the DS's buttons into it: while the room's process
// runs for Connect now, the buttons a player would press.
void mq_coop_session_pad(NtrCpu* c, uint32_t pad);  // (game/src/coop_session.cpp: the visit's end for a new day)
extern "C" void mq_coop_pad(NtrCpu* c) {
    mq_coop_session_pad(c, c->r[0]);
    if (run.state != State::Running) return;
    const uint32_t pad = c->r[0];
    if (pad != ntr_rd32(c, kPad) || !ram(pad)) return;
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx)) return;
    if (run.frames % kPressEvery != 0) return;
    const uint32_t box = ctx + kTextBox;
    const uint32_t message = ntr_rd32(c, box + kMessage) + 100 * ntr_rd32(c, box + kMessageBlock);
    const int cursor = int16_t(ntr_rd16(c, ctx + kMenu + kCursor));
    if (message != run.last_message) {
        if (message == kMenuMain) ++run.main_menus;
        if (message == kMenuSearchAgain) ++run.searches;
        run.last_message = message;
    }
    if (static const bool log = std::getenv("MQ_COOP_CONNECT_LOG") != nullptr; log) {  // (each message, cursor)
        static uint32_t last = 0;
        if ((message << 8 | uint32_t(cursor & 0xFF)) != last)
            std::fprintf(stderr, "coop connect: message %u cursor %d\n", message, cursor);
        last = message << 8 | uint32_t(cursor & 0xFF);
    }
    int want = -1;  // the option to pick, or -1: press A
    switch (message) {
    case kMenuMain:
        want = run.main_menus >= 2 ? kMainExit : run.role == coop_connect::Role::Host ? kMainInvite : kMainVisit;
        break;
    case kMenuKind:
    case kMenuStart:
    case kTownList:
    case kMenuConnect: want = 0; break;
    case kMenuSearchAgain: {
        static const int searches = std::getenv("MQ_COOP_SEARCHES") ? std::atoi(std::getenv("MQ_COOP_SEARCHES")) : kSearches;
        want = run.searches <= searches ? 0 : 1;  // (MQ_COOP_SEARCHES: fewer, for tests)
        break;
    }
    default: break;
    }
    uint16_t press = kA;
    if (want >= 0 && cursor >= 0 && cursor < 8 && cursor != want) press = cursor < want ? kDown : kUp;
    ntr_wr16(c, pad + 4, uint16_t(ntr_rd16(c, pad + 4) | press));
    ntr_wr16(c, pad + 8, uint16_t(ntr_rd16(c, pad + 8) | press));
    ntr_wr16(c, pad + 0xA, uint16_t(ntr_rd16(c, pad + 0xA) | press));
}
