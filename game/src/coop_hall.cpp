// Magician's Quest co-op: the auditorium together: Principal Sol's evaluation and the magic tournament (docs/COOP.md,
// "The evaluation and the magic tournament together"; docs/COOP_GAME.md, item 5).
//
// What the game does (static: event files 510, 521, 590 with the script decoder, the start tables, ov028's queries,
// overlay table; checked live where docs/COOP.md says so):
// - The evaluation is event 590. Asked, Principal Sol (590.2) says "come to the auditorium" when the student's
//   countdown s196 is 0 (s507 = 1); the auditorium (map 8, mc_ss block 8 record 5: s507 = 1) starts 590.1: the
//   student and three classmates (t131-t133) on stage, the student graded on its own record (the titles by mysteries,
//   the tournament s200, mischief, love, trends, festivals...; the rank s3 1-5 by the score of query 0x86 in t49,
//   with its hat), then s196 = 7, s507 = 0, s558 = 0 and back to the hall (map 6). The day roll (ov027 0x0211928C)
//   takes the days gone by off s196: one evaluation a week. On a rank-up the scene plays the staff roll: query 0xA0
//   loads overlay 25 (ov025 0x0210F76C, /title/stfrll_a.txt) and 0xA1 waits for its end. Overlay 25 shares its RAM
//   (0x0210F0C0) with overlay 23, the session's wireless code: in a session it cannot be loaded.
// - The tournament is event 521. The extracurricular teacher (actor 125) stands in town from the 21st of each month
//   for a student who knows four of the twelve incantations s296-s307 (510.1, s225 = 1); "Extracurricular lesson"
//   (bank 32 line 1672) enters (510.3: s225 = 2) and the town's record (mc_ss block 100 record 3: s225 = 2, s234 = 1)
//   starts 521.1: the teacher, the student and three classmates, then the auditorium (s225 = 3). There 521.2 (record 0
//   of block 8): three rounds, each against one classmate (t131, t132, t133): the teacher draws an incantation both
//   know (t80 = rand 0..11 until the student knows it: s305, s297, s299, s301, s306, s302, s307, s300, s303, s298, s296,
//   s304; t236 the incantation's id), "Ready, begin!", query 0xB1 opens the magic words (with t237 set, the words
//   cast set t238: 1 right, 2 wrong, main 0x02046D84), and the classmate's time is drawn by 0xAF (its skill, ov028
//   0x02119D84) and counted down by 0xB0 (t0: 0 not yet, 1 done first, 2 a miss). The first to finish wins the round;
//   a loss ends the tournament (to the hall, s225 = 4); three wins make the champion (s200 = 1, more on).
//
// What is built (only in a co-op session; message type 48):
// - The staff roll a session can't load is not loaded: query 0xA0 is passed over in a session (0xA1 then answers
//   "done" at once; "The staff roll will play once you're back home.", event 90041), and the staff roll is owed (a
//   marker beside the save, <save>.staffroll): once the student is home, free to move, out of any session, our event
//   90040 takes it to the auditorium (the roll draws on the top screen's 2D, which the auditorium has, as in the
//   scene; in the dorm the field's 3D covered it) and 90042 plays it there (the scene's music, 0xA0, 0xA1), then out
//   to the school's hall as the evaluation ends.
// - The other student is asked to go along to the other's evaluation or tournament (coop_myst.cpp, as a festival:
//   taken beside them, where its own record decides its own part).
// - The duel: when both students are in the tournament (521.2), the first round is between them. Each console holds
//   its scene at the rules (query 0x15) until the other's is there too (or 2 minutes while it is on its way); then on
//   both the first opponent (t131) is the other student (actor 110 + its slot; the script's moves, effects and names
//   act on it), the incantation is the host's draw among those both know, the round starts on both together (0xB1
//   held until both are there), and each casts with its own words. The classmate's clock (0xAF, 0xB0) is not used:
//   the round's result is the host's verdict from the two students' own times (frames from 0xB1 to the right words,
//   measured on each console): a console's right words (t238 = 1) wait for it; the winner's console goes on with
//   t238 = 1, the loser's with the opponent's "done" (t0 = 1). Rounds two and three are against classmates as alone.
// - The other student in the auditorium's scenes: a step beside in the evaluation, and in the duel at the opponent's
//   place (its own console puts it where this console puts its own student).
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "coop_sync.hpp"
#include "mq_student.hpp"
#include "ntr/coop.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

