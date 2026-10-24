// Game-specific glue for the hooks listed in game/config/<release>/hooks.txt (CFIE: USA, CFIP: Europe).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>

#include "cfie_game.hpp"
#include "ntr/lan.hpp"
#include "ntr/nitro.h"
#include "ntr/recomp.h"

namespace cfie {
GameEvents events;
OnlineBuffer online_buffer = OnlineBuffer::Low;
bool fast_saving = true;
AnalogMovement analog_movement = AnalogMovement::SinglePlayer;
uint16_t aspect = 0x1555;
TextSpeed text_speed = TextSpeed::Normal;
int walk_speed = 100;
Stick stick;
bool log_sounds = false;
Game game = Game::Ffcc;
bool log_movies = false;
int force_movie = 0;
}  // namespace cfie

namespace {
// The game's addresses. The European release is the USA build with some code added in the middle of the program: its
// code after about 0x020A3800 sits 0x1C higher and everything from the end of the program (the data, then the
// overlays) 0x20 higher. Addresses in the comments below are the USA release's (CFIE).
struct Addresses {
    uint32_t os_irq_table;  // OS_IRQTable (DTCM)
    uint32_t os_irq_handler_thread_switch;  // OS_IrqHandler_ThreadSwitch (ITCM)
    uint32_t errno_;  // the C library's errno
    uint32_t guest_send_allowance;  // how many more inputs the host lets a guest send, less one
    uint32_t online_timing;  // the online timing table (see cfie_set_online_timing)
    uint32_t network;  // the game's network state (+2: 0 alone, 2 online)
    uint32_t network_is_online;  // (network): the Wi-Fi Connection is up
    uint32_t card_common;  // the card library's common state (cardi_common)
    uint32_t players;  // the players (572 bytes each; +328: their character)
    uint32_t player_present;  // (players, n): player n is in the game
    uint32_t move_yaw;  // (character): the camera's yaw its movement turns by
    uint32_t sin_cos_table;  // FX_SinCosTable_: sin, cos (4.12) per 16 angle units
    uint32_t animation_step;  // (character): 1, or 1.5 and 0.5 under Haste and Slow
    uint32_t run_animation;  // 'RUN ', at a character's +364 while it plays
    uint32_t start_sound;  // (_, id): the sound manager starts sound effect `id`
    uint32_t spell_element;  // the selected spell element (a byte, 0 to 5)
    uint32_t spell_actions;  // the field HUD's actions: (actions, element + 1) selects one
    uint32_t trigger_action;  // (actions, id): runs the handler of an action
    uint32_t blank_input_call;  // return address: a slot nobody's records came in for
};
constexpr Addresses kUsa = {
    0x027E0000,  // os_irq_table
    0x01FF8190,  // os_irq_handler_thread_switch
    0x02103AFC,  // errno_
    0x0212F8A8 + 68,  // guest_send_allowance
    0x0212FD28,  // online_timing
    0x0212F700,  // network
    0x020AD998,  // network_is_online
    0x020FEE40,  // card_common
    0x02122E98,  // players
    0x02099780,  // player_present
    0x02057220,  // move_yaw
    0x020CD780,  // sin_cos_table
    0x0206F1C8,  // animation_step
    0x52554E20,  // run_animation
    0x020B2B10,  // start_sound
    0x02135148,  // spell_element
    0x02136D6C,  // spell_actions
    0x020C68A0,  // trigger_action
    0x020ADF81  // blank_input_call
};
constexpr Addresses kEur = {
    0x027E0000,  // os_irq_table
    0x01FF8190,  // os_irq_handler_thread_switch
    0x02103B1C,  // errno_
    0x0212F8C8 + 68,  // guest_send_allowance
    0x0212FD48,  // online_timing
    0x0212F720,  // network
    0x020AD9B4,  // network_is_online
    0x020FEE60,  // card_common
    0x02122EB8,  // players
    0x02099780,  // player_present
    0x02057220,  // move_yaw
    0x020CD79C,  // sin_cos_table
    0x0206F1C8,  // animation_step
    0x52554E20,  // run_animation
    0x020B2B2C,  // start_sound
    0x02135168,  // spell_element
    0x02136D8C,  // spell_actions
    0x020C68BC,  // trigger_action
    0x020ADF9D  // blank_input_call
};
const Addresses* A = &kUsa;  // the release being played (cfie::select_release)

// Magician's Quest: Mysterious Times (YNNE). Same NitroSDK as FFCC; OS_IrqHandler_ThreadSwitch sits elsewhere in ITCM.
constexpr uint32_t kMqIrqTable = 0x027E0000, kMqIrqThreadSwitch = 0x01FFCDB0;

// Menu sounds: a small ring of requests from the frontend, started by cfie_sound_update.
struct SoundRequest {
    std::atomic<uint16_t> id{0};
    std::atomic<int64_t> at{0};  // steady-clock milliseconds
};
constexpr uint32_t kSoundQueue = 16;
constexpr int64_t kSoundMaxAgeMs = 300;
SoundRequest sound_queue[kSoundQueue];
std::atomic<uint32_t> sound_head{0}, sound_tail{0};

std::atomic<int> spell_request{-1};
std::atomic<int> spell_now{-1};
std::atomic<uint64_t> spell_tick_count{0};

// The cutscene being played, packed for one atomic read: bit 63 playing, bits 32..46 the movie's number, the low 32 bits the
// frame last decoded.
std::atomic<uint64_t> movie_now{0};

int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Analog movement: the stick each player slot's input records carry (see cfie_pack_input), the same on every console.
struct SlotStick {
    bool marked = false;  // the records come from this build
    uint8_t angle = 0;    // 256 steps, 0 = down, 64 = right
    uint8_t push = 0;     // how far it is pushed, 0 (at rest) to kFullPush
};
constexpr uint8_t kFullPush = 63;
SlotStick slot_sticks[4];

// The stick of player slot `player` (a character's +172), or null if the D-pad moves it as on the DS.
const SlotStick* moving_stick(int32_t player) {
    if (player < 0 || player >= 4 || slot_sticks[player].push == 0) return nullptr;
    return &slot_sticks[player];
}

// Whether this console sends its stick: as set, and in a network session only while every player's console reads it
// (a DS or another emulator would move the character by the D-pad bits and fall out of step).
bool send_stick(NtrCpu* c) {
    if (cfie::analog_movement == cfie::AnalogMovement::Off) return false;
    if (ntr_rd8(c, A->network + 2) == 0) return true;
    if (cfie::analog_movement != cfie::AnalogMovement::Multiplayer) return false;
    const uint32_t lr = c->r[14], r0 = c->r[0], r1 = c->r[1], r2 = c->r[2], r3 = c->r[3];
    bool all = true;
    for (uint32_t n = 0; n < 4 && all; n++) {
        c->r[0] = A->players;
        c->r[1] = n;
        ntr_call(c, A->player_present | 1);
        all = c->r[0] == 0 || slot_sticks[n].marked;
    }
    c->r[0] = r0, c->r[1] = r1, c->r[2] = r2, c->r[3] = r3, c->r[14] = lr;
    return all;
}
}  // namespace

