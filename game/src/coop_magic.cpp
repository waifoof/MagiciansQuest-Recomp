// Magician's Quest co-op: every incantation in a session (docs/COOP.md, "Incantations in a session"; docs/
// COOP_GAME.md, items 4 and 10).
//
// What the game does. Casting (the touch screen's words) ends in main 0x02046FF8: the incantation's record (the table
// of /magic/magic_charm_comp.bin, 92 bytes each: +48 its event file, +78 its index in the results table) gives the
// result id (the results table /magic/magic_result_comp.bin, 36 bytes each, +2), which goes to flag byte 199, and the
// event's block 1 starts. In a session the events refuse some incantations:
// - the summons and transforms (World Tree 332, Phantasm 350/351, Candyman 352, Werewolf 353, Homunculus 355,
//   Familiar 356, Golem 357, Charm 359) begin with "if t212 != 0 go to <the refusal>" (flag byte 212: the console's
//   slot + 1 while connected), the refusal ending in system message 3120 "This incantation cannot be used during
//   wireless communications";
// - query 0xB5 (ov028 0x0211A16C, "may this incantation be used", by the result id in t199) answers 0 in a session
//   for Connect doors (31), Sweet dreams (33), Make peace (35), Sit by me (36), Honor student (41) and Mushroom (66)
//   (messages 3114-3119); for Rainbow (26), Flower rain (29) and Rainmaking (50) it asks the other console (packet
//   0x45) and for Friendship (30) and Declare love (32) it looks for the other student casting with this one: those
//   five are the game's own two-player incantations;
// - query 0x16 (ov028 0x02114E38), the end of an incantation's event (ov001 0x020FAF14 on field+0x12930: the five
//   words of the incantation at +0x984 / +0x98E cleared, the word display redrawn, +0x998 cleared), does nothing in a
//   session.
// - Rainbow, Flower rain and Rainmaking are the game's own two-player path: the caster's block 1 writes the town's
//   g1100/g1101 (and Rainmaking g1400), the other console runs block 4 (query 0x0D, the same weather): both show it.
//
// What is built (only in a co-op session; message type 44 on the side channel):
// - The refusals of the incantations below are lifted (the event's test, a RAM copy made again for each event, made
//   one that never holds): the caster's console runs the event as alone, for its own student.
// - The end-of-spell cleanup runs for those (query 0x16's own call, when the event ends), so the words do not stay.
// - The other console is told: "<name> cast the <incantation> incantation!" (events 90021/90022), with the cast's
//   sparkle at the caster when both stand in the same area.
// - A guest's cast that changes the town (kHost) sends its writes of the host's variables to the host, which writes
//   them if the week's mystery is unchanged; the town sync brings them back to everyone.
// - The host sends its town's weather bytes to the guests as soon as they change; a guest's sky is made again when
//   the host's weekly weather table or the week's mystery arrive changed (sky_follow).
// - (test driver, MQ_COOP_CAST_FILE=<path>: "<result id>" in the file casts that incantation (t199 and the event's
//   block 1, as the magic menu would after the words); 30 and 32 are coop_talk's.)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

namespace content = mq::content;