int mq_myst_joining();  // (game/src/coop_myst.cpp: the scene this console is on its way to join, or 0)

namespace {

namespace content = mq::content;

constexpr uint32_t kField = 0x020AD7EC;          // the field object (+0x3EB4 the area, +0x3EAE loading)
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the session)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+8 running, +0x23BC start kind, +0x2254 bytes)
constexpr uint32_t kEventNote = 0x020B8AAC;      // +2 the event's block, +6 its file, +8 its commands, +12 the one
                                                 // running (main 0x0201C4C8)
constexpr uint32_t kSetPlace = 0x020205A8;       // main (object, x, y, z, [sp] 0): an actor's place
constexpr uint32_t kNpcs = 0x3E48, kNpcCount = 0x3EAD, kNpcSize = 0x2B0, kNpcId = 0xA1;  // field+: the people
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF)
constexpr uint32_t kControlled = 0x020FE264;     // the actor the player controls (null outside the field)
constexpr uint32_t kSleep = 0x02003BFC;          // (frames), ARM: the task waits that many frames (the wait command's)
constexpr uint32_t kVarGet = 0x0201D960;         // (context, variable): a script variable
constexpr uint32_t kFieldArea = 0x3EB4, kFieldLoading = 0x3EAE;
constexpr uint8_t kMsgType = 48;  // (40 talk, 41 new day, 42 lessons, 43 mysteries, 44 incantations, 46 Gateway, 47 insects)
constexpr int kAuditorium = 8;
constexpr int kEvaluationFile = 590, kTournamentFile = 521;
constexpr int kOtherPlayer0 = content::kOtherPlayer0;
constexpr int32_t kBeside = 1 << 12;             // the other student a step beside in the evaluation
constexpr int32_t kOpposite = 4 << 12;           // ... and at the opponent's place in the duel (3, 9) -> (7, 9)
constexpr int kComingFrames = 120 * 60;          // the duel waits this long for the other student on its way
constexpr int kStale = 3 * 60;                   // the other's state counts this long after its last message
constexpr int kVerdictGrace = 90;                // the host waits this long for the other's time after the first
// t80's draw -> the student field that must be 1 (521.2 commands #95-#106)
constexpr int kDrawField[12] = {305, 297, 299, 301, 306, 302, 307, 300, 303, 298, 296, 304};

enum Msg : uint8_t {
    kState = 1,    // u16 file, u8 block, u8 area, u8 point, u16 spells, u8 pick (255 none), u8 duel, u8 coming
    kDone = 2,     // u16 elapsed frames
    kVerdict = 3,  // u8 the winner's slot
};
// Where this console's tournament is: at the rules waiting (kRules), past them without a duel (kAlone), or in the
// duel's first round (kCast waiting to begin, kCasting, kRoundOver).
enum Point : uint8_t { kNone = 0, kRules = 1, kCast = 2, kCasting = 3, kRoundOver = 4, kAlone = 5 };

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0, uint32_t a1 = 0, uint32_t a2 = 0, uint32_t a3 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0, c->r[1] = a1, c->r[2] = a2, c->r[3] = a3;
    ntr_call(c, fn);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

uint32_t call5(NtrCpu* c, uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[13] -= 8;
    ntr_wr32(c, c->r[13], a4);
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
    static const bool on = std::getenv("MQ_HALL_LOG") != nullptr;
    return on;
}

uint32_t ctx_of(NtrCpu* c) { return ntr_rd32(c, kScriptContext); }
uint8_t flag(NtrCpu* c, uint32_t var) {
    const uint32_t ctx = ctx_of(c);
    return ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (var - 10000)) : 0;
}
void set_flag(NtrCpu* c, uint32_t var, uint8_t v) {
    const uint32_t ctx = ctx_of(c);
    if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + (var - 10000), v);
}
// (r2 -1: the local student's record; 0 would read the host's on a guest, main 0x0201DA9C)
int var(NtrCpu* c, uint32_t v) { return int32_t(call(c, kVarGet | 1, ctx_of(c), v, 0xFFFFFFFFu)); }

bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ctx_of(c);
    return ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0);
}
bool running(NtrCpu* c) {
    const uint32_t ctx = ctx_of(c);
    return ram(ctx) && ntr_rd8(c, ctx + 8) != 0;
}
int event_file(NtrCpu* c) { return int16_t(ntr_rd16(c, kEventNote + 6)); }
int event_block(NtrCpu* c) { return int16_t(ntr_rd16(c, kEventNote + 2)); }
int area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? ntr_rd8(c, field + kFieldArea) : -1;
}
// The running command's index in its block (the interpreter's +12 from its start, +8), -1 unknown.
int command_index(NtrCpu* c) {
    const uint32_t start = ntr_rd32(c, kEventNote + 8), cur = ntr_rd32(c, kEventNote + 12);
    if (!ram(start) || !ram(cur) || cur < start || cur - start > 0x8000) return -1;
    int n = 0;
    for (uint32_t a = start; a < cur; n++) {
        const uint8_t size = ntr_rd8(c, a + 1);
        if (size < 2) return -1;
        a += size;
    }
    return n;
}

// A person of the field by number (the NPC entries), 0 none.
uint32_t person(NtrCpu* c, int id) {
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return 0;
    const uint32_t npcs = ntr_rd32(c, field + kNpcs);
    const int n = ram(npcs) ? int8_t(ntr_rd8(c, field + kNpcCount)) : 0;
    for (int i = 0; i < n; i++) {
        const uint32_t o = npcs + uint32_t(i) * kNpcSize;
        if (int(ntr_rd8(c, o + kNpcId)) == id) return o;
    }
    return 0;
}

bool free_to_move(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) && ntr_rd8(c, field + kFieldLoading) == 0 && ram(ntr_rd32(c, kControlled)) && !event_busy(c);
}

void put16(std::vector<uint8_t>& m, uint32_t v) { m.push_back(uint8_t(v)), m.push_back(uint8_t(v >> 8)); }
uint32_t get16(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8; }

struct Other {
    int seen = -1000000;
    int slot = -1;
    int file = -1, block = -1, area = -1;
    uint8_t point = kNone;
    uint16_t spells = 0;
    uint8_t pick = 255;
    bool duel = false;
    bool coming = false;  // on its way to the tournament (asked to go along, entering, the town's scene)
};

struct State {
    int frames = 0;
    bool listening = false;
    Other other;
    // this console
    uint8_t point = kNone;
    int sent_at = -1000;
    std::vector<uint8_t> last_sent;
    // the duel
    bool duel = false;        // this tournament's first round is against the other student
    bool decided = false;     // (duel or not, decided at the rules)
    uint8_t pick = 255;       // the host's draw (t80) for the first round
    bool picked = false;      // (set into t80 once)
    int cast_start = -1;      // frame of this console's 0xB1 in the duel round
    int my_time = -1;         // frames to the right words (-1 not yet)
    bool reported = false;
    int other_time = -1;      // (host) the other's time
    int first_known = -1;     // (host) frame the first time became known
    int winner = -1;          // the verdict's winner slot
    bool round_over = false;
    int32_t offset_x = 0, offset_z = 0;  // the auditorium's positions less its scripts' places (learned at 521.2's start)
    int classmate = -1;       // the first round's classmate (t131 before the other student took its place)
    bool restored = false;    // (the winner's console: t131 is the classmate again after the round)
    // the staff roll
    bool roll_owed = false;
    bool roll_there = false;  // (taken to the auditorium for it)
    bool roll_note = false;  // (the session's scene passed over it: say so once it has ended)
    bool roll_checked = false;
    int home_free = 0;
} st;

bool other_fresh() { return st.other.slot >= 0 && st.frames - st.other.seen <= kStale; }

std::string roll_path() {
    const std::string& pairs = cfie::coop_pairs_path;  // <save>.coop
    if (pairs.size() < 5) return {};
    return pairs.substr(0, pairs.size() - 5) + ".staffroll";
}

void owe_roll() {
    st.roll_owed = true;
    const std::string p = roll_path();
    if (!p.empty())
        if (FILE* f = std::fopen(p.c_str(), "wb")) std::fputs("1\n", f), std::fclose(f);
}
void roll_paid() {
    st.roll_owed = false;
    const std::string p = roll_path();
    if (!p.empty()) std::remove(p.c_str());
}