// Magician's Quest: the first frame set up for 3D on both screens is the title's (the logos before it are 2D).
extern "C" void mq_on_dual_screen_3d(NtrCpu*) { cfie::events.title_bgm_started = true; }
extern "C" void mq_os_irq_handler(NtrCpu* c) { ntr::nitro::irq_handler(c, kMqIrqTable, kMqIrqThreadSwitch); }
extern "C" void cfie_os_irq_handler(NtrCpu* c) { ntr::nitro::irq_handler(c, A->os_irq_table, A->os_irq_handler_thread_switch); }

namespace cfie {
void queue_sound(uint16_t id) {
    const uint32_t head = sound_head.load(std::memory_order_relaxed);
    if (head - sound_tail.load(std::memory_order_acquire) >= kSoundQueue) return;  // full: this one is dropped
    sound_queue[head % kSoundQueue].id.store(id, std::memory_order_relaxed);
    sound_queue[head % kSoundQueue].at.store(now_ms(), std::memory_order_relaxed);
    sound_head.store(head + 1, std::memory_order_release);
}
}  // namespace cfie

// 0x02076A94 (sound manager): its update, once per pass of the main loop. Starts the queued menu sounds.
extern "C" void cfie_sound_update(NtrCpu* c) {
    uint32_t tail = sound_tail.load(std::memory_order_relaxed);
    if (tail == sound_head.load(std::memory_order_acquire)) return;
    const uint32_t r0 = c->r[0], r1 = c->r[1], r2 = c->r[2], r3 = c->r[3], lr = c->r[14];
    const int64_t now = now_ms();
    while (tail != sound_head.load(std::memory_order_acquire)) {
        const SoundRequest& q = sound_queue[tail % kSoundQueue];
        const uint16_t id = q.id.load(std::memory_order_relaxed);
        const bool fresh = now - q.at.load(std::memory_order_relaxed) <= kSoundMaxAgeMs;
        sound_tail.store(++tail, std::memory_order_release);
        if (!fresh || id == 0) continue;
        c->r[0] = 0;
        c->r[1] = id;
        ntr_call(c, A->start_sound | 1);
    }
    c->r[0] = r0, c->r[1] = r1, c->r[2] = r2, c->r[3] = r3, c->r[14] = lr;
}

namespace cfie {
void request_spell(int element) { spell_request.store(element, std::memory_order_relaxed); }
int spell_element() { return spell_now.load(std::memory_order_relaxed); }
uint64_t spell_ticks() { return spell_tick_count.load(std::memory_order_relaxed); }
}  // namespace cfie

// 0x020C6C3C (the field HUD's pad updater, once per tick: r0 = its state): applies a requested spell element through the
// game's action dispatcher, the call its R + D-pad handler (0x020C6CCC) makes.
extern "C" void cfie_spell_pad(NtrCpu* c) {
    const int current = int(int8_t(ntr_rd8(c, A->spell_element)));
    spell_now.store(current, std::memory_order_relaxed);
    spell_tick_count.fetch_add(1, std::memory_order_relaxed);
    const int want = spell_request.exchange(-1, std::memory_order_relaxed);
    if (want < 0 || want > 5 || want == current) return;
    const uint32_t r0 = c->r[0], r1 = c->r[1], r2 = c->r[2], r3 = c->r[3], lr = c->r[14];
    c->r[0] = A->spell_actions;
    c->r[1] = uint32_t(want + 1);
    ntr_call(c, A->trigger_action | 1);
    c->r[0] = r0, c->r[1] = r1, c->r[2] = r2, c->r[3] = r3, c->r[14] = lr;
}

namespace cfie {
MovieState movie() {
    const uint64_t v = movie_now.load(std::memory_order_relaxed);
    return {(v >> 63) != 0, int((v >> 32) & 0x7FFF), int(uint32_t(v))};
}
}  // namespace cfie

// 0x020ACE48 (the movie object's open: r0 = the object, r1 = the movie's number).
extern "C" void cfie_on_movie_open(NtrCpu* c) {
    if (cfie::force_movie > 0) c->r[1] = uint32_t(cfie::force_movie);
}

// 0x020ACED4 (the movie object's frame count): the movie's update calls it right after it has decoded a frame and counted
// it (+0x50 is the number of frames decoded, +0x58 the movie's number: its file is movie/movie%03d.mods).
extern "C" void cfie_on_movie_frame(NtrCpu* c) {
    const uint32_t decoded = ntr_rd32(c, c->r[0] + 0x50), id = ntr_rd32(c, c->r[0] + 0x58);
    if (decoded == 0) return;
    if (cfie::log_movies) std::fprintf(stderr, "movie %u: frame %u\n", id, decoded - 1);
    movie_now.store((uint64_t(1) << 63) | (uint64_t(id & 0x7FFF) << 32) | (decoded - 1), std::memory_order_relaxed);
}

// 0x020ACEB4 (the movie object's close): the movie is over, or the object is about to open another.
extern "C" void cfie_on_movie_close(NtrCpu*) {
    if (cfie::log_movies && (movie_now.load(std::memory_order_relaxed) >> 63)) std::fprintf(stderr, "movie: closed\n");
    movie_now.store(0, std::memory_order_relaxed);
}

// 0x02076FD0: the sound manager starts a sound effect (r1 = the manager, r2 = its id).
extern "C" void cfie_on_sound_start(NtrCpu* c) {
    if (cfie::log_sounds) std::fprintf(stderr, "sound: start %u\n", c->r[2]);
}

// 0x0207713C: start background music (r1 = track id, 0 = stop).
extern "C" void cfie_on_bgm_play(NtrCpu* c) {
    if (c->r[1] != 0) cfie::events.title_bgm_started = true;
}

