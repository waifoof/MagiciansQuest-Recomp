// Magician's Quest co-op: how long a visit lasts (docs/COOP.md, "How a visit ends").
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cfie_game.hpp"
#include "coop_connect.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/coop_net.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the wireless session, null when alone)
constexpr uint32_t kField = 0x020AD7EC;          // the town/field object
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+0x2254 the flag bytes, +0x30 the text box)
constexpr uint32_t kTown = 0x020AF020;           // the town save block (+0x2CC hour, +0x2CD minute)

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FD000; }

constexpr uint32_t kSessionPtr = 0xA28;     // game+: the wireless session (null when alone)
constexpr uint32_t kHostByte = 0x6912;      // session+: nonzero on the host (its connect save writes the town too)
constexpr uint32_t kFieldChange = 0x3EAE;   // field+: the change (warp) under way (4: the day change)
constexpr uint32_t kFieldSession = 0x3EAF;  // field+: the field was loaded in a session
constexpr uint32_t kFieldPeriods = 0x3E80;  // field+: the day's periods (8 bytes a season: half hours they start at)
constexpr uint32_t kTownTime = 0x2C8;       // town+: today (year - 2000, month - 1, day - 1, weekday), then the time
constexpr uint32_t kTownHour = 0x2CC, kTownMinute = 0x2CD;
constexpr uint32_t kLastDay = 0x9008;       // town+: the day last rolled (year - 2000, month - 1, day - 1, weekday)
constexpr uint32_t kControlled = 0x020FE264;  // ov001: the actor the player controls (null outside the field)
constexpr uint32_t kEventRunning = 0x0201DF58;  // main (2): an event runs or is queued
constexpr uint32_t kPad = 0x020ACAA4;       // the game's pad (+4 held, +8 pressed this frame, +0xA repeated)
constexpr uint32_t kTextBox = 0x30, kMessage = 0x1470, kMessageBlock = 0x146C, kMenu = 0x1CE4, kCursor = 0x294;
constexpr uint16_t kA = 1u << 0, kSelect = 1u << 2, kUp = 1u << 6;
// System messages (bank 999) of the game's own Disconnect (Select, in a session)
constexpr uint32_t kAskDisconnect = 111, kDisconnectMenu = 112, kDisconnectedHome = 110, kDisconnecting = 115,
                   kSaved = 118;  // (110 "The person you were playing with has disconnected", 115-118 to "Save complete")
constexpr uint8_t kDayEndsMessage = 41;     // ours, on the side channel: the host ends the visit for the new day
constexpr int kFreeFrames = 120;            // free to move this long before the host ends the visit for the day
constexpr int kPressEvery = 8;              // frames from one press to the next
constexpr int kEndingFrames = 60 * 60;
constexpr int kHomeFrames = 60 * 8;        // free to move at home this long before connecting again      // the game's Disconnect is left to the player after this long

uint32_t session(NtrCpu* c) {
    const uint32_t game = ntr_rd32(c, kGame);
    return ram(game) ? ntr_rd32(c, game + kSessionPtr) : 0;
}

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

bool day_off() {
    static const bool v = std::getenv("MQ_COOP_DAY_GAME") != nullptr;  // (research: the game's own frozen day)
    return v;
}

// Days since 2000-01-01 of a date as the game keeps it (year - 2000, month - 1, day - 1).
int days(int y, int m, int d) {
    y += 2000, m += 1, d += 1;
    y -= m <= 2;
    const int era = y / 400, yoe = y - era * 400;
    const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 730425;
}

// The host's town is due a new day: today, counting the night (the season's first period, which starts at 3:00 to
// 4:30) as the day before, as the field's time does (ov001 0x020C8E66), is after the day last rolled.
bool day_due(NtrCpu* c, uint32_t field) {
    const uint32_t t = kTown + kTownTime, l = kTown + kLastDay;
    const int month = int8_t(ntr_rd8(c, t + 1));
    if (month < 0 || month > 11) return false;
    int today = days(int8_t(ntr_rd8(c, t)), month, int8_t(ntr_rd8(c, t + 2)));
    const uint32_t periods = ntr_rd32(c, field + kFieldPeriods);
    if (!ram(periods)) return false;
    const int season = ((month - 2 + 12) % 12) / 3;
    const int first = int8_t(ntr_rd8(c, periods + uint32_t(season) * 8));
    const int half = ntr_rd8(c, kTown + kTownHour) * 2 + (ntr_rd8(c, kTown + kTownMinute) >= 30);
    if (half < first) today--;
    const int last = days(int8_t(ntr_rd8(c, l)), int8_t(ntr_rd8(c, l + 1)), int8_t(ntr_rd8(c, l + 2)));
    return today > last;
}