uint16_t known_spells(NtrCpu* c) {
    uint16_t m = 0;
    for (int i = 0; i < 12; i++)
        if (var(c, uint32_t(kDrawField[i])) == 1) m |= uint16_t(1u << i);
    return m;
}

bool in_tournament(int file, int block) { return file == kTournamentFile && block == 2; }

void send_state(NtrCpu* c, bool force) {
    const bool run = running(c);
    std::vector<uint8_t> m = {kState};
    put16(m, uint32_t(run ? event_file(c) : -1) & 0xFFFF);
    m.push_back(uint8_t(run ? event_block(c) : 255));
    m.push_back(uint8_t(area(c)));
    m.push_back(st.point);
    put16(m, known_spells(c));
    m.push_back(st.pick);
    m.push_back(st.duel ? 1 : 0);
    const int joining = mq_myst_joining();
    const bool coming = joining / 100 == kTournamentFile || (run && (event_file(c) == 510 || event_file(c) == kTournamentFile));
    m.push_back(coming ? 1 : 0);
    if (!force && m == st.last_sent && st.frames - st.sent_at < 30) return;
    st.last_sent = m;
    st.sent_at = st.frames;
    ntr::coop_send(kMsgType, m.data(), m.size());
}

void decide(NtrCpu* c);

void on_message(NtrCpu* c, uint32_t from, const uint8_t* d, size_t len) {
    (void)from;
    if (len < 1) return;
    switch (d[0]) {
    case kState: {
        if (len < 11) return;
        Other& o = st.other;
        o.seen = st.frames;
        o.file = int16_t(get16(d + 1)), o.block = d[3] == 255 ? -1 : d[3], o.area = d[4];
        o.point = d[5];
        o.spells = uint16_t(get16(d + 6));
        o.pick = d[8];
        o.duel = d[9] != 0;
        o.coming = d[10] != 0;
        // (the other's slot: the session's other player, as the town sync knows it)
        uint32_t nodes[8];
        (void)nodes;
        o.slot = coop_sync::own_slot() == 0 ? 1 : 0;
        if (!coop_sync::hosting() && st.duel && o.pick != 255) st.pick = o.pick;
        return;
    }
    case kDone:
        if (len < 3 || !coop_sync::hosting() || !st.duel) return;
        if (st.other_time < 0) {
            st.other_time = int(get16(d + 1));
            if (st.first_known < 0) st.first_known = st.frames;
            std::fprintf(stderr, "coop hall: the other student's right words in %d frames\n", st.other_time);
        }
        return;
    case kVerdict:
        if (len < 2 || coop_sync::hosting() || !st.duel || st.winner >= 0) return;
        st.winner = d[1];
        std::fprintf(stderr, "coop hall: the host's verdict: slot %d wins the duel\n", st.winner);
        return;
    }
    (void)c;
}

// The host's verdict, once it can be given: both times (the shorter wins; the host on a tie), or one time and the
// other has had kVerdictGrace frames since to send its own.
void judge(NtrCpu* c) {
    if (!coop_sync::hosting() || st.winner >= 0) return;
    const int own = coop_sync::own_slot(), other = st.other.slot;
    int w = -1;
    if (st.my_time >= 0 && st.other_time >= 0) w = st.my_time <= st.other_time ? own : other;
    else if (st.first_known >= 0 && st.frames - st.first_known > kVerdictGrace) w = st.my_time >= 0 ? own : other;
    else if (!other_fresh() && st.my_time >= 0) w = own;  // (the other has gone)
    if (w < 0) return;
    st.winner = w;
    const uint8_t m[2] = {kVerdict, uint8_t(w)};
    ntr::coop_send(kMsgType, m, sizeof m);
    std::fprintf(stderr, "coop hall: duel verdict: slot %d wins (host %d frames, guest %d frames)\n", w, st.my_time,
                 st.other_time);
    (void)c;
}