// 0x02082A40: pack a tick's input record (out, buttons, touch: x, y, touching): buttons (bit 13: touching), then the
// touch x and y. While the screen isn't touched no console reads x and y (0x02077CB4 keeps the last touched point),
// and a DS never sends y over 191 (TP_GetCalibratedPoint clamps it), so this build sends its stick there: x = its
// angle, y = 192 + how far it is pushed. A DS still sees the D-pad bits; y = 192 alone marks a console that reads them.
extern "C" int cfie_pack_input(NtrCpu* c) {
    const uint32_t out = c->r[0], touch = c->r[2];
    const bool touching = ntr_rd8(c, touch + 2) != 0;
    const uint32_t buttons = ntr_rd16(c, c->r[1]) | (touching ? 0x2000 : 0);
    uint8_t x = ntr_rd8(c, touch), y = ntr_rd8(c, touch + 1);
    if (!touching) {
        x = 0;
        y = 192;
        const cfie::Stick s = cfie::stick;
        if (const float length = std::hypot(s.x, s.y); length > 0 && send_stick(c)) {
            const long push = std::lround(std::min(length, 1.0f) * kFullPush);
            x = uint8_t(std::lround(std::atan2(s.x, s.y) * (128 / 3.14159265358979)) & 0xFF);
            y = uint8_t(192 + push);
        }
    }
    ntr_wr8(c, out, uint8_t(buttons));
    ntr_wr8(c, out + 1, uint8_t(buttons >> 8));
    ntr_wr8(c, out + 2, x);
    ntr_wr8(c, out + 3, y);
    return 1;
}

// 0x020AF120: apply player slot r1's input record (r0) for this tick, on every console for every slot. Keeps the
// stick it carries; while the player touches the screen, the last one.
extern "C" void cfie_apply_input(NtrCpu* c) {
    const uint32_t slot = c->r[1], record = c->r[0];
    if (slot >= 4) return;
    SlotStick& s = slot_sticks[slot];
    if (c->r[14] == A->blank_input_call) {
        s = {};
        return;
    }
    if (ntr_rd8(c, record + 1) & 0x20) return;
    const uint8_t y = ntr_rd8(c, record + 3);
    s.marked = y >= 192;
    s.angle = s.marked ? ntr_rd8(c, record + 2) : 0;
    s.push = s.marked ? uint8_t(std::min(y - 192, int(kFullPush))) : 0;
}

// 0x02056498: a character's movement (velocity out, buttons, speed, player slot): the D-pad's direction (one of
// eight, length 1), turned by the camera's yaw, times the speed. Before it, the game speeds the character up over 8
// ticks while the D-pad is held. With a stick in the player's records the character goes the stick's way instead, at
// the speed times how far it is pushed. Fixed point only, so every console moves it the same.
extern "C" int cfie_analog_move(NtrCpu* c) {
    const int32_t player = int8_t(c->r[3]);
    const SlotStick* s = moving_stick(player);
    if (!s || !(c->r[1] & 0xF0)) return 0;
    const uint32_t out = c->r[0];
    const int64_t speed = int64_t(int32_t(c->r[2])) * s->push / kFullPush;
    // The stick's direction (x = sin, z = cos of its angle), z towards the bottom of the screen (the D-pad's Down).
    const uint32_t a = uint32_t(s->angle) << 4;
    const int64_t x = int16_t(ntr_rd16(c, A->sin_cos_table + a * 4)), z = int16_t(ntr_rd16(c, A->sin_cos_table + a * 4 + 2));

    const uint32_t lr = c->r[14];
    c->r[0] = ntr_rd32(c, A->players + 572 * uint32_t(player) + 328);
    ntr_call(c, A->move_yaw | 1);
    c->r[14] = lr;
    const uint32_t i = (c->r[0] & 0xFFFF) >> 4;
    const int64_t sin = int16_t(ntr_rd16(c, A->sin_cos_table + i * 4)), cos = int16_t(ntr_rd16(c, A->sin_cos_table + i * 4 + 2));
    // As MTX_RotY33 and MTX_MultVec33, then FX_Mul by the speed.
    const int64_t vx = (x * cos + z * sin) >> 12, vz = (z * cos - x * sin) >> 12;
    ntr_wr32(c, out, uint32_t((vx * speed + 0x800) >> 12));
    ntr_wr32(c, out + 4, 0);
    ntr_wr32(c, out + 8, uint32_t((vz * speed + 0x800) >> 12));
    c->r[0] = 1;
    return 1;
}

// 0x0206F1BC: how far a player character's animation (r0, +8 +20: the character) goes in a tick (4.12). The run has
// one speed on the DS, so with the stick pushed part way its feet would slide: it plays slower, as much as the
// character moves slower.
// The perspective setup (G3 perspective: r0 = sin and r1 = cos of the half field of view, r2 = the aspect ratio):
// widescreen is the aspect ratio. Its only callers pass the DS's 4:3.
// The geometry engine (third_party/CMakeLists.txt) widens a viewport only while this is counting down (per VBlank), so
// scenes that do not use this projection (the title) are not stretched.
// A scene of orthographic geometry only (the title) stays 4:3 (ffcc_wide_block counts frames).
extern "C" int ffcc_wide_scene;
extern "C" int ffcc_wide_block;
// Magician's Quest: the game's own perspective setup (fovy, aspect, near, far), called with 4:3 (0x1555).
// Magician's Quest's text box (0x020127F8: box r0, characters to show r1, draw every line r2): draws the box's lines
// with their first r1 characters, and marks it complete (+0xD18) when r1 is exactly the number of characters in all its lines. The
// game's dialogue calls it every frame with one more character. Faster text shows more of them per call, up to that
// total so the box still completes: the lines are +0xAEC of them, each a UTF-8 string at +0x96C + 12 per line, counted
// in characters as the game does (0x0200A480).
// Walk speed (Magician's Quest): an actor's step is its direction scaled by its speed (s16 at actor + 0x88) in
// 0x0201F3BC, called from the actor movement at 0x0201F33C with the actor in r5. For the one the player controls (the
// pointer at 0x020FE264), the speed is raised by the walk speed setting.
extern "C" void mq_walk_speed(NtrCpu* c) {
    if (cfie::walk_speed <= 100 || (c->r[14] & ~1u) != 0x0201F340u) return;
    const uint32_t player = ntr_rd32(c, 0x020FE264u);
    if (player == 0 || c->r[5] != player) return;
    c->r[2] = uint32_t(int32_t(c->r[2]) * cfie::walk_speed / 100);
}

namespace cfie {
bool coop_anytime = true;
}

// In a Multi-Card Play session (or, for research, MQ_COOP_SOLO: as if in one).
static bool coop_session() {
    static const bool solo = std::getenv("MQ_COOP_SOLO") != nullptr;
    return solo || ntr::multiplayer_active();
}