// The game's addresses (USA, YNNE).
constexpr uint32_t kField = 0x020AD7EC;          // the field object
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the session)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+0x2254: flag bytes; +0x23BC start kind)
constexpr uint32_t kEventNote = 0x020B8AAC;      // +2 the running event's block, +6 its file
constexpr uint32_t kEventRunning = 0x0201DF58;   // (2): an event runs or is queued
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kSpellEnd = 0x020FAF14;       // (field + kSpellWords): the words of the last incantation cleared
constexpr uint32_t kSpellWords = 0x12930;
constexpr uint32_t kCharms = 0x020B8E2C;          // the incantations' table (+0 records of kCharmSize, +4 count)
constexpr uint32_t kCharmSize = 92;              // (+48 the event file, +78 the result's index)
constexpr uint32_t kCast = 0x02047A10;           // (record): the incantation cast (after t199 is set)
constexpr uint32_t kControlled = 0x020FE264;     // the actor the player controls (null outside the field)
constexpr uint32_t kSpellId = 10199;             // t199: the incantation's result id
constexpr uint32_t kSlotFlag = 10212;            // t212: the console's slot + 1 while connected
constexpr uint32_t kTown = 0x020AF020, kTownSize = 0x993C;  // the town block
constexpr uint32_t kStore = kTown + 0x2D8;       // the town's variable store (g1000 + index)
constexpr uint32_t kStoreWrite = 0x0201E070;     // (store, index, value)
constexpr uint32_t kChecksum = 0x02011D9C;       // (block, size)
constexpr uint32_t kWeatherBytes = 0x3F4;        // the town's weather (store indexes 540-559: 20 bytes)
// The sky (docs/COOP.md, "The guest's sky"): its kind for a time of day is worked out at a field load from the date,
// the town's weekly weather table (town block +0x6D1C, rows of 8 kinds by weekday; main 0x0200A178), the weather
// incantations in effect (g1081-g1100), the week's mystery (g1000 = 42 is a sky of its own) and the day's Anywhere
// flag; ov001 0x020CE81C (field, -1) does it again as the load does (0x020CE760: 0x020D8BD8, then the sky's set-up
// 0x020D8738; then the field's vtable +120), no roll; the scripts' weather queries call it too (0x0D, 0x9E, 0x9F,
// with a kind).
constexpr uint32_t kWeekWeather = 0x6D1C, kWeekWeatherSize = 0x60;
constexpr uint32_t kSkyMake = 0x020CE760;        // (field, -1): the sky's kind and set-up, as a load does (ov001)
constexpr uint32_t kFieldLight = 120;             // ... then the field's vtable +120 (field), as 0x020CE81C does
// The field: +0x3E4C the other players' objects (3 x 0x638; +0x5A0 set while in this area, +0x62C session slot),
// +0x3EB4 the area shown, +0x3EAE loading.
constexpr uint32_t kRemotePlayers = 0x3E4C, kRemoteSize = 0x638, kRemoteSlot = 0x62C;
constexpr uint32_t kFieldArea = 0x3EB4, kFieldLoading = 0x3EAE;

enum Lift : uint8_t {
    kNone = 0,
    kOwn,     // the student's own business: the event runs as alone
    kHost,    // changes the town: the host decides
};

struct Spell {
    int id;           // the result id (t199)
    int event;        // its event file (block 1 is the cast)
    const char* name;
    Lift lift;
};

// Every incantation with an event of its own (the charm table), and what co-op does with it.
constexpr Spell kSpells[] = {
    {26, 310, "Rainbow", kNone},  // (the game's own two-player incantations: left as the game does them)
    {29, 313, "Flower rain", kNone},
    {50, 334, "Rainmaking", kNone},
    {30, 320, "Friendship", kNone},  // (made to work between players: coop_talk.cpp)
    {32, 344, "Declare love", kNone},
    // Summons and transforms (the t212 refusal). Their effect is the cast's scene on the caster's console (a
    // sparkle, a flash, "You succeeded in using..."; Phantasm's fortune-teller and Candyman's scene with their
    // creature) and a few of the town's variables, which are the host's: Golem g1006 = 3, Familiar g1310 = 1,
    // Candyman the mystery's stage (g1001, g1011, g1054, g1070, g1010).
    {85, 332, "World Tree", kOwn},
    // (Phantasm stays refused. The magic menu casts it only in the dorm's washrooms (maps 4 and 5: its records 8 and
    // 11, field+0x3EB5 = 1, area 4 or 5; main 0x02047650). Its scene reloads the map (query 0x14(1) holds the white,
    // warp_back) and the load (ov001 0x020CDF2C) runs the event's nowait block itself (main 0x0201C4C8 in load mode)
    // while the event's own thread, let go by warp_back, steps the same commands: alone the load runs through first;
    // in a session the load waits on the session, the thread reaches query 0x11 (waits for field+0x3E7C) and the
    // load's run ends the event (a zero-size command, main 0x0201C542): the fortune-teller never comes and the
    // student stays behind a white screen. Holding the event's thread (context +9) for the load stalled the load.
    // What it would do is the caster's own (s69, s479) and two ordinary town variables, g1383 and g1410: docs/COOP.md)
    {34, 350, "Phantasm", kNone},
    {67, 352, "Candyman", kHost},
    {64, 353, "Werewolf", kOwn},
    {69, 355, "Homunculus", kOwn},
    {70, 356, "Familiar", kHost},
    {71, 357, "Golem", kHost},
    {72, 359, "Charm", kOwn},
    // Query 0xB5's refusals. Their effect is the result the magic menu applies before the event (main 0x02045500:
    // the caster's own record and the classmates' values toward the caster's slot, which a session keeps per
    // console) and the event: Connect doors the caster's own s49/s50 (query 0x3F), Sweet dreams a dream scene of the
    // caster's, Mushroom the town's g1096/g1121 (ordinary town variables: a guest's go to the host as requests).
    {31, 358, "Connect doors", kOwn},
    {33, 345, "Sweet dreams", kOwn},
    {35, 321, "Make peace", kOwn},
    {36, 322, "Sit by me", kOwn},
    {41, 346, "Honor student", kOwn},
    {66, 349, "Mushroom", kHost},
};