enum class Day { Idle, Ending, Home };
struct DayEnd {
    Day state = Day::Idle;
    bool host = false;
    int free = 0;     // frames in a row the host was free to move
    int frames = 0;   // since the end began
    uint32_t session = 0;
    uint32_t message = 0;  // the system message shown last
    bool seen = false;     // the host: "Disconnect?" has shown
    bool again = true;     // connect again afterwards (not when a stand-in leaves: coop_net.cpp)
} day;

void day_ends(NtrCpu* c, uint32_t, const uint8_t*, size_t) {
    if (day.state != Day::Idle || day_off()) return;
    day = DayEnd{Day::Ending, false, 0, 0, session(c), 0, false};
    std::fprintf(stderr, "coop day: the host ends the visit for the new day; joining again afterwards\n");
}

// (research, MQ_COOP_SESSION_LOG=1: the session's state bytes, the field's change kind and the message shown, on
// each change)
void log_state(NtrCpu* c) {
    static const bool on = std::getenv("MQ_COOP_SESSION_LOG") != nullptr;
    if (!on) return;
    static unsigned frame = 0;
    frame++;
    const uint32_t game = ntr_rd32(c, kGame);
    const uint32_t s = ram(game) ? ntr_rd32(c, game + 0xA28) : 0;
    const uint32_t field = ntr_rd32(c, kField), ctx = ntr_rd32(c, kScriptContext);
    char now[256];
    int msg = -1;
    if (ram(ctx)) {
        const uint32_t box = ctx + 0x30;
        msg = int(ntr_rd32(c, box + 0x1470) + 100 * ntr_rd32(c, box + 0x146C));
    }
    std::snprintf(now, sizeof now,
                  "session %08x state %d event %d msg %d 6912 %d 6919 %d | flag212 %d | field change %d insess %d area %d | "
                  "date %08x | clock %02d:%02d | box %d",
                  s, ram(s) ? int8_t(ntr_rd8(c, s + 0x6917)) : -9, ram(s) ? int8_t(ntr_rd8(c, s + 0x691C)) : -9,
                  ram(s) ? int8_t(ntr_rd8(c, s + 0x691F)) : -9, ram(s) ? int8_t(ntr_rd8(c, s + 0x6912)) : -9,
                  ram(s) ? int8_t(ntr_rd8(c, s + 0x6919)) : -9, ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + 212) : -1,
                  ram(field) ? int8_t(ntr_rd8(c, field + 0x3EAE)) : -9, ram(field) ? ntr_rd8(c, field + 0x3EAF) : -1,
                  ram(field) ? ntr_rd8(c, field + 0x3EB4) : -1, ntr_rd32(c, kTown + 0x2C8), ntr_rd8(c, kTown + 0x2CC), ntr_rd8(c, kTown + 0x2CD), msg);
    static char last[256];
    // (the clock alone changing is not logged)
    auto strip = [](const char* a, char* out) {
        std::snprintf(out, 256, "%s", a);
        if (char* k = std::strstr(out, "| clock")) *k = 0;
    };
    char a[256], b[256];
    strip(now, a);
    strip(last, b);
    if (std::strcmp(a, b) != 0) std::fprintf(stderr, "coop session [%u]: %s\n", frame, now);
    std::snprintf(last, sizeof last, "%s", now);
}