// The duel's round, each time the script looks at the result (query 0xB0 and the tests of t238 after it): the right
// words wait for the verdict; then the winner's console sees t238 = 1, the loser's the opponent's t0 = 1.
void duel_tick(NtrCpu* c, bool from_query) {
    if (st.round_over) return;
    const uint8_t t238 = flag(c, 10238);
    if (t238 == 1 && st.my_time < 0) {
        st.my_time = st.cast_start >= 0 ? st.frames - st.cast_start : 0;
        std::fprintf(stderr, "coop hall: the right words in %d frames\n", st.my_time);
        if (coop_sync::hosting()) {
            if (st.first_known < 0) st.first_known = st.frames;
        } else if (!st.reported) {
            std::vector<uint8_t> m = {kDone};
            put16(m, uint32_t(st.my_time));
            ntr::coop_send(kMsgType, m.data(), m.size());
            st.reported = true;
        }
    }
    if (st.my_time >= 0 && t238 == 1) set_flag(c, 10238, 0);  // (held until the verdict)
    judge(c);
    if (from_query) set_flag(c, content::kQueryResult, 0);
    if (st.winner < 0) return;
    const bool won = st.winner == coop_sync::own_slot();
    if (won) {
        set_flag(c, 10238, 1);
        set_flag(c, content::kQueryResult, 0);
    } else {
        set_flag(c, 10238, 0);
        set_flag(c, content::kQueryResult, 1);
    }
    st.round_over = true;
    st.point = kRoundOver;
    std::fprintf(stderr, "coop hall: duel over here: %s\n", won ? "this student won" : "the other student won");
}

// Hold the script (it runs on its own task: the wait command's sleep) until done() or the time is up.
template <typename F>
void hold(NtrCpu* c, int max_frames, F done) {
    if (log_all()) std::fprintf(stderr, "coop hall: hold in %d.%d #%d\n", event_file(c), event_block(c), command_index(c));
    for (int i = 0; i < max_frames && in_session(c) && !done(); i++) {
        send_state(c, false);
        call(c, kSleep, 1);
    }
}

// The rounds' classmates (t131-t133) each need a model: the scene waits on their moves (actor_action_wait). In a
// session the auditorium has two models for classmates (the other student's object holds one), so the room kept two
// of the three (or two others): a round's opponent without one is replaced by a classmate of the scene who has one
// (in the duel the first round is the other student's: the two others are needed; alone, one classmate meets the
// student twice). The scene's moves after the rules (the row, 521.2 #49-#52) then place them.
void fix_opponents(NtrCpu* c, bool duel) {
    auto modeled = [&](int id) {
        const uint32_t o = id > 0 ? person(c, id) : 0;
        return o && ntr_rd32(c, o + 0x20) != 0;
    };
    int ids[3];
    for (int i = 0; i < 3; i++) ids[i] = flag(c, uint32_t(10131 + i));
    std::vector<int> pool;
    for (int i = 0; i < 3; i++)
        if (modeled(ids[i])) pool.push_back(ids[i]);
    for (int i = duel ? 1 : 0; i < 3; i++) {
        if (modeled(ids[i]) || pool.empty()) continue;
        int pick = pool.front();
        for (int p : pool) {  // (one not already in another round, if any)
            bool used = false;
            for (int j = duel ? 1 : 0; j < 3; j++) used |= ids[j] == p;
            if (!used) { pick = p; break; }
        }
        std::fprintf(stderr, "coop hall: round %d's classmate %d has no model here: classmate %d instead\n", i + 1, ids[i], pick);
        ids[i] = pick;
        set_flag(c, uint32_t(10131 + i), uint8_t(pick));
    }
}