const Spell* spell_by_id(int id) {
    for (const Spell& s : kSpells)
        if (s.id == id) return &s;
    return nullptr;
}
const Spell* spell_by_event(int file) {
    for (const Spell& s : kSpells)
        if (s.event == file) return &s;
    if (file == 351) return spell_by_id(34);  // (Phantasm's second event)
    return nullptr;
}

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

bool in_session(NtrCpu* c) {
    static const bool solo = std::getenv("MQ_COOP_SOLO") != nullptr;  // (research: as if in one)
    return cfie::coop_anytime && (solo || (ntr::multiplayer_active() && ram(session(c))));
}

bool log_on() {
    static const bool on = std::getenv("MQ_MAGIC_LOG") != nullptr;
    return on;
}

uint8_t flag(NtrCpu* c, uint32_t var) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (var - 10000)) : 0;
}
void set_flag(NtrCpu* c, uint32_t var, uint8_t value) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + (var - 10000), value);
}

int event_file(NtrCpu* c) { return int(int16_t(ntr_rd16(c, kEventNote + 6))); }

// An event runs (the script context's +8) or is about to (main 0x0201DF58 with 2, the talk's own test). (The start
// kind at +0x23BC can stay set after the other console's block of a two-player incantation, Rainbow's block 4,
// with nothing pending.)
bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return !ram(ctx) || ntr_rd8(c, ctx + 8) != 0 || call(c, kEventRunning, 2) != 0;
}

bool queue_event(NtrCpu* c, int id) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) == 0) return false;
    ntr_wr8(c, ctx + 0x23BC, 2);
    return true;
}

// Our messages (type kMsgType on the side channel).
constexpr uint8_t kMsgType = 44;  // (40 talk, 41 new day, 42 lessons, 43 mysteries)
enum Op : uint8_t {
    kOpCast = 1,     // an incantation cast here (as alone): id, the caster's area
    kOpWrites = 2,   // guest -> host: the town variables the cast wrote (id, mystery, stage, n, (index u16, value u8)*n)
    kOpWeather = 3,  // host -> guests: the town's weather bytes changed (20 bytes)
};
constexpr int kShowWithin = 60 * 60;   // a cast is shown on the other console if it is free within this long

struct Shown {
    int id = 0, slot = -1, area = -1, until = 0;
};

struct Write {
    uint16_t index;
    uint8_t value;
};