// 0x02112E20 (overlay 28, the dorm's event commands): command 0x38 (id +0, size +1), "if variable <+4> <op +2>
// <value +8>, go to <+12>". Variable 509 is set from the start of a new game until the first lesson; while it is,
// the game holds the new student back with tests of the form "if variable 509 != 1, go on" (op 1, value 1), each
// followed by a message instead: the Gateway's "You cannot use it at this time" (5360), the purple door off the hall's
// "The door is locked" (5306). (The way out of the dorm is the hall's bottom edge; on the first day the warden waits
// there and stops a new student who has not seen enough of the dorm, to walk them to school.)
// Co-op from the first day: the Gateway's test always passes, so the communications room opens; and while the
// console is in a multiplayer session (Multi-Card Play), every such test passes (and mq_coop_var below reads the
// variable as 0), so both students can go out together even though both are new. (Every console in the session runs the same scripts on the same inputs: the
// rule is the same on all of them.) The script is a copy in RAM, made again for each event, so the change is
// passing: the test's value becomes one the variable never has.
extern "C" void mq_coop_gate(NtrCpu* c) {
    // (research, MQ_SCRIPT_TRACE: every command run, as bytes)
    if (static const bool trace = std::getenv("MQ_SCRIPT_TRACE") != nullptr; trace && c->r[0] >= 0x02000000 &&
        c->r[0] < 0x023FFFF0) {
        const uint32_t at = c->r[0];
        const unsigned n = std::min(ntr_rd8(c, at + 1) ? ntr_rd8(c, at + 1) : 4u, 32u);
        std::fprintf(stderr, "script %08x:", at);
        for (unsigned i = 0; i < n; ++i) std::fprintf(stderr, " %02x", ntr_rd8(c, at + i));
        std::fprintf(stderr, "\n");
        if (const char* dump = std::getenv("MQ_SCRIPT_DUMP"); dump && ntr_rd8(c, at) == 0x38 && ntr_rd32(c, at + 4) == 509 &&
            ntr_rd16(c, at + ntr_rd8(c, at + 1) + 2) == 5306) {
            if (FILE* f = std::fopen(dump, "wb")) {
                std::fprintf(f, "%08x\n", at - 0x800);
                for (uint32_t a = at - 0x800; a < at + 0x1000; ++a) std::fputc(int(ntr_rd8(c, a)), f);
                std::fclose(f);
            }
        }
    }
    if (!cfie::coop_anytime) return;
    const uint32_t cmd = c->r[0];
    if (cmd < 0x02000000 || cmd >= 0x023FFFF0) return;
    if (ntr_rd8(c, cmd) != 0x38 || ntr_rd32(c, cmd + 4) != 509 || ntr_rd8(c, cmd + 2) != 1 || ntr_rd32(c, cmd + 8) != 1)
        return;
    const uint32_t next = cmd + ntr_rd8(c, cmd + 1);  // (a command's size is its second byte)
    const bool gateway = ntr_rd8(c, next) == 0x67 && ntr_rd16(c, next + 2) == 5360;
    if (gateway || coop_session()) ntr_wr32(c, cmd + 8, 0x7FFF);
}

// 0x0201D960: an event script variable's value (r0: the script context, r1: the variable). While the console is in a
// multiplayer session, variable 509 (the new student's first day, until the first lesson) reads as 0, as after the
// first lesson: the warden is back at his counter instead of waiting at the way out, and both players can walk out
// into town together. The same on every console. Nothing is written: when the session ends, the first day is back.
extern "C" int mq_coop_var(NtrCpu* c) {
    if (static const bool trace = std::getenv("MQ_SCRIPT_TRACE") != nullptr; trace)
        std::fprintf(stderr, "script variable %u (lr %08x)\n", c->r[1], c->r[14]);
    if (!cfie::coop_anytime || !coop_session()) return 0;
    // The first day's steps are ordered by the script context's byte 233 (variable 10233): 1 after the dorm's intro,
    // 2 once the warden has walked the new student to the school, 3 and on during the principal's welcome (event 001,
    // blocks 5-7; each needs variable 509 = 1). A session has no escort, so for a student still at step 1 the step
    // reads as 2: walking into the school yourselves starts the welcome (the wand and hat, then the first day ends),
    // and the warden doesn't wait to walk you there. The welcome's own scripts move the step on for real.
    if (c->r[1] == 10233) {
        const uint32_t ctx = c->r[0];
        if (ctx < 0x02000000 || ctx >= 0x023FD000 || ntr_rd8(c, ctx + 0x2254 + 233) != 1) return 0;
        if (static const bool keep = std::getenv("MQ_COOP_KEEP_DAY1") != nullptr; keep) return 0;  // (research)
        c->r[0] = 2;
        return 1;
    }
    if (c->r[1] != 509) return 0;
    // Only where the town places its people (overlay 1, 0x020F4214: an NPC's conditions): the warden is then at his
    // counter, not holding the way out. The lessons' own scripts read the true value, so the first lesson still ends
    // the first day (in the student's own record).
    if (c->r[14] < 0x020F4214 || c->r[14] >= 0x020F42C8) return 0;
    // The same check (the record in r4: the event it starts at +1 block / +2 file, conditions from +8) also decides
    // the story's start records (start/mc_ss.bin): the principal's welcome (event 001, blocks 6 and 7) still sees the
    // student's first day, so walking into the school starts it. (Not the rest of file 001: the dorm's first-day
    // scenes would start in the middle of connecting.)
    if (const uint32_t rec = c->r[4]; rec >= 0x02000000 && rec < 0x023FFFB0 && int16_t(ntr_rd16(c, rec + 2)) == 1) {
        const uint32_t block = ntr_rd8(c, rec + 1);
        if (block == 6 || block == 7) return 0;
    }
    if (static const bool keep = std::getenv("MQ_COOP_KEEP_DAY1") != nullptr; keep) return 0;  // (research)
    c->r[0] = 0;
    return 1;
}

// 0x020F4900 (overlay 1, the town): may this NPC be placed (r0: its placement, whose record +0x4C is its kind)? During a
// visit the game leaves out kind 4 — the school's staff, the principal among them — so a visit has no lessons. In a
// co-op session they are placed as on any day: the principal waits at the classroom and classes run for both. (The
// same rule on every console.)
extern "C" int mq_coop_npc(NtrCpu* c) {
    if (!cfie::coop_anytime || !coop_session()) return 0;
    c->r[0] = 1;
    return 1;
}