void decide(NtrCpu* c) {
    if (st.decided) return;
    st.decided = true;
    // (the other one waits at the rules, or has decided on the duel already: both decide the same)
    const bool both = other_fresh() && in_tournament(st.other.file, st.other.block) && st.other.area == kAuditorium &&
                      (st.other.point == kRules || (st.other.duel && st.other.point >= kRules && st.other.point < kAlone));
    const uint16_t common = uint16_t(known_spells(c) & st.other.spells);
    static const bool off = std::getenv("MQ_HALL_NO_DUEL") != nullptr;  // (research: the tournament as alone)
    st.duel = both && common != 0 && !off;
    fix_opponents(c, st.duel);
    if (!st.duel) {
        st.point = kAlone;
        std::fprintf(stderr, "coop hall: tournament without a duel (%s)\n",
                     !both ? "the other student is not in it" : off ? "MQ_HALL_NO_DUEL" : "no incantation both know");
        return;
    }
    if (coop_sync::hosting()) {  // (the host draws the first round's incantation among those both know)
        int n = 0, k[12];
        for (int i = 0; i < 12; i++)
            if (common & (1u << i)) k[n++] = i;
        st.pick = uint8_t(k[unsigned(std::rand() + st.frames) % unsigned(n)]);
    }
    // the first opponent is the other student; its classmate leaves the stage to it
    st.classmate = flag(c, 10131);
    if (const uint32_t o = person(c, st.classmate); !o || ntr_rd32(c, o + 0x20) == 0)
        st.classmate = flag(c, 10132);  // (the winner's round end walks it back to the row: one with a model)
    set_flag(c, 10131, uint8_t(kOtherPlayer0 + st.other.slot));
    if (const uint32_t o = person(c, st.classmate)) call5(c, kSetPlace, o, uint32_t(128 << 12), 0, 0, 0);
    std::fprintf(stderr, "coop hall: duel: the first round is against the other student (actor %d)%s\n",
                 kOtherPlayer0 + st.other.slot,
                 coop_sync::hosting() ? (", the host's draw " + std::to_string(st.pick)).c_str() : "");
}

void reset_duel() {
    st.point = kNone;
    st.duel = st.decided = st.picked = st.reported = st.round_over = false;
    st.pick = 255;
    st.cast_start = st.my_time = st.other_time = st.first_known = st.winner = -1;
    st.classmate = -1;
    st.restored = false;
}

}  // namespace

// From coop_talk's remote hook (main 0x02031404, a remote player's update): in the auditorium's scenes both play, the
// other student a step beside (the evaluation) or at the opponent's place (the duel). Returns true when it moved it.
bool mq_hall_remote(NtrCpu* c, uint32_t player) {
    if (!in_session(c) || area(c) != kAuditorium || !running(c) || !other_fresh()) return false;
    const int file = event_file(c), block = event_block(c);
    if (st.other.file != file || st.other.block != block) return false;
    const bool eval = file == kEvaluationFile && block == 1;
    const bool duel = in_tournament(file, block) && st.duel && st.other.duel && !st.round_over;
    if (!eval && !duel && !in_tournament(file, block)) return false;
    const int32_t dx = duel ? kOpposite : kBeside;
    for (uint32_t i = 0; i < 4; i++) {
        const uint32_t p = player + 0xE4 + i * 16;
        const uint8_t type = ntr_rd8(c, p);
        if ((type != 0x0F && type != 0x11) || ntr_rd8(c, p + 2) != uint8_t(kAuditorium)) continue;
        ntr_wr32(c, p + 4, ntr_rd32(c, p + 4) + uint32_t(dx));
    }
    return true;
}

// Before the other student is asked to go along to this console's scene there (coop_myst.cpp, as a festival's): the
// town's gathering for the tournament only to a student who can enter it this month (s225 1, or 2 entered), the
// tournament itself (to watch) not to one on its way to enter.
bool mq_hall_offer_ok(NtrCpu* c, int file, int block) {
    if (file != kTournamentFile) return true;
    const int entry = var(c, 225);
    if (block == 1) return entry == 1 || entry == 2;
    const bool coming = mq_myst_joining() / 100 == kTournamentFile ||
                        (running(c) && (event_file(c) == 510 || event_file(c) == kTournamentFile));
    return !coming && entry != 2;
}

// Before each event command 0x38 (from mq_coop_gate, the command's address): in the duel, the first draw (t80) is the
// host's, and the tests of t238 after the round's 0xB0 wait for the verdict.
void mq_hall_command(NtrCpu* c, uint32_t cmd) {
    if (!st.duel || !in_session(c) || !in_tournament(event_file(c), event_block(c))) return;
    const uint32_t v = ntr_rd32(c, cmd + 4);
    if (v == 10080 && ntr_rd8(c, cmd + 2) == 0 && ntr_rd32(c, cmd + 8) == 0 && !st.picked) {
        // (the guest: the host's draw may still be on its way)
        hold(c, 5 * 60, [] { return st.pick != 255; });
        if (st.pick != 255) {
            set_flag(c, 10080, st.pick);
            std::fprintf(stderr, "coop hall: the first round's incantation: draw %d (s%d)\n", st.pick, kDrawField[st.pick]);
        }
        st.picked = true;
    } else if (v == 10238 && st.point == kCasting) {
        duel_tick(c, false);
    }
}