struct State {
    const Spell* lifted = nullptr;  // the incantation whose event runs here with its refusal lifted
    int lifted_file = -1;           // ... its event file
    uint8_t mys = 0, stg = 0;       // the town's mystery and stage when it started
    std::vector<Write> writes;      // guest: the host's variables it wrote
    std::vector<Shown> shows;       // casts of the other students to show here
    int frames = 0;
    bool listening = false;
    uint8_t weather[20] = {};       // host: the weather bytes last sent
    bool weather_known = false;
    // guest: what the sky was last made from (the week's table, g1000), the field it was made in, frames free
    uint8_t sky_from[kWeekWeatherSize + 1] = {};
    uint32_t sky_field = 0;
    int sky_area = -1, sky_free = 0;
    bool sky_known = false;
    int cast_frames = 0;            // (test driver)
    int cast_id = 0;
} st;

void put16(std::vector<uint8_t>& m, uint32_t v) { m.push_back(uint8_t(v)), m.push_back(uint8_t(v >> 8)); }
uint32_t get16(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8; }

// The refusal of a summon or transform: "if t212 != 0 go to <refusal>", "go to <the cast>", then the refusal's
// commands up to system message 3120.
bool summon_refusal(NtrCpu* c, uint32_t cmd) {
    if (ntr_rd8(c, cmd) != 0x38 || ntr_rd32(c, cmd + 4) != kSlotFlag || ntr_rd8(c, cmd + 2) != 1 || ntr_rd32(c, cmd + 8) != 0)
        return false;
    const uint32_t go = cmd + ntr_rd8(c, cmd + 1);
    if (ntr_rd8(c, go) != 0x37) return false;
    uint32_t a = go + ntr_rd8(c, go + 1);
    for (int i = 0; i < 16; i++) {
        const uint8_t op = ntr_rd8(c, a), size = ntr_rd8(c, a + 1);
        if (op == 0x67) return ntr_rd16(c, a + 2) == 3120;
        if (op == 0x42 || size == 0) return false;
        a += size;
    }
    return false;
}

// Query 0xB5's refusal: "query 0xB5" then "if t0 == 0 go to <refusal>".
bool b5_refusal(NtrCpu* c, uint32_t cmd) {
    return ntr_rd8(c, cmd) == 0x38 && ntr_rd32(c, cmd + 4) == 10000 && ntr_rd8(c, cmd + 2) == 0 &&
           ntr_rd32(c, cmd + 8) == 0 && ntr_rd8(c, cmd - 8) == 0x53 && ntr_rd8(c, cmd - 7) == 8 &&
           ntr_rd16(c, cmd - 6) == 0xB5;
}

// (research, MQ_MAGIC_LIFT=<id>,<id>...: those incantations' refusals lifted too)
bool research_lift(int id) {
    static const char* ids = std::getenv("MQ_MAGIC_LIFT");
    if (!ids) return false;
    const std::string list = std::string(",") + ids + ",", key = "," + std::to_string(id) + ",";
    return list.find(key) != std::string::npos;
}

void never_holds(NtrCpu* c, uint32_t cmd) {
    ntr_wr8(c, cmd + 2, 0);        // ==
    ntr_wr32(c, cmd + 8, 0x7FFF);  // a value the byte never has
}