// 0x0201D9F8: set an event script variable (r0: the script context, r1: the variable, r2: its value, r3: the player
// whose record it is). (research, MQ_SCRIPT_TRACE: each write, with the record it lands in)
extern "C" void mq_coop_set(NtrCpu* c) {
    static const bool trace = std::getenv("MQ_SCRIPT_TRACE") != nullptr;
    if (!trace) return;
    const uint32_t var = c->r[1], value = c->r[2], player = c->r[3];
    const uint32_t session = ntr_rd32(c, ntr_rd32(c, 0x020ACA58) + 0xA28);
    uint32_t slot = 0;
    if (session && player < 4) slot = ntr_rd32(c, session + player * 0x50 + 0x4A00);
    std::fprintf(stderr, "script set %u = %d (player %u, session %08x, record %08x, lr %08x)\n", var, int32_t(value),
                 player, session, slot, c->r[14]);
}

// 0x0201E070: a variable store's write (r0: the store, r1: the index in it, r2: the value). (research, MQ_VAR_WATCH=<n>,...:
// log writes of those indexes, with the caller)
extern "C" void mq_var_store(NtrCpu* c) {
    static const char* watch = std::getenv("MQ_VAR_WATCH");
    if (!watch) return;
    char key[16];
    std::snprintf(key, sizeof key, ",%u,", c->r[1]);
    static const std::string list = std::string(",") + watch + ",";
    if (list != ",*," && list.find(key) == std::string::npos) return;
    std::fprintf(stderr, "var store %08x[%u] = %d (lr %08x)\n", c->r[0], c->r[1], int32_t(c->r[2]), c->r[14]);
}

// ov001 0x020C6F1C (field, dual): the field's screen setup. Its callers pass dual = 0 whenever the field was loaded
// in a session (field+0x3EAF, the session flag it caches): on the DS a visit draws 3D on the bottom screen only, and
// the top screen shows a pattern. On PC there is no need to save that time: in a co-op session the town (ov002) and
// the areas that have a top-screen view (ov003, area flag 4) keep 3D on both screens, as in single player. Only the
// argument changes; the session flag and everything else stay as they are, and it is the same on every console.
extern "C" void mq_dual_in_session(NtrCpu* c) {
    if (!cfie::coop_anytime || c->r[1] != 0 || ntr_rd8(c, c->r[0] + 0x3EAF) == 0) return;
    static const bool off = std::getenv("MQ_COOP_SINGLE_3D") != nullptr;  // (research: the game's own choice)
    if (off) return;
    const uint32_t lr = c->r[14] & ~1u;
    if (lr == 0x020FFD72) {
        c->r[1] = 1;  // the town
    } else if (lr == 0x0210004A) {
        const uint32_t area = ntr_rd32(c, c->r[0] + 0x42368);
        if (area >= 0x02000000 && area < 0x02400000 && (ntr_rd8(c, area + 21) & 4)) c->r[1] = 1;
    }
}

// ov001 0x020E2ABC (npc manager, online): loads the area's NPC triggers. Its caller (0x020E14C4) passes online = 1
// for a field loaded in a session (field+0x3EAF), which loads /npc/NpcTrgOL.bin, a small set without the story's
// triggers (the warden's escort, the principal's welcome...). In a co-op session the single-player set
// (/npc/NpcTrg.bin) is loaded instead, so the story's people do what they do in single player. The same on every
// console. (MQ_SCRIPT_TRACE logs each load.)
extern "C" void mq_coop_triggers(NtrCpu* c) {
    static const bool trace = std::getenv("MQ_SCRIPT_TRACE") != nullptr;
    if (trace)
        std::fprintf(stderr, "npc triggers: online %u, mode byte %u, session %d\n", c->r[1], ntr_rd8(c, 0x020AD7FF),
                     int(coop_session()));
    if (cfie::coop_anytime && coop_session() && !std::getenv("MQ_COOP_ONLINE_TRIGGERS")) c->r[1] = 0;
}

extern "C" void mq_text_box_draw(NtrCpu* c) {
    // (research and test drivers, MQ_TEXT_LOG: each new text box's first line, as "text: <line>")
    if (static const bool log = std::getenv("MQ_TEXT_LOG") != nullptr; log) {
        const uint32_t p = ntr_rd32(c, c->r[0] + 0x96C);
        char line[96] = {};
        for (int i = 0; i < 95 && p >= 0x02000000 && p < 0x02400000; i++)
            if (!(line[i] = char(ntr_rd8(c, p + uint32_t(i))))) break;
        static std::string last;
        if (last != line) {
            std::fprintf(stderr, "text: %s\n", line);
            std::fflush(stderr);
            last = line;
        }
    }
    // (research, MQ_TEXT_TRACE=<text>: who opened a text box whose first line starts with it)
    if (static const char* trace = std::getenv("MQ_TEXT_TRACE"); trace) {
        const uint32_t p = ntr_rd32(c, c->r[0] + 0x96C);
        char line[64] = {};
        for (int i = 0; i < 63 && p >= 0x02000000 && p < 0x02400000; i++)
            if (!(line[i] = char(ntr_rd8(c, p + uint32_t(i))))) break;
        static std::string last;
        if (std::strncmp(line, trace, std::strlen(trace)) == 0 && last != line) {
            std::fprintf(stderr, "text-trace: \"%s\" lr %08x stack", line, c->r[14]);
            for (uint32_t i = 0; i < 256; i++) {
                const uint32_t w = ntr_rd32(c, c->r[13] + 4 * i);
                if ((w >= 0x02000000 && w < 0x020AA000) || (w >= 0x020C1980 && w < 0x02130000)) std::fprintf(stderr, " %08x", w);
            }
            std::fprintf(stderr, "\n");
        }
        last = line;
    }
    if (cfie::text_speed == cfie::TextSpeed::Normal || int32_t(c->r[1]) <= 0) return;
    const uint32_t box = c->r[0];
    const uint32_t lines = ntr_rd32(c, box + 0xAEC);
    if (lines == 0 || lines > 16) return;
    uint32_t total = 0;
    for (uint32_t i = 0; i < lines; i++) {
        uint32_t p = ntr_rd32(c, box + 0x96C + 12 * i);
        if (p < 0x02000000 || p >= 0x02400000) return;  // (not a string: leave the call as it was)
        for (uint32_t n = 0; n < 1024; n++, p++) {
            const uint32_t b = ntr_rd8(c, p);
            if (b == 0) break;
            if ((b & 0xC0) != 0x80) total++;  // (a UTF-8 sequence's first byte)
        }
    }
    const uint32_t want = std::max(c->r[1], std::min(cfie::text_speed == cfie::TextSpeed::Instant ? total : c->r[1] * 3, total));
    if (want == c->r[1]) return;
    // The box draws only the line being written unless r2 asks for every line: lines this call completes at once
    // would otherwise never be drawn.
    c->r[1] = want;
    c->r[2] = 1;
}