// The new day in a session (docs/COOP.md, "How a visit ends").
//
// The game never ends a visit for the time of day, but while connected the day does not turn: the field's time
// (ov001 0x020C8CD4) leaves the day change out when the field was loaded in a session (0x020C8E02), and a field's
// start of day (ov001 0x020D5C14) rolls the new day only without a session (0x020D5CFE). It cannot: the roll loads
// overlays 25, 3 and 27 into the slots the session's own code (overlay 23) uses; run in a session the game ended
// ("interpreter left guest code", tried). So the day turns for everyone together the game's own way: once the host's
// town is due a new day (3:30 in autumn) and the host is free to move, the host tells the guests, and ends the visit
// with the game's Disconnect (Select, Yes: each console saves, "Save complete", and goes home), every console's
// own town turns the day as when alone, and then each connects again with Connect now in its role: the guests
// arrive in the host's new day.
void day_frame(NtrCpu* c) {
    if (day_off() || !cfie::coop_anytime) return;
    const uint32_t s = session(c), field = ntr_rd32(c, kField), ctx = ntr_rd32(c, kScriptContext);
    // (a stand-in, co-op over the internet: its player's console ends the visit; it only follows, coop_net.cpp)
    if (day.state == Day::Idle && ntr::net_config().standin) return;
    if (day.state == Day::Idle) {
        if (!ram(s) || ntr_rd8(c, s + kHostByte) == 0 || !ram(field) || !ram(ctx) ||
            ntr_rd8(c, field + kFieldSession) == 0 || !ntr::multiplayer_active()) {
            day.free = 0;
            return;
        }
        const bool free = ntr_rd8(c, field + kFieldChange) == 0 && ram(ntr_rd32(c, kControlled)) &&
                          ntr_rd8(c, ctx + 8) == 0 && ntr_rd8(c, ctx + 0x23BC) == 0 && call(c, kEventRunning, 2) == 0;
        if (!free || !day_due(c, field)) {
            day.free = 0;
            return;
        }
        if (++day.free < kFreeFrames) return;
        day = DayEnd{Day::Ending, true, 0, 0, s, 0, false};
        ntr::coop_send(kDayEndsMessage, nullptr, 0);
        std::fprintf(stderr, "coop day: a new day in the host's town: the visit ends for it, and starts again\n");
        return;
    }
    if (day.state == Day::Ending) {
        ++day.frames;
        if (ram(s) && day.frames > kEndingFrames) {
            std::fprintf(stderr, "coop day: the visit did not end: left to the players\n");
            day = DayEnd{};
            return;
        }
        if (ram(s)) return;
        day.state = Day::Home;  // (the session is gone: the field goes home, and turns its day)
        day.free = 0;
        std::fprintf(stderr, "coop day: the visit ended; home for the new day\n");
        return;
    }
    if (day.state == Day::Home) {
        // Free to move at home for a while first: the new day's own scenes (the town's start records, e.g. the
        // warden's) start before Connect now asks for the room.
        const bool free = !ram(s) && ram(field) && ram(ctx) && ntr_rd8(c, field + kFieldChange) == 0 &&
                          ram(ntr_rd32(c, kControlled)) && ntr_rd8(c, ctx + 8) == 0 && ntr_rd8(c, ctx + 0x23BC) == 0 &&
                          call(c, kEventRunning, 2) == 0;
        day.free = free ? day.free + 1 : 0;
        if (day.free < kHomeFrames) return;
        if (!day.again) {
            day = DayEnd{};
            return;
        }
        coop_connect::request(day.host ? coop_connect::Role::Host : coop_connect::Role::Join);
        std::fprintf(stderr, "coop day: connecting again as %s\n", day.host ? "host" : "guest");
        day = DayEnd{};
    }
}

}  // namespace

namespace coop_session {
// The game's own Disconnect, pressed as a player would (a stand-in whose player left: coop_net.cpp); nothing after.
void leave(NtrCpu* c) {
    if (day.state != Day::Idle) return;
    day = DayEnd{Day::Ending, true, 0, 0, session(c), 0, false};
    day.again = false;
    std::fprintf(stderr, "coop day: leaving the visit (Disconnect)\n");
}
bool leaving() { return day.state != Day::Idle; }
}  // namespace coop_session

// From mq_coop_pad (main 0x02003628, the pad just read): while the visit ends for the day, the buttons a player
// would press: the host Select, then Yes to "Disconnect?"; everyone A for the messages until home.
void mq_coop_session_pad(NtrCpu* c, uint32_t pad) {
    if (day.state != Day::Ending || pad != ntr_rd32(c, kPad) || !ram(pad)) return;
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || day.frames % kPressEvery != 0) return;
    const uint32_t message = ntr_rd32(c, ctx + kTextBox + kMessage) + 100 * ntr_rd32(c, ctx + kTextBox + kMessageBlock);
    const int cursor = int16_t(ntr_rd16(c, ctx + kMenu + kCursor));
    if (message != day.message) day.message = message, day.seen = day.seen || message == kAskDisconnect;
    uint16_t press = 0;
    const bool ending = message == kDisconnectedHome || (message >= kDisconnecting && message <= kSaved);
    if (day.host && !day.seen) {
        // (Select opens "Disconnect?"; again if it has not shown after a while)
        if (ram(session(c)) && day.frames % (kPressEvery * 30) == 0) press = kSelect;
    } else if (message == kDisconnectMenu) {
        press = cursor > 0 && cursor < 8 ? kUp : kA;  // (Yes is the first)
    } else if (message == kAskDisconnect || ending) {
        press = kA;
    }
    if (!press) return;
    ntr_wr16(c, pad + 4, uint16_t(ntr_rd16(c, pad + 4) | press));
    ntr_wr16(c, pad + 8, uint16_t(ntr_rd16(c, pad + 8) | press));
    ntr_wr16(c, pad + 0xA, uint16_t(ntr_rd16(c, pad + 0xA) | press));
}

// Once a frame (from mq_frame_end).
void mq_coop_session_frame(NtrCpu* c) {
    static bool listening = false;
    if (!listening) coop_sync::listen(kDayEndsMessage, day_ends), listening = true;
    log_state(c);
    day_frame(c);
}