void cast_test(NtrCpu* c) {
    static const char* path = std::getenv("MQ_COOP_CAST_FILE");
    if (!path) return;
    if (st.cast_id == 0) {
        FILE* f = std::fopen(path, "rb");
        if (!f) return;
        int id = 0;
        if (std::fscanf(f, "%d", &id) != 1) id = 0;
        std::fclose(f);
        if (id == 30 || id == 32) return;  // (coop_talk's)
        std::remove(path);
        if (!spell_by_id(id)) return;
        st.cast_id = id;
        st.cast_frames = 0;
    }
    const Spell* s = spell_by_id(st.cast_id);
    // As the magic menu after the words (main 0x02047A6C): t199, then the record's cast (main 0x02047A10: t202, the
    // result applied by main 0x02045500, the event started).
    const uint32_t table = ntr_rd32(c, kCharms), count = ntr_rd32(c, kCharms + 4);
    uint32_t rec = 0;
    for (uint32_t i = 0; ram(table) && i < count && i < 64 && !rec; i++)
        if (int16_t(ntr_rd16(c, table + i * kCharmSize + 48)) == s->event) rec = table + i * kCharmSize;
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    const bool free = ram(ntr_rd32(c, kControlled)) && ram(ctx) && !event_busy(c);
    if (st.cast_frames == 0)
        std::fprintf(stderr, "coop magic: test cast of %s asked (record %08x, table %08x/%u, free %d)\n", s->name, rec,
                     table, count, free ? 1 : 0);
    if (rec && free) {
        // The magic menu casts only what the game's own conditions allow here (main 0x020478A8, before the cast:
        // time, items, place...). The place test (main 0x02047650): record +70 must equal field+0x3EB5, then +71 is
        // the ground's zone under the player outdoors (ov001 0x020D5650) or the area's kind indoors. Phantasm's
        // records want zones 5 / 4 of a map with +0x3EB5 = 1: cast anywhere else (the switch skips the menu) its
        // scene runs where it was never meant to, and in town the camera's jump to the scene's place makes the
        // town's cell-row cache shift by more than its 32 rows (ov002 0x020FED08: a copy with a negative length),
        // which overwrites RAM and DTCM and ends the game. So the switch refuses Phantasm where the menu's place test
        // would (MQ_COOP_CAST_FORCE=1 casts anyway); for the others the test is only logged (their scenes stay where
        // the student is, and the earlier tests cast them anywhere).
        static const bool force = std::getenv("MQ_COOP_CAST_FORCE") != nullptr;
        const uint32_t field = ntr_rd32(c, kField);
        uint32_t allowed = 0;
        for (uint32_t i = 0; ram(table) && i < count && i < 64 && !allowed; i++) {
            const uint32_t r = table + i * kCharmSize;
            if (int16_t(ntr_rd16(c, r + 48)) == s->event && call(c, 0x02047650, r) != 0) allowed = r;
        }
        const bool refuse = !allowed && s->id == 34 && !force;
        std::fprintf(stderr, "coop magic: test cast of %s: the game's place test %s here (area %d, map %d)%s\n", s->name,
                     allowed ? "allows it" : "refuses it", ram(field) ? ntr_rd8(c, field + 0x3EB4) : -1,
                     ram(field) ? ntr_rd8(c, field + 0x3EB5) : -1, refuse ? ": not cast" : "");
        if (refuse) {
            st.cast_id = 0;
            return;
        }
        if (allowed) rec = allowed;
        set_flag(c, kSpellId, uint8_t(s->id));
        call(c, kCast, rec);
        std::fprintf(stderr, "coop magic: test cast of %s (%d, event %d, record %08x)\n", s->name, s->id, s->event, rec);
        st.cast_id = 0;
    } else if (!rec && (set_flag(c, kSpellId, uint8_t(s->id)), queue_event(c, s->event * 100 + 1))) {
        std::fprintf(stderr, "coop magic: test cast of %s (%d, event %d)\n", s->name, s->id, s->event);
        st.cast_id = 0;
    } else if (++st.cast_frames > 600) {
        std::fprintf(stderr, "coop magic: test cast of %s given up (not free)\n", s->name);
        st.cast_id = 0;
    }
}

// ------------------------------------------------------------------------------------------------ the session


// Free to be shown something: walking about in the field, no event, nothing loading.
bool free_now(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) && ram(ntr_rd32(c, kControlled)) && !event_busy(c) && ntr_rd8(c, field + kFieldLoading) == 0;
}

// The other student's object for a session slot, if it stands in this area (coop_talk.cpp's test).
bool remote_here(NtrCpu* c, int slot) {
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return false;
    const uint32_t remotes = ntr_rd32(c, field + kRemotePlayers);
    if (!ram(remotes)) return false;
    for (uint32_t i = 0; i < 3; i++) {
        const uint32_t o = remotes + i * kRemoteSize;
        if (int(ntr_rd8(c, o + kRemoteSlot)) == slot && ntr_rd32(c, o + 0x20) != 0 && ntr_rd8(c, o + 0x5A0) != 0) return true;
    }
    return false;
}

int area(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    return ram(field) ? ntr_rd8(c, field + kFieldArea) : -1;
}