// The game's last perspective setup (fovy, near, far as it passed them; 0 fovy: none yet), and the aspect it was given.
static uint32_t mq_persp_args[3] = {0, 0, 0};
static uint16_t mq_persp_applied = 0x1555;
static int mq_zoom_applied = 100;
namespace cfie {
int view_zoom = 100;
}

// The game's field of view (degrees, 20.12: the angle whose tangent spans half the view's height) narrowed so the
// picture is magnified cfie::view_zoom hundredths. The game's own projection, culling and picking (the stylus on the
// 3D world) all follow it.
static uint32_t mq_zoomed_fov(uint32_t fov) {
    if (cfie::view_zoom <= 100) return fov;
    const double pi = 3.14159265358979323846;
    const double deg = int32_t(fov) / 4096.0;
    const double zoomed = std::atan(std::tan(deg * pi / 180) * 100.0 / cfie::view_zoom) * 180 / pi;
    return uint32_t(std::lround(zoomed * 4096));
}

extern "C" void mq_perspective_aspect(NtrCpu* c) {
    if (std::getenv("MQ_LOG_PERSPECTIVE"))
        std::fprintf(stderr, "perspective: fovy %08x aspect %08x near %08x far %08x lr %08x (wide aspect %04x)\n", c->r[0], c->r[1],
                     c->r[2], c->r[3], c->r[14], cfie::aspect);
    // The game sets its projection up once (at boot, and for a few scenes of its own) and keeps it; the widened
    // scene lasts until the next setup (the geometry engine still holds orthographic-only scenes at 4:3 through
    // ffcc_wide_block). Both screens show this 3D (one per frame, the other through display capture): the frontend
    // shows both wide. mq_frame_end sets it up again when the width changes while playing.
    if (c->r[1] != 0x1555) return;
    mq_persp_args[0] = c->r[0], mq_persp_args[1] = c->r[2], mq_persp_args[2] = c->r[3];
    mq_persp_applied = cfie::aspect;
    mq_zoom_applied = cfie::view_zoom;
    c->r[0] = mq_zoomed_fov(c->r[0]);
    if (cfie::aspect != 0x1555) {
        c->r[1] = cfie::aspect;
        ffcc_wide_scene = 1 << 30;
    }
}

// The end of the game's frame, before its 3D buffer swap (0x020092A8): when the aspect ratio changed (a new window
// shape, a new setting), the game's perspective setup (0x020093D0: fovy, aspect, near, far) is run again with the
// arguments it last used, so the projection and the game's own culling follow; the next scene is drawn with it.
extern "C" void ntr_world_extend(NtrCpu* c);  // (runtime/src/world_ext.cpp)

// PC view (docs/PC_VIEW.md, step 1): the field's two screens drawn from one camera, so the top screen continues the
// bottom one's picture upward (the sky over the town) instead of showing it from a second camera pitched ~37 degrees
// higher. The game draws its 3D one screen per frame (parity bit 0 of 0x020ACA55: 0 the top screen, 1 the bottom);
// each pass builds its camera from that screen's view object (0x020AD7B8[parity], look-at 0x0200B7BC). With the PC
// view on, the top pass uses the bottom screen's view object and a projection window shifted up by one screen
// (the game's own off-axis offset, short 0x020AD7D8 in pixels), so the two halves are one taller frustum's slices.
namespace cfie {
bool pc_view = [] { const char* e = std::getenv("MQ_PC_VIEW"); return e && *e && *e != '0'; }();
}
static int mq_pc_view_lift() {
    static const int lift = std::getenv("MQ_PC_VIEW_LIFT") ? std::atoi(std::getenv("MQ_PC_VIEW_LIFT")) : 192;
    return lift;
}
static bool mq_dual_3d(NtrCpu* c) { return ntr_rd8(c, 0x020ACA54) != 0; }

// main 0x0200B7BC (a view object's look-at into the camera matrix), before: the top screen's view -> the bottom's.
extern "C" void mq_view_camera(NtrCpu* c) {
    if (!cfie::pc_view || !mq_dual_3d(c)) return;
    const uint32_t top = ntr_rd32(c, 0x020AD7B8), bottom = ntr_rd32(c, 0x020AD7BC);
    if (c->r[0] == top && bottom >= 0x02000000 && bottom < 0x02400000) c->r[0] = bottom;
}

// main 0x020094C8 (the view object of screen r0), before: asked for the top screen's, the bottom's (the town's own
// reads of the camera's pitch then match the camera actually used).
extern "C" void mq_view_of(NtrCpu* c) {
    if (static const bool off = std::getenv("MQ_PC_VIEW_KEEP_PITCH") != nullptr; off) return;  // (research)
    if (cfie::pc_view && c->r[0] == 0 && mq_dual_3d(c) && ntr_rd32(c, 0x020AD7BC) != 0) c->r[0] = 1;
}

// main 0x02009204 (the frame's 3D begins), before: the projection's window for this pass (lifted for the top screen).
static int mq_lift_applied = 0;
extern "C" void mq_view_pass(NtrCpu* c) {
    const int want = cfie::pc_view && mq_dual_3d(c) && (ntr_rd8(c, 0x020ACA55) & 1) == 0 ? mq_pc_view_lift() : 0;
    if (want == mq_lift_applied || mq_persp_args[0] == 0) return;
    const int16_t game = int16_t(ntr_rd16(c, 0x020AD7D8)) - int16_t(mq_lift_applied);
    ntr_wr16(c, 0x020AD7D8, uint16_t(int16_t(game + want)));
    mq_lift_applied = want;
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = mq_persp_args[0];
    c->r[1] = 0x1555;
    c->r[2] = mq_persp_args[1];
    c->r[3] = mq_persp_args[2];
    ntr_call(c, 0x020093D0 | 1);
    std::memcpy(c->r, saved, sizeof saved);
}