int mq_class_query(NtrCpu* c, int q);  // (game/src/coop_class.cpp)
// ov028 0x0211343C (event command 0x53, a query: r0 the command, +2 the query, +4 its argument), instead.
extern "C" int mq_hall_query(NtrCpu* c) {
    const uint32_t cmd = c->r[0];
    if (!ram(cmd)) return 0;
    const int q = int16_t(ntr_rd16(c, cmd + 2));
    // (research, MQ_HALL_SCORE=<n>: the evaluation's score, query 0x86 (t0, then t49), reads n: 10 and more raise the
    // rank and play the staff roll, to see it without a year of play)
    if (static const char* score = std::getenv("MQ_HALL_SCORE"); score && q == 0x86) {
        set_flag(c, content::kQueryResult, uint8_t(std::atoi(score)));
        std::fprintf(stderr, "coop hall: research: the evaluation's score reads %d\n", std::atoi(score));
        c->r[0] = 1;
        return 1;
    }
    if (!in_session(c)) return 0;
    if (const int r = mq_class_query(c, q); r >= 0) return r;  // (homeroom's people: the host's, coop_class.cpp)
    const int file = event_file(c), block = event_block(c);
    if (log_all() && q != 0xAF && q != 0xB0 && (file == kTournamentFile || file == 510 || file == kEvaluationFile))
        std::fprintf(stderr, "coop hall: query %02x(%d) in %d.%d #%d, area %d\n", q, int16_t(ntr_rd16(c, cmd + 4)), file,
                     block, command_index(c), area(c));
    // The staff roll: overlay 25 would take the session's overlay 23's place. Owed, for home.
    if (q == 0xA0) {
        owe_roll();
        st.roll_note = true;
        std::fprintf(stderr, "coop hall: the staff roll can't play in a session: it plays once you're home\n");
        c->r[0] = 1;
        return 1;
    }
    if (!in_tournament(file, block)) return 0;
    if (q == 0x15 && st.point == kNone) {
        // The rules: both students' tournaments meet here.
        reset_duel();
        st.point = kRules;
        // (the set-up put the student at (4, 9): the auditorium's positions less its scripts' places)
        const mq::StudentPos me = mq::student_pos(c);
        st.offset_x = me.x - (4 << 12);
        st.offset_z = me.z - (9 << 12);
        std::fprintf(stderr, "coop hall: the auditorium's offset (%.2f, %.2f)\n", st.offset_x / 4096.0, st.offset_z / 4096.0);
        send_state(c, true);
        std::fprintf(stderr, "coop hall: the tournament's rules: waiting for the other student if it is coming\n");
        int waited = 0;
        hold(c, kComingFrames, [&] {
            waited++;
            if (!other_fresh()) return true;
            if (waited < 4 * 60 && in_session(c)) return false;  // (a moment for the other console to ask its student)
            if (in_tournament(st.other.file, st.other.block) && st.other.point >= kRules) return true;
            if (std::getenv("MQ_HALL_NO_WAIT")) return true;  // (research)
            return !st.other.coming;  // (not in it nor on its way)
        });
        std::fprintf(stderr, "coop hall: after %d frames at the rules\n", waited);
        decide(c);
        send_state(c, true);
        return 0;
    }
    if (!st.duel) return 0;
    if (q == 0xB1 && st.point == kRules) {
        // Round one's start: both together.
        st.point = kCast;
        send_state(c, true);
        hold(c, 60 * 60, [] { return !other_fresh() || (st.other.point >= kCast && st.other.duel); });
        st.point = kCasting;
        st.cast_start = st.frames;
        send_state(c, true);
        std::fprintf(stderr, "coop hall: duel round: begin\n");
        return 0;
    }
    if ((q == 0xAF || q == 0xB0) && st.point >= kCasting && ntr_rd32(c, cmd + 4) == 631 && !st.round_over) {
        if (q == 0xB0) duel_tick(c, true);
        c->r[0] = 1;
        return 1;
    }
    return 0;
}