void send(const std::vector<uint8_t>& m, uint32_t to = 0) { ntr::coop_send(kMsgType, m.data(), m.size(), to); }

uint32_t host_node() {
    uint32_t nodes[8];
    const size_t n = ntr::coop_peers(nodes, 8);
    return n ? nodes[0] : 0;
}

int name_index(const Spell* s) {
    for (int i = 0; i < int(std::size(content::kMagicNames)); i++)
        if (std::strcmp(content::kMagicNames[i], s->name) == 0) return i;
    return -1;
}

// A lifted incantation's event ended here: the words cleared as alone (query 0x16's own call, which the query skips
// in a session), the other console told, and a guest's writes of the host's variables sent to the host.
void cast_done(NtrCpu* c) {
    const Spell* s = st.lifted;
    st.lifted = nullptr;
    const uint32_t field = ntr_rd32(c, kField);
    if (ram(field)) call(c, kSpellEnd, field + kSpellWords);
    send({kOpCast, uint8_t(s->id), uint8_t(coop_sync::own_slot()), uint8_t(area(c))});
    std::string w;
    if (!coop_sync::hosting() && !st.writes.empty()) {
        std::vector<uint8_t> m = {kOpWrites, uint8_t(s->id), st.mys, st.stg, uint8_t(st.writes.size())};
        for (const Write& x : st.writes) {
            put16(m, x.index), m.push_back(x.value);
            w += " g" + std::to_string(1000 + x.index) + "=" + std::to_string(x.value);
        }
        send(m, host_node());
    }
    std::fprintf(stderr, "coop magic: %s cast here: the words cleared, the other console told%s%s\n", s->name,
                 w.empty() ? "" : "; to the host:", w.c_str());
    st.writes.clear();
}

// Host: a guest's incantation wrote the town's variables that are the host's (kHostVars: Golem's g1006, Familiar's
// g1310, Candyman's mystery stage): written here if the town is still at the mystery and stage the guest's cast saw.
void apply_writes(NtrCpu* c, const uint8_t* d, size_t len) {
    if (len < 5) return;
    const Spell* s = spell_by_id(d[1]);
    const uint8_t mys = d[2], stg = d[3], n = d[4];
    if (!s || len < 5u + 3u * n) return;
    const uint8_t now_mys = ntr_rd8(c, kStore), now_stg = ntr_rd8(c, kStore + 1);
    // (the stage the cast itself moves the town to counts as unchanged: the town sync may bring g1001 first)
    bool own_stage = false;
    for (int i = 0; i < n; i++)
        if (get16(d + 5 + 3 * i) == 1 && d[5 + 3 * i + 2] == now_stg) own_stage = true;
    if (now_mys != mys || (now_stg != stg && !own_stage)) {
        std::fprintf(stderr, "coop magic: the guest's %s not written: the town has moved on (mystery %d stage %d, now %d %d)\n",
                     s->name, mys, stg, now_mys, now_stg);
        return;
    }
    std::string w;
    for (int i = 0; i < n; i++) {
        const uint16_t index = uint16_t(get16(d + 5 + 3 * i));
        const uint8_t value = d[5 + 3 * i + 2];
        if (!coop_sync::host_var(1000u + index)) continue;
        call(c, kStoreWrite, kStore, index, value);
        w += " g" + std::to_string(1000 + index) + "=" + std::to_string(value);
    }
    call(c, kChecksum, kTown, kTownSize);
    std::fprintf(stderr, "coop magic: the guest's %s writes the town:%s\n", s->name, w.c_str());
}

// A guest's sky follows the host's weather at once. The sky is made at a field load from the town (above); the
// host's bytes that arrive afterwards (the town sync) changed nothing on screen until the next load. Once the week's
// table or the week's mystery (its sky of its own) differs from what this field's sky was made from, and the student
// has stood free for half a second, the sky is made again as the load makes it (no roll: the same date, the host's
// table). (The weather incantations need nothing: the game's own two-player path shows them on both consoles.)
void sky_inputs(NtrCpu* c, uint8_t* out) {
    for (uint32_t i = 0; i < kWeekWeatherSize; i++) out[i] = uint8_t(ntr_rd8(c, kTown + kWeekWeather + i));
    out[kWeekWeatherSize] = ntr_rd8(c, kStore) == 42 ? 1 : 0;  // (g1000: mystery 42's sky)
}