extern "C" void mq_frame_end(NtrCpu* c) {
    // (research and test drivers, MQ_POS_FILE=<path>: every 6 frames, the player's position (0x020B8B7C: x, y, z,
    // 20.12 fixed point; x east, z south) as "x y z" in units)
    if (static const char* pos = std::getenv("MQ_POS_FILE"); pos) {
        static unsigned frames = 0;
        if (++frames % 6 == 0) {
            const std::string tmp = std::string(pos) + ".tmp";
            if (FILE* f = std::fopen(tmp.c_str(), "w")) {
                std::fprintf(f, "%.2f %.2f %.2f\n", int32_t(ntr_rd32(c, 0x020B8B7C)) / 4096.0,
                             int32_t(ntr_rd32(c, 0x020B8B80)) / 4096.0, int32_t(ntr_rd32(c, 0x020B8B84)) / 4096.0);
                std::fclose(f);
                std::rename(tmp.c_str(), pos);
            }
            static const bool trail = std::getenv("MQ_POS_TRAIL") != nullptr;  // (and appended to <path>.trail)
            if (trail && frames % 30 == 0)
                if (FILE* f = std::fopen((std::string(pos) + ".trail").c_str(), "a")) {
                    std::fprintf(f, "%u %.2f %.2f %.2f\n", frames, int32_t(ntr_rd32(c, 0x020B8B7C)) / 4096.0,
                                 int32_t(ntr_rd32(c, 0x020B8B80)) / 4096.0, int32_t(ntr_rd32(c, 0x020B8B84)) / 4096.0);
                    std::fclose(f);
                }
        }
    }
    // (research, MQ_RAM_DUMP=<path>: when <path>.req exists, main RAM (4 MB) is written to <path>.<n> and the request
    // removed)
    if (static const char* dump = std::getenv("MQ_RAM_DUMP"); dump) {
        static int n = 0;
        const std::string req = std::string(dump) + ".req";
        if (FILE* r = std::fopen(req.c_str(), "rb")) {
            std::fclose(r);
            const std::string out = std::string(dump) + "." + std::to_string(n++);
            if (FILE* f = std::fopen(out.c_str(), "wb")) {
                for (uint32_t a = 0x02000000; a < 0x02400000; a += 4) {
                    const uint32_t w = ntr_rd32(c, a);
                    std::fwrite(&w, 4, 1, f);
                }
                std::fclose(f);
            }
            std::remove(req.c_str());
        }
    }
    // (research, MQ_POKE=<hex address>=<value>[/<bytes>],...: written once, a few seconds into the game)
    if (static const char* poke = std::getenv("MQ_POKE"); poke) {
        static int frames = 0;
        static const int at = std::getenv("MQ_POKE_AT") ? std::atoi(std::getenv("MQ_POKE_AT")) : 2000;
        if (++frames == at) {
            for (const char* p = poke; p && *p;) {
                unsigned addr = 0, value = 0, bytes = 1;
                int n = 0;
                if (std::sscanf(p, "%x=%u/%u%n", &addr, &value, &bytes, &n) < 2 && std::sscanf(p, "%x=%u%n", &addr, &value, &n) < 2) break;
                if (bytes == 4) ntr_wr32(c, addr, value);
                else if (bytes == 2) ntr_wr16(c, addr, uint16_t(value));
                else ntr_wr8(c, addr, uint8_t(value));
                std::fprintf(stderr, "poke: %08x = %u (%u bytes)\n", addr, value, bytes);
                p = std::strchr(p, ',');
                if (p) p++;
            }
        }
    }
    ntr_world_extend(c);  // (the ground on into the wide sides, before this frame's 3D is swapped in)
    if (mq_persp_args[0] == 0 || (mq_persp_applied == cfie::aspect && mq_zoom_applied == cfie::view_zoom)) return;
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = mq_persp_args[0];
    c->r[1] = 0x1555;
    c->r[2] = mq_persp_args[1];
    c->r[3] = mq_persp_args[2];
    ntr_call(c, 0x020093D0 | 1);
    std::memcpy(c->r, saved, sizeof saved);
    mq_persp_applied = cfie::aspect;
    mq_zoom_applied = cfie::view_zoom;
}

extern "C" void cfie_perspective_aspect(NtrCpu* c) {
    if (c->r[2] == 0x1555 && cfie::aspect != 0x1555 && ffcc_wide_block == 0) {
        c->r[2] = cfie::aspect;
        ffcc_wide_scene = 8;
    }
}

extern "C" int cfie_player_animation_step(NtrCpu* c) {
    const uint32_t character = ntr_rd32(c, ntr_rd32(c, c->r[0] + 8) + 20);
    if (ntr_rd32(c, character + 364) != A->run_animation) return 0;
    const int32_t player = int8_t(ntr_rd8(c, character + 172));
    const SlotStick* s = moving_stick(player);
    if (!s || s->push >= kFullPush || character != ntr_rd32(c, A->players + 572 * uint32_t(player) + 328)) return 0;
    const uint32_t lr = c->r[14];
    c->r[0] = character;
    ntr_call(c, A->animation_step | 1);
    c->r[14] = lr;
    c->r[0] = uint32_t(int64_t(int32_t(c->r[0])) * s->push / kFullPush);
    return 1;
}

// 0x02025260: CARDi_RequestStreamCommand. The game's save thread reads and writes the save 1 KB at a time, and the
// card library hands each 256-byte page to the ARM7: a save of 25 KB took 3.2 s. Fast saving does it at once.
extern "C" int cfie_card_backup_request(NtrCpu* c) {
    return cfie::fast_saving && ntr::nitro::card_backup_request(c, A->card_common);
}

// 0x02005BBC: the Wi-Fi Connection test (DWC netcheck). It converts the HTTP status with atoi and fails if errno is
// ERANGE, but errno is never cleared: the stats server's session key (\sesskey\, read with atoi at 0x02040D02)
// overflows on today's revival servers, and every connection after the first then fails with error 52200.
extern "C" void cfie_on_netcheck(NtrCpu* c) { ntr_wr32(c, A->errno_, 0); }

// 0x020ADDBC: how many of the guest's queued inputs (one per game frame, 30 a second) to send to the host now: at most
// 3, and no more than the host allows. Online (Wi-Fi Connection) the game sends nothing until it has 3, so an input
// waits up to 100 ms before it leaves, and the guest sees the result only with the host's reply to it. Over local
// wireless it sends whatever it has every frame; this does the same online (unless the game's own timing is chosen).
extern "C" void cfie_guest_inputs_to_send(NtrCpu* c) {
    int32_t n = std::min(int32_t(ntr_rd32(c, c->r[0] + 108)), 3);
    n = std::min(n, int32_t(ntr_rd32(c, A->guest_send_allowance)) + 1);
    if (cfie::online_buffer == cfie::OnlineBuffer::Original && ntr_rd8(c, A->network + 2) == 2 && n != 3) {
        const uint32_t lr = c->r[14];
        c->r[0] = A->network;
        ntr_call(c, A->network_is_online | 1);
        c->r[14] = lr;
        if (c->r[0] != 0) n = 0;
    }
    c->r[0] = uint32_t(n);
}