// Once a frame (from mq_frame_end).
void mq_hall_frame(NtrCpu* c) {
    if (!st.listening) {
        coop_sync::listen(kMsgType, on_message);
        st.listening = true;
    }
    st.frames++;
    // (test driver, MQ_HALL_TEST=<path>: "words" there: the tournament's magic words are right, as the words cast
    // with t237 set would make them (t238 = 1); "miss": wrong (2). The file is removed.)
    if (static const char* test = std::getenv("MQ_HALL_TEST"); test) {
        if (FILE* f = std::fopen(test, "rb")) {
            char w[32] = {};
            if (std::fscanf(f, "%31s", w) != 1) w[0] = 0;
            std::fclose(f);
            std::remove(test);
            if (!std::strcmp(w, "words") || !std::strcmp(w, "miss")) {
                set_flag(c, 10238, w[0] == 'w' ? 1 : 2);
                std::fprintf(stderr, "coop hall: test: the words %s\n", w[0] == 'w' ? "right" : "wrong");
            }
        }
    }
    if (!st.roll_checked) {
        st.roll_checked = true;
        const std::string p = roll_path();
        if (!p.empty())
            if (FILE* f = std::fopen(p.c_str(), "rb")) std::fclose(f), st.roll_owed = true;
    }
    if (!in_session(c) || coop_sync::own_slot() < 0) {
        st.other = Other{};
        if (st.point != kNone) reset_duel();
        // The staff roll owed by a session's evaluation, once home and free to move for 3 seconds.
        if (st.roll_owed && !ntr::multiplayer_active()) {
            st.home_free = free_to_move(c) ? st.home_free + 1 : 0;
            if (st.home_free > (st.roll_there ? 30 : 180)) {
                // (to the auditorium first: the roll draws on the top screen's 2D, which the auditorium has, as in the
                // evaluation; then the roll there)
                const int ev = st.roll_there && area(c) == kAuditorium ? content::kEventStaffRollPlay : content::kEventStaffRoll;
                const uint32_t ctx = ctx_of(c);
                if (ram(ctx) && call(c, kQueueEvent | 1, uint32_t(ev / 100), uint32_t(ev % 100), 0xFF) != 0) {
                    ntr_wr8(c, ctx + 0x23BC, 2);
                    if (ev == content::kEventStaffRollPlay) {
                        roll_paid();
                        st.roll_there = false;
                        std::fprintf(stderr, "coop hall: the staff roll the session owed plays now\n");
                    } else {
                        st.roll_there = true;
                        std::fprintf(stderr, "coop hall: to the auditorium for the staff roll the session owed\n");
                    }
                }
                st.home_free = 0;
            }
        }
        return;
    }
    if (st.roll_note && free_to_move(c)) {
        const uint32_t ctx = ctx_of(c);
        if (ram(ctx) && call(c, kQueueEvent | 1, uint32_t(content::kEventStaffRollLater / 100),
                             uint32_t(content::kEventStaffRollLater % 100), 0xFF) != 0) {
            ntr_wr8(c, ctx + 0x23BC, 2);
            st.roll_note = false;
        }
    }
    // This console's tournament ended (or another event): the duel is over.
    if (st.point != kNone && !(running(c) && in_tournament(event_file(c), event_block(c)))) {
        if (log_all() || st.duel) std::fprintf(stderr, "coop hall: the tournament ended here\n");
        reset_duel();
    }
    if (st.duel && st.point >= kCasting) judge(c);
    // The winner's console: the round's end walks the opponent back to its place in the row (521.2 #349, then waits
    // for it to arrive), which the other student, gone on to its own tournament's end, never does; from the winner's
    // announcement (#338) the first round's classmate is t131 again, back in its place (6.5, 7) to walk there.
    if (st.duel && st.round_over && !st.restored && st.winner == coop_sync::own_slot() && st.classmate > 0 &&
        command_index(c) >= 338) {
        st.restored = true;
        set_flag(c, 10131, uint8_t(st.classmate));
        if (const uint32_t o = person(c, st.classmate))
            call5(c, kSetPlace, o, uint32_t(int32_t(6.5 * 4096) + st.offset_x), 0, uint32_t((7 << 12) + st.offset_z), 0);
        std::fprintf(stderr, "coop hall: the first round's classmate %d back in the row\n", st.classmate);
    }
    send_state(c, false);
}