void sky_follow(NtrCpu* c) {
    static const bool off = std::getenv("MQ_COOP_SKY_GAME") != nullptr;  // (research: as the game has it)
    const uint32_t field = ntr_rd32(c, kField);
    if (off || coop_sync::hosting() || coop_sync::own_slot() < 0 || !ram(field)) return;
    if (ntr_rd8(c, field + kFieldLoading) != 0 || field != st.sky_field || area(c) != st.sky_area || !st.sky_known) {
        // (a field being loaded makes its sky from the town as it is then)
        sky_inputs(c, st.sky_from);
        st.sky_field = field, st.sky_area = area(c), st.sky_known = true, st.sky_free = 0;
        return;
    }
    if (!free_now(c)) return void(st.sky_free = 0);
    if (++st.sky_free < 30) return;
    uint8_t now[kWeekWeatherSize + 1];
    sky_inputs(c, now);
    if (std::memcmp(now, st.sky_from, sizeof now) == 0) return;
    const uint8_t before = ntr_rd8(c, 0x020AD2A5);  // (the kind shown)
    // (0x020CE81C, the scripts' entry, is not a function of the recompiled code by itself: its two calls here)
    call(c, kSkyMake, field, 0xFFFFFFFFu);
    const uint32_t vt = ntr_rd32(c, field), light = ram(vt) ? ntr_rd32(c, vt + kFieldLight) : 0;
    if (ram(light & ~1u)) {
        uint32_t saved[16];
        std::memcpy(saved, c->r, sizeof saved);
        c->r[0] = field;
        ntr_call(c, light);
        std::memcpy(c->r, saved, sizeof saved);
    }
    std::memcpy(st.sky_from, now, sizeof now);
    std::fprintf(stderr, "coop magic: the host's weather: the sky made again (kind %d -> %d)\n", before,
                 ntr_rd8(c, 0x020AD2A5));
}

void on_message(NtrCpu* c, uint32_t from, const uint8_t* d, size_t len) {
    if (len < 1) return;
    if (d[0] == kOpCast && len >= 4) {
        const Spell* s = spell_by_id(d[1]);
        if (!s) return;
        st.shows.push_back({s->id, d[2], d[3], st.frames + kShowWithin});
        std::fprintf(stderr, "coop magic: slot %d cast %s (area %d)\n", d[2], s->name, d[3]);
    } else if (d[0] == kOpWrites && coop_sync::hosting()) {
        apply_writes(c, d, len);
    } else if (d[0] == kOpWeather && len >= 21 && !coop_sync::hosting()) {
        for (uint32_t i = 0; i < 20; i++) ntr_wr8(c, kTown + kWeatherBytes + i, d[1 + i]), st.weather[i] = d[1 + i];
        call(c, kChecksum, kTown, kTownSize);
        st.weather_known = true;
        std::fprintf(stderr, "coop magic: the host's weather changed\n");
    }
    (void)from;
}

}  // namespace

// Every event command 0x38 (if-variable-goto), before it runs (from mq_coop_gate, ov028 0x02112E20; r0 the command).
void mq_magic_command(NtrCpu* c, uint32_t cmd) {
    if (log_on()) {
        const int file = event_file(c);
        if (file >= 310 && file <= 361)
            std::fprintf(stderr, "coop magic: event %d: if var %d op %d %d (t199 %d)\n", file, int32_t(ntr_rd32(c, cmd + 4)),
                         ntr_rd8(c, cmd + 2), int32_t(ntr_rd32(c, cmd + 8)), flag(c, kSpellId));
    }
    if (!in_session(c) || ntr_rd8(c, cmd) != 0x38) return;
    const Spell* s = nullptr;
    if (summon_refusal(c, cmd)) s = spell_by_event(event_file(c));
    else if (b5_refusal(c, cmd)) s = spell_by_id(flag(c, kSpellId));
    if (!s || (s->lift == kNone && !research_lift(s->id))) return;
    never_holds(c, cmd);
    st.lifted = s;
    st.lifted_file = event_file(c);
    st.mys = ntr_rd8(c, kStore), st.stg = ntr_rd8(c, kStore + 1);
    st.writes.clear();
    std::fprintf(stderr, "coop magic: %s (%d) lifted: cast as alone\n", s->name, s->id);
}