// 0x020AE314: fills the timing table for online play (r1: one of two presets, 0 or 1). Over local wireless the host
// holds each guest's inputs 1-2 game frames and runs one frame at a time. Online it holds them (ping / 30 ms) + 5
// frames, at least 5 (3 in preset 1), trimmed only 2 past that, and runs and replies 3 frames at a time, so a guest sees
// its own moves some 200 ms late. Low: ping + 1 frame, trimmed 1 past it, one frame at a time. The buffer still grows
// with the ping, as on a slow connection it must; between two instances on a PC it holds 0-1 inputs. Medium: twice the
// margins, for connections whose packets come late. Original: the game's own table.
extern "C" void cfie_set_online_timing(NtrCpu* c) {
    const uint32_t preset = c->r[1];
    const bool second = preset == 1;
    const int32_t game = second ? 3 : 5;
    const int32_t margin = cfie::online_buffer == cfie::OnlineBuffer::Low ? 1 : 2;
    const bool original = cfie::online_buffer == cfie::OnlineBuffer::Original;
    const int32_t fields[] = {
        original ? 2 : margin,     // +84: the host trims a guest's queued inputs once they are this many over the target
        original ? -2 : -margin,   // +88: down to this many under it
        second ? 2 : 0,            // +92, +96: when to tell guests to wait (unchanged)
        second ? 0 : -2,           //
        original ? 3 : 1,          // +100: frames run (and replied) at a time
        original ? game : margin,  // +104: the smallest target
        20,                        // +108: the largest target
        original ? game : margin,  // +112: the target over the ping
    };
    for (int i = 0; i < 8; i++) ntr_wr32(c, A->online_timing + 84 + 4 * i, uint32_t(fields[i]));
    ntr_wr32(c, A->online_timing + 116, preset);
}

bool cfie::select_release(const std::string& game_code) {
    if (game_code == "CFIE") A = &kUsa;
    else if (game_code == "CFIP") A = &kEur;
    else if (game_code == "YNNE") {
        // The logo skip ends when the title starts (mq_on_dual_screen_3d).
        cfie::game = cfie::Game::MagiciansQuest;
    }
    else return false;
    return true;
}

void* cfie::find_hook(const char* name) {
    static const struct {
        const char* name;
        void* fn;
    } kHooks[] = {
        {"cfie_os_irq_handler", reinterpret_cast<void*>(&cfie_os_irq_handler)},
        {"mq_os_irq_handler", reinterpret_cast<void*>(&mq_os_irq_handler)},
        {"mq_perspective_aspect", reinterpret_cast<void*>(&mq_perspective_aspect)},
        {"mq_view_camera", reinterpret_cast<void*>(&mq_view_camera)},
        {"mq_view_of", reinterpret_cast<void*>(&mq_view_of)},
        {"mq_view_pass", reinterpret_cast<void*>(&mq_view_pass)},
        {"mq_text_box_draw", reinterpret_cast<void*>(&mq_text_box_draw)},
        {"mq_walk_speed", reinterpret_cast<void*>(&mq_walk_speed)},
        {"mq_coop_gate", reinterpret_cast<void*>(&mq_coop_gate)},
        {"mq_coop_var", reinterpret_cast<void*>(&mq_coop_var)},
        {"mq_coop_npc", reinterpret_cast<void*>(&mq_coop_npc)},
        {"mq_coop_triggers", reinterpret_cast<void*>(&mq_coop_triggers)},
        {"mq_dual_in_session", reinterpret_cast<void*>(&mq_dual_in_session)},
        {"mq_coop_set", reinterpret_cast<void*>(&mq_coop_set)},
        {"mq_var_store", reinterpret_cast<void*>(&mq_var_store)},
        {"mq_frame_end", reinterpret_cast<void*>(&mq_frame_end)},
        {"mq_on_dual_screen_3d", reinterpret_cast<void*>(&mq_on_dual_screen_3d)},
        {"cfie_on_bgm_play", reinterpret_cast<void*>(&cfie_on_bgm_play)},
        {"cfie_sound_update", reinterpret_cast<void*>(&cfie_sound_update)},
        {"cfie_spell_pad", reinterpret_cast<void*>(&cfie_spell_pad)},
        {"cfie_on_sound_start", reinterpret_cast<void*>(&cfie_on_sound_start)},
        {"cfie_pack_input", reinterpret_cast<void*>(&cfie_pack_input)},
        {"cfie_apply_input", reinterpret_cast<void*>(&cfie_apply_input)},
        {"cfie_analog_move", reinterpret_cast<void*>(&cfie_analog_move)},
        {"cfie_perspective_aspect", reinterpret_cast<void*>(&cfie_perspective_aspect)},
        {"cfie_player_animation_step", reinterpret_cast<void*>(&cfie_player_animation_step)},
        {"cfie_card_backup_request", reinterpret_cast<void*>(&cfie_card_backup_request)},
        {"cfie_on_movie_open", reinterpret_cast<void*>(&cfie_on_movie_open)},
        {"cfie_on_movie_frame", reinterpret_cast<void*>(&cfie_on_movie_frame)},
        {"cfie_on_movie_close", reinterpret_cast<void*>(&cfie_on_movie_close)},
        {"cfie_on_netcheck", reinterpret_cast<void*>(&cfie_on_netcheck)},
        {"cfie_guest_inputs_to_send", reinterpret_cast<void*>(&cfie_guest_inputs_to_send)},
        {"cfie_set_online_timing", reinterpret_cast<void*>(&cfie_set_online_timing)},
        {"ntr_nitro_save_context", reinterpret_cast<void*>(&ntr_nitro_save_context)},
        {"ntr_nitro_load_context", reinterpret_cast<void*>(&ntr_nitro_load_context)},
        {"ntr_nitro_init_context", reinterpret_cast<void*>(&ntr_nitro_init_context)},
        {"ntr_dsprot_not_detected", reinterpret_cast<void*>(&ntr_dsprot_not_detected)},
        {"ntr_task_swap", reinterpret_cast<void*>(&ntr_task_swap)},
        {"ntr_task_load", reinterpret_cast<void*>(&ntr_task_load)},
        {"ntr_os_spin_wait", reinterpret_cast<void*>(&ntr_os_spin_wait)},
    };
    for (const auto& h : kHooks)
        if (std::strcmp(h.name, name) == 0) return h.fn;
    return nullptr;
}