// Before the store write (main 0x0201E070; r0 the store, r1 the index, r2 the value): a guest's lifted incantation
// writing one of the host's variables (the town sync drops those: coop_sync.cpp, kHostVars), noted for the host.
void mq_magic_store(NtrCpu* c) {
    // (only the incantations that change the town: another's write of a host's variable is a side effect of its
    // scene, e.g. Sweet dreams' g1242 = 0, which must not reset the host's mystery; the town sync drops it)
    if (!st.lifted || st.lifted->lift != kHost || c->r[0] != kStore || coop_sync::hosting() || coop_sync::own_slot() < 0)
        return;
    if (c->r[1] >= 0x240 || !coop_sync::host_var(1000u + c->r[1])) return;
    for (Write& w : st.writes)
        if (w.index == c->r[1]) return void(w.value = uint8_t(c->r[2]));
    st.writes.push_back({uint16_t(c->r[1]), uint8_t(c->r[2])});
}

// The end of a frame (mq_frame_end).
void mq_magic_frame(NtrCpu* c) {
    cast_test(c);
    st.frames++;
    if (!st.listening) {
        coop_sync::listen(kMsgType, on_message);
        st.listening = true;
    }
    if (!in_session(c)) {
        st.lifted = nullptr, st.shows.clear(), st.writes.clear(), st.weather_known = false, st.sky_known = false;
        return;
    }
    sky_follow(c);
    // A lifted incantation's event has ended (its file no longer runs).
    if (st.lifted && (!event_busy(c) || event_file(c) != st.lifted_file) &&
        !(st.lifted_file == 350 && event_file(c) == 351))
        cast_done(c);
    // The other students' casts, shown once this student is free: in the same area, a sparkle at the caster.
    st.shows.erase(std::remove_if(st.shows.begin(), st.shows.end(), [&](const Shown& x) { return st.frames > x.until; }),
                   st.shows.end());
    if (!st.shows.empty() && free_now(c)) {
        const Shown x = st.shows.front();
        const Spell* s = spell_by_id(x.id);
        const int line = s ? name_index(s) : -1;
        const bool here = remote_here(c, x.slot);
        if (line >= 0) {
            set_flag(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + x.slot));
            set_flag(c, content::kTalkShow, uint8_t(content::kShowMagic + line));
            if (queue_event(c, here ? content::kEventMagicHere : content::kEventMagicAway)) {
                std::fprintf(stderr, "coop magic: shown: slot %d cast %s (%s)\n", x.slot, s->name,
                             here ? "here: the sparkle at them" : "elsewhere");
                st.shows.erase(st.shows.begin());
            }
        } else {
            st.shows.erase(st.shows.begin());
        }
    }
    // The weather: the host tells the guests at once when its town's weather bytes change (the town sync would bring
    // them within a second too). (These are the day's weather numbers the twice-a-day roll writes, read by the
    // bulletin board, ov016, and others; the sky's kind is not drawn from them: sky_follow above.)
    if (coop_sync::hosting()) {
        uint8_t now[20];
        for (uint32_t i = 0; i < 20; i++) now[i] = ntr_rd8(c, kTown + kWeatherBytes + i);
        if (st.weather_known && std::memcmp(now, st.weather, 20) != 0) {
            std::vector<uint8_t> m = {kOpWeather};
            m.insert(m.end(), now, now + 20);
            send(m);
            std::fprintf(stderr, "coop magic: the town's weather changed: told the guests\n");
        }
        std::memcpy(st.weather, now, 20);
        st.weather_known = true;
    }
}
