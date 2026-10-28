// Magician's Quest co-op: one town for everyone in a session (docs/COOP.md, "One town: the town kept in step").
//
// The game is not lockstep: each console runs its own student and its own copy of the host's town. At connect the
// host sends its whole town save block (0x993C bytes at 0x020AF020: the town's time, the global variable store, the
// ground-object grid, the classmates, shops) and every student's record (0x1858 bytes), once; afterwards each console
// writes only its own copies, so whatever a guest does to the town is lost when it leaves, and the host never sees it.
//
// Here the host's block is the truth, kept the same on every console over the co-op side channel (ntr/coop.hpp):
// - About once a second (60 frames) the host compares its block with what it last sent and sends the bytes that
//   changed; the guests write them into their copies at the end of a frame (mq_frame_end), and fix the block's
//   checksum as the game does after receiving it.
// - A guest compares its copy with what it last had from the host every frame. What its own student did to the
//   town (an event or the code writing a global variable, through main 0x0201E070, for the local student) goes to
//   the host as a request, which the host writes into its block: its next comparison sends it back to everyone.
//   Until the host has answered a request, the guest keeps its own value for those bytes (a script that reads back
//   what it wrote sees it). The guest's own *rolls* are not sent: they are made by the host too, and the host's are
//   the town's (docs/COOP.md): the classroom's picks of classmates (ov003, variables 236-239, 251, 252), the
//   weather (ov001 0x020CFBC0), and everything outside the variable store that the guest's game changes by itself
//   (relation drift on each scene load, ov001 0x020F5F08; the day's roll, ov027: buried items, weeds, ores, the
//   classmates' records). Those stay on the guest until the host's own values arrive.
// - Each console sends its own student's record (0x020AD824) when it changes, into the others' copies of it (the
//   session's entry for that slot, +0x18).
// Some bytes of the block belong to each console and are never sent: the checksum, the town time object (+0x2C8:
// the date and time it last saw, and the guest's offsets between its clock and the host's), and the classmates'
// values toward "player slot n" (+0x60..+0x6B of each classmate record), which on a guest mean the visiting
// student's slot in its own town, not the host's student with that number.
//
// Two lists of global variables are not shared the usual way (docs/COOP.md, "Variables each console keeps, and the
// host's alone"):
// - Each console's own (kOwnVars: the lesson picked, "lesson done today", the after-class sequence): the student's own
//   business kept in the town's store. A guest's writes are not sent, the host's are not taken; when a guest arrives
//   it gets back its own values from its own town (the game's transfer gave it the host's).
// - The host's alone (kHostVars: the week's mystery and its stages, the festivals' done flags, the transfer in
//   homeroom): a guest's writes are not sent, and once no event runs on the guest it sees the host's values again
//   (until scenes are run on both consoles together, a guest's scene cannot advance the host's story).
// The host ignores both in a guest's request too.
//
// Only in a co-op session, only once the console stands in the host's town (the field loaded in the session,
// field+0x3EAF), and only while the block still is the host's town (bytes +2..+7, the same on both). Outside a
// session nothing here runs (but the guest's own values of kOwnVars are noted, and the visit marker below).
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "coop_sync.hpp"

#include "cfie_game.hpp"
#include "ntr/coop.hpp"
#include "ntr/coop_net.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace {

// The game's addresses (USA, YNNE).
constexpr uint32_t kGame = 0x020ACA58;      // the game object (+0xA28: the wireless session, null when alone)
constexpr uint32_t kField = 0x020AD7EC;     // the town/field object
constexpr uint32_t kFieldInSession = 0x3EAF;  // field byte: this field was loaded in a session
constexpr uint32_t kTown = 0x020AF020, kTownSize = 0x993C;     // the town save block (main 0x02010ED8)
constexpr uint32_t kRecord = 0x020AD824, kRecordSize = 0x1858;  // the local student's record (main 0x02010ED0)
constexpr uint32_t kRecordSummed = 0x17E4;  // the part of a record its checksum covers (ov001 0x020EE364)
constexpr uint32_t kChecksum = 0x02011D9C;  // (block, size): the block's u16 checksum, written at +0
constexpr uint32_t kSessionPlayers = 0x49E8, kSessionPlayerSize = 0x50;  // +0x18: the slot's student record
// Inside the town block.
constexpr uint32_t kTownId = 2, kTownIdSize = 6;  // the same on every console of a session (the host's town)
constexpr uint32_t kTownTime = 0x2C8, kTownTimeSize = 0x10;  // date, time, the guest's day and second offsets
constexpr uint32_t kClassmates = 0x6468, kClassmateSize = 0xA4, kClassmateCount = 13;
constexpr uint32_t kTowardPlayers = 0x60, kTowardPlayersSize = 0xC;  // stage, friendship, affection x 4 slots
// The global variable store (main 0x0201D9F8: variables 1000-9999 at index - 1000, 30000+ at index - 0x72F0).
constexpr uint32_t kStore = 0x2D8, kStoreSize = 0x160;
// Who wrote a store variable (the store write's return address, or the setter's when it went through the setter):
// rolls the host makes too. The setter's own calls of the store write return to these.
constexpr uint32_t kSetterReturns[] = {0x0201DA1A, 0x0201DA38, 0x0201DA6A, 0x0201DA80};
constexpr uint32_t kClassroomRolls[] = {0x021061B2, 0x021061E6, 0x021066DE};  // ov003: picks, vars 251/252, 236-239
constexpr uint32_t kWeather = 0x020CFBC0, kWeatherEnd = 0x020CFE30;          // ov001: the day's weather variables
// ov001: the sky's kind as a field load (or a script) sets it (0x020D8CCC: g1517 and store index 301) and the
// weather incantations' adjustment of it (0x020D8D48): worked out from the date, the town's weekly weather table and
// the incantations in effect, the same on every console; never a guest's action.
constexpr uint32_t kSkyKind = 0x020D8CCC, kSkyKindEnd = 0x020D8DCC;
constexpr uint32_t kMushroomDay = 0x020FFC4A;  // ov002: the town's mark of the day its mushrooms were placed (var 198)

constexpr int kPeriod = 60;        // frames between the host's comparisons (and each console's record's)
constexpr int kQuietFrames = 120;  // after a guest's own day change: its changes are its own day's rolls, not sent
constexpr size_t kMaxRuns = ntr::kCoopMaxMessage - 256;

enum Message : uint8_t {
    kHello = 1,       // u8 slot, u8 hosting: this console is in the host's town, in that slot
    kTownBytes = 2,   // host: u8 whole, town id, u8 n, n x (u32 node, u32 request), runs
    kRequest = 3,     // guest: u32 request, town id, runs (the guest's own changes to the town)
    kRecordBytes = 4, // u8 slot, runs (that student's record)
    kResend = 5,      // guest: the whole town again, please
};
// Runs: u16 offset, u16 length, the bytes; to the end of the message.

// Global variables (1000-1575: store index variable - 1000) each console keeps for itself: the lesson picked from
// Principal Sol's menu (1017, ov038 0x0211C2B2; the classroom's start records match it), "lesson done today" (1015, the
// lesson's end; it starts the after-class scene), and the after-class scene's sequence (event 010 block 10: 1255 the
// step, 1253, 1254, 1257, 1259 what the next lessons branch on, set with the student's s227, s228, s230).
constexpr uint16_t kOwnVars[] = {1015, 1017, 1253, 1254, 1255, 1257, 1259};
// Global variables only the host's console advances (a guest's scene would advance or finish the host's story without
// the host seeing it): the week's mystery (1000 its number, 1001 its stage) and the fields a new mystery clears (ov001
// 0x020CE89C: 1002-1007, 1009-1012, 1070, 1242), those every mystery's scripts move (1020, 1054, 1221, 1258, 1310-1321)
// and those only mysteries' scripts write (events 071-122); the school festivals' done flags (1387-1390; 1242 is also
// the week's event); the transfer of a classmate announced in homeroom (1061: 1 / 2, 11 / 12, 21 / 22).
constexpr uint16_t kHostVars[] = {
    1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1009, 1010, 1011, 1012, 1020, 1021, 1022, 1023, 1024, 1025,
    1034, 1036, 1037, 1039, 1042, 1043, 1054, 1056, 1059, 1060, 1061, 1070, 1073, 1180, 1181, 1182, 1183, 1184,
    1185, 1186, 1187, 1188, 1189, 1190, 1191, 1195, 1196, 1197, 1200, 1220, 1221, 1242, 1258, 1281, 1283, 1290,
    1307, 1310, 1311, 1312, 1313, 1314, 1315, 1316, 1317, 1318, 1319, 1320, 1321, 1322, 1324, 1325, 1387, 1388,
    1389, 1390, 1391, 1393, 1460, 1485, 1486, 1487, 1488, 1489, 1490, 1491, 1492, 1493, 1494, 1495, 1496, 1497,
    1498, 1499, 1575};

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0, uint32_t a1 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0, c->r[1] = a1;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

void read_ram(NtrCpu* c, uint32_t a, uint8_t* out, uint32_t n) {
    while (n) {
        const uint32_t in_page = std::min<uint32_t>(n, NTR_PAGE_MASK + 1 - (a & NTR_PAGE_MASK));
        if (const uint8_t* p = c->read_pages[a >> NTR_PAGE_SHIFT]) {
            std::memcpy(out, p + (a & NTR_PAGE_MASK), in_page);
        } else {
            for (uint32_t i = 0; i < in_page; i++) out[i] = uint8_t(ntr_rd8(c, a + i));
        }
        a += in_page, out += in_page, n -= in_page;
    }
}

// Bytes of the town block that each console keeps for itself.
const std::vector<uint8_t>& town_own() {
    static const std::vector<uint8_t> own = [] {
        std::vector<uint8_t> v(kTownSize, 0);
        v[0] = v[1] = 1;  // the checksum
        for (uint32_t i = 0; i < kTownTimeSize; i++) v[kTownTime + i] = 1;
        for (uint32_t k = 0; k < kClassmateCount; k++)
            for (uint32_t i = 0; i < kTowardPlayersSize; i++) v[kClassmates + k * kClassmateSize + kTowardPlayers + i] = 1;
        return v;
    }();
    return own;
}
// The bits of the town block a global variable (1000-1575) lives in, as the store write (main 0x0201E070) puts it:
// index < 0x100 a byte at +index, < 0x200 a bit (index & 7) at +0x140 + (index - 0x100) / 8, < 0x240 a byte at
// +(index - 0x100).
void var_bits(uint32_t var, std::vector<uint8_t>& mask) {
    const uint32_t i = var - 1000;
    if (i < 0x100) mask[kStore + i] = 0xFF;
    else if (i < 0x200) mask[kStore + 0x140 + ((i - 0x100) >> 3)] |= uint8_t(1u << ((i - 0x100) & 7));
    else if (i < 0x240) mask[kStore + i - 0x100] = 0xFF;
}
// Per byte of the town block, the bits of kOwnVars and of kHostVars.
const std::vector<uint8_t>& own_vars() {
    static const std::vector<uint8_t> m = [] {
        std::vector<uint8_t> v(kTownSize, 0);
        for (uint16_t var : kOwnVars) var_bits(var, v);
        return v;
    }();
    return m;
}
const std::vector<uint8_t>& host_vars() {
    static const std::vector<uint8_t> m = [] {
        std::vector<uint8_t> v(kTownSize, 0);
        for (uint16_t var : kHostVars) var_bits(var, v);
        return v;
    }();
    return m;
}
// The offsets of the bytes either list touches (a short list: the store's 0x160 bytes at most).
const std::vector<uint32_t>& kept_vars_at() {
    static const std::vector<uint32_t> at = [] {
        std::vector<uint32_t> v;
        for (uint32_t i = kStore; i < kStore + kStoreSize; i++)
            if (own_vars()[i] | host_vars()[i]) v.push_back(i);
        return v;
    }();
    return at;
}

// ... and of a student's record: its checksum and what the checksum does not cover.
const std::vector<uint8_t>& record_own() {
    static const std::vector<uint8_t> own = [] {
        std::vector<uint8_t> v(kRecordSize, 0);
        v[0] = v[1] = 1;
        for (uint32_t i = kRecordSummed; i < kRecordSize; i++) v[i] = 1;
        return v;
    }();
    return own;
}

void put16(std::vector<uint8_t>& m, uint32_t v) { m.push_back(uint8_t(v)), m.push_back(uint8_t(v >> 8)); }
void put32(std::vector<uint8_t>& m, uint32_t v) { put16(m, v & 0xFFFF), put16(m, v >> 16); }
uint32_t get16(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8; }
uint32_t get32(const uint8_t* p) { return get16(p) | get16(p + 2) << 16; }

// The bytes where now differs from base (or all of them: whole), leaving out a console's own, as runs appended to m;
// base takes them. Returns how many bytes.
uint32_t diff(const std::vector<uint8_t>& now, std::vector<uint8_t>& base, const std::vector<uint8_t>& own, bool whole,
              std::vector<uint8_t>& m) {
    uint32_t bytes = 0;
    const uint32_t n = uint32_t(now.size());
    for (uint32_t i = 0; i < n;) {
        if (own[i] || (!whole && now[i] == base[i])) {
            i++;
            continue;
        }
        uint32_t j = i;
        while (j < n && !own[j] && (whole || now[j] != base[j])) j++;
        put16(m, i), put16(m, j - i);
        m.insert(m.end(), now.begin() + i, now.begin() + j);
        std::copy(now.begin() + i, now.begin() + j, base.begin() + i);
        bytes += j - i;
        i = j;
    }
    return bytes;
}

// Calls fn(offset, byte) for each byte of the runs in [p, end) that lies within size.
template <typename F>
void each_byte(const uint8_t* p, const uint8_t* end, uint32_t size, F fn) {
    while (end - p >= 4) {
        const uint32_t at = get16(p), n = get16(p + 2);
        p += 4;
        if (uint32_t(end - p) < n) return;
        for (uint32_t i = 0; i < n && at + i < size; i++) fn(at + i, p[i]);
        p += n;
    }
}

std::string offsets(const std::vector<uint8_t>& runs, size_t from) {
    std::string s;
    int k = 0;
    for (size_t p = from; p + 4 <= runs.size() && k < 6; k++) {
        const uint32_t at = get16(&runs[p]), n = get16(&runs[p + 2]);
        char b[32];
        std::snprintf(b, sizeof b, "%s+%x:%u", k ? " " : "", at, n);
        s += b;
        p += 4 + n;
    }
    return s;
}

struct Peer {
    int slot = -1;
    bool greeted = false;   // our hello went to it
    uint32_t applied = 0;   // host: the last of its requests written into the block
};

struct State {
    uint32_t session = 0;
    bool active = false;
    int own = -1;
    bool hosting = false;
    uint8_t town_id[kTownIdSize] = {};
    std::vector<uint8_t> town, record;  // what the others have of them (host, every console) / had from the host
    std::vector<uint32_t> pending;       // guest: per byte of the town, the request that last changed it
    uint32_t requests = 0, answered = 0; // guest: requests sent, and the last the host has written
    uint32_t host_node = 0;
    std::map<uint32_t, Peer> peers;
    std::map<uint32_t, uint32_t> acks_sent;  // host: the request of each guest its last town bytes answered
    int frames = 0;
    uint32_t date = 0;
    int quiet = 0;
    std::vector<uint8_t> acted;  // guest: town bytes its student's own actions wrote since the last comparison
    uint32_t setter_lr = 0;      // the variable setter's caller, for the store write it makes next
    std::map<uint32_t, uint8_t> noted;  // guest: the kept variables' bits last logged, per byte
} st;

// A guest's own town's values of kOwnVars (its bytes at kept_vars_at()), noted while it is alone, given back when it
// arrives in the host's town (the game's transfer gave it the host's).
struct Home {
    bool valid = false;
    std::vector<uint8_t> bytes;
} home;

constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context
// An event runs (the script context's +8) or is being started (+0x23BC), as coop_talk.cpp tests it.
bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0);
}

// The variables of a list that live in the given bits of the town block's byte at, with their values in v.
std::string vars_in(uint32_t at, uint8_t bits, const std::vector<uint8_t>& v) {
    std::string s;
    auto one = [&](const uint16_t* b, const uint16_t* e) {
        std::vector<uint8_t> m(kTownSize, 0);
        for (; b != e; ++b) {
            std::fill(m.begin() + kStore, m.begin() + kStore + kStoreSize, 0);
            var_bits(*b, m);
            if (!(m[at] & bits)) continue;
            const uint32_t value = m[at] == 0xFF ? v[at] : (v[at] & m[at]) != 0;
            s += " g" + std::to_string(*b) + "=" + std::to_string(value);
        }
    };
    one(std::begin(kOwnVars), std::end(kOwnVars));
    one(std::begin(kHostVars), std::end(kHostVars));
    return s;
}

// A guest, before its changes are compared with the host's: the bits of kOwnVars and kHostVars in now take the host's
// last values (so they are never sent); in RAM, kHostVars' bits go back to the host's once no event runs.
void keep_vars(NtrCpu* c, std::vector<uint8_t>& now) {
    const auto &mine = own_vars(), &hosts = host_vars();
    bool reverted = false;
    std::string back;
    for (uint32_t at : kept_vars_at()) {
        const uint8_t m = mine[at] | hosts[at];
        const uint8_t local = now[at] & m;
        if (auto it = st.noted.find(at); it == st.noted.end() || it->second != local) {
            // (a change of the host's values that came from the host is not this console's)
            const uint8_t changed = it == st.noted.end() ? 0 : uint8_t((it->second ^ local) & (mine[at] | (now[at] ^ st.town[at])));
            if (changed)
                std::fprintf(stderr, "coop sync: this console wrote%s: %s, not sent\n", vars_in(at, changed, now).c_str(),
                             (changed & mine[at]) ? "each console's own" : "the host's alone");
            st.noted[at] = local;
        }
        if (!((now[at] ^ st.town[at]) & m)) continue;
        if ((now[at] ^ st.town[at]) & hosts[at] && !event_busy(c)) {
            const uint8_t v = uint8_t((now[at] & ~hosts[at]) | (st.town[at] & hosts[at]));
            back += vars_in(at, uint8_t((now[at] ^ st.town[at]) & hosts[at]), st.town);
            ntr_wr8(c, kTown + at, v);
            st.noted[at] = uint8_t(v & m);
            reverted = true;
        }
        now[at] = uint8_t((now[at] & ~m) | (st.town[at] & m));
    }
    if (reverted) {
        call(c, kChecksum, kTown, kTownSize);
        std::fprintf(stderr, "coop sync: the host's values back (no event runs):%s\n", back.c_str());
    }
}

// A guest, before its changes are compared with the host's: a classmate's needs (+0x48..+0x4F of its record, eight
// values the game moves when the classmate starts a pastime: the director's round of its people, ov001 0x020EBD4C ->
// 0x020EBD80 -> 0x020EBE68, and a job's start, 0x020EBFC0, through main 0x0202F388; read by the classmates' talk
// rules, ov001 0x020F60A0, and the field's set-up of its people, main 0x02043408) and +0x9F (bits the same writes set)
// are the town's as the console that decides that classmate moves them (game/src/coop_npc.cpp; docs/COOP.md "The
// classmates' needs"). A change this guest's game made to them is sent to the host (as its student's own) when this
// console decides that classmate or a scene runs here (a talk, a gift: the student's doing); when the host or another
// guest decides it, it is the copy's start of what the owner already did (at a field's load a guest decides its people
// until the host's word comes, ov001 0x020E6220), so the host's values go back at once; not known yet (just after a
// field's load), it waits, up to kNeedsWait frames, then the host's go back. (Kept here until the host's next change,
// they differed for minutes, docs/COOP_PLAYTEST.md "Soak".)
constexpr uint32_t kNeeds = 0x48, kNeedsEnd = 0x50, kNeedsFlags = 0x9F, kClassmateId = 0x96;
constexpr int kNeedsWait = 300;
void classmate_needs(NtrCpu* c, std::vector<uint8_t>& now, bool busy) {
    static int waiting = 0;
    static const bool log = std::getenv("MQ_COOP_NEEDS_LOG") != nullptr;
    bool any = false, held = false, back = false;
    std::string sent, given;
    for (uint32_t k = 0; k < kClassmateCount; k++) {
        const uint32_t r = kClassmates + k * kClassmateSize;
        bool changed = false;
        for (uint32_t o = kNeeds; o <= kNeedsFlags; o = o + 1 == kNeedsEnd ? kNeedsFlags : o + 1)
            if (now[r + o] != st.town[r + o] && !st.acted[r + o] && st.pending[r + o] <= st.answered) changed = true;
        if (!changed) continue;
        any = true;
        const uint8_t id = now[r + kClassmateId];
        const int decides = busy ? 1 : mq_npc_decides(c, id);
        if (decides < 0 && waiting < kNeedsWait) {
            held = true;
            for (uint32_t o = kNeeds; o <= kNeedsFlags; o = o + 1 == kNeedsEnd ? kNeedsFlags : o + 1)
                now[r + o] = st.town[r + o];  // (not sent, not kept: decided once the host's word comes)
            continue;
        }
        char b[24];
        std::snprintf(b, sizeof b, " %u (id %u)", k, id);
        for (uint32_t o = kNeeds; o <= kNeedsFlags; o = o + 1 == kNeedsEnd ? kNeedsFlags : o + 1) {
            const uint32_t at = r + o;
            if (now[at] == st.town[at] || st.acted[at] || st.pending[at] > st.answered) continue;
            if (decides > 0) {
                st.acted[at] = 1;  // (this console's doing: sent to the host as a request)
            } else {
                ntr_wr8(c, kTown + at, st.town[at]);
                now[at] = st.town[at];
                back = true;
            }
        }
        (decides > 0 ? sent : given) += b;
    }
    waiting = held ? waiting + 1 : 0;
    if (back) call(c, kChecksum, kTown, kTownSize);
    if (log && held && waiting == 1) std::fprintf(stderr, "coop sync: classmates' needs moved here, held until the host's word\n");
    if (any && (!sent.empty() || !given.empty()))
        std::fprintf(stderr, "coop sync: classmates' needs this console's game moved:%s%s%s%s\n",
                     sent.empty() ? "" : " sent to the host (this console decides them, or a scene ran):",
                     sent.c_str(), given.empty() ? "" : "; the host's values back (another console decides them):",
                     given.c_str());
}

std::map<uint8_t, coop_sync::Handler>& listeners() {
    static std::map<uint8_t, coop_sync::Handler> l;
    return l;
}

// The byte of the store that the store write (main 0x0201E070) changes for an index, or -1 (as its code: bytes,
// then bits from +0x140, bytes from +0x100, bits from +0).
int store_byte(uint32_t i) {
    if (i < 0x100) return int(i);
    if (i < 0x200) return int(0x140 + ((i - 0x100) >> 3));
    if (i < 0x240) return int(i - 0x100);
    if (i < 0xA40) return int((i - 0x240) >> 3);
    return -1;
}

uint32_t session(NtrCpu* c) {
    const uint32_t game = ntr_rd32(c, kGame);
    return ram(game) ? ntr_rd32(c, game + 0xA28) : 0;
}

// This console's slot in the session (the session's own answer, its vtable +0x30, as coop_talk.cpp asks it).
int own_slot(NtrCpu* c, uint32_t s) {
    const uint32_t vt = ntr_rd32(c, s);
    if (!ram(vt)) return -1;
    return int(call(c, ntr_rd32(c, vt + 0x30), s) & 0xFF);
}

std::vector<uint8_t> read_town(NtrCpu* c) {
    std::vector<uint8_t> v(kTownSize);
    read_ram(c, kTown, v.data(), kTownSize);
    return v;
}

bool same_town(NtrCpu* c, const uint8_t* id) {
    for (uint32_t i = 0; i < kTownIdSize; i++)
        if (ntr_rd8(c, kTown + kTownId + i) != id[i]) return false;
    return true;
}

void send_town(NtrCpu* c, uint32_t to, bool whole) {
    std::vector<uint8_t> m;
    m.push_back(whole);
    for (uint32_t i = 0; i < kTownIdSize; i++) m.push_back(uint8_t(ntr_rd8(c, kTown + kTownId + i)));
    m.push_back(0);
    uint8_t n = 0;
    for (auto& [node, p] : st.peers)
        if (p.applied) put32(m, node), put32(m, p.applied), n++;
    m[1 + kTownIdSize] = n;
    const size_t runs_at = m.size();
    const std::vector<uint8_t> now = read_town(c);
    // (a whole block for one guest leaves the others' comparison alone)
    std::vector<uint8_t> scratch = st.town;
    const uint32_t bytes = diff(now, whole ? scratch : st.town, town_own(), whole, m);
    if (!whole && m.size() > kMaxRuns) {  // (too scattered for runs: the whole block instead)
        m.resize(runs_at);
        m[0] = 1;
        diff(now, st.town, town_own(), true, m);
    }
    bool news = bytes != 0;
    for (auto& [node, p] : st.peers)
        if (p.applied && st.acks_sent[node] != p.applied) news = true;
    if (!whole && !news) return;
    if (to == 0)
        for (auto& [node, p] : st.peers) st.acks_sent[node] = p.applied;
    ntr::coop_send(kTownBytes, m.data(), m.size(), to);
    if (bytes) std::fprintf(stderr, "coop sync: town -> %s: %u bytes (%s)\n", to ? "a guest" : "guests", bytes,
                            whole ? "whole" : offsets(m, runs_at).c_str());
}

void send_record(NtrCpu* c, uint32_t to, bool whole) {
    std::vector<uint8_t> now(kRecordSize);
    read_ram(c, kRecord, now.data(), kRecordSize);
    std::vector<uint8_t> m;
    m.push_back(uint8_t(st.own));
    std::vector<uint8_t> scratch = st.record;
    const uint32_t bytes = diff(now, whole ? scratch : st.record, record_own(), whole, m);
    if (!bytes) return;
    ntr::coop_send(kRecordBytes, m.data(), m.size(), to);
    if (!whole) std::fprintf(stderr, "coop sync: own record -> all: %u bytes (%s)\n", bytes, offsets(m, 1).c_str());
}

void stop(const char* why) {
    if (st.active) std::fprintf(stderr, "coop sync: stopped (%s)\n", why);
    st = State{};
}

void received(uint32_t from, uint8_t type, const uint8_t* d, size_t len, void* user) {
    NtrCpu* c = static_cast<NtrCpu*>(user);
    const uint8_t* end = d + len;
    Peer& peer = st.peers[from];
    switch (type) {
    case kHello: {
        if (len < 2) return;
        peer.slot = d[0];
        peer.applied = 0;
        st.acks_sent.erase(from);
        if (d[1]) st.host_node = from;
        std::fprintf(stderr, "coop sync: node %08x is slot %d%s\n", from, peer.slot, d[1] ? " (hosting)" : "");
        send_record(c, from, true);
        if (st.hosting) send_town(c, from, true);
        return;
    }
    case kResend:
        if (st.hosting) send_town(c, from, true);
        return;
    case kTownBytes: {
        if (st.hosting || len < 2 + kTownIdSize) return;
        st.host_node = from;
        if (!same_town(c, d + 1)) {
            std::fprintf(stderr, "coop sync: town bytes for another town, left alone\n");
            return;
        }
        const uint8_t n = d[1 + kTownIdSize];
        const uint8_t* p = d + 2 + kTownIdSize;
        if (end - p < 8 * n) return;
        const uint32_t me = ntr::coop_node();
        for (int i = 0; i < n; i++, p += 8)
            if (get32(p) == me) st.answered = std::max(st.answered, get32(p + 4));
        uint32_t written = 0, kept = 0;
        std::string where;
        const auto& own = town_own();
        const auto& mine = own_vars();
        const bool busy = event_busy(c);
        each_byte(p, end, kTownSize, [&](uint32_t at, uint8_t v) {
            if (own[at]) return;
            if (st.pending[at] > st.answered) {  // (our own change, not yet written by the host)
                kept += st.town[at] != v;
                return;
            }
            st.town[at] = v;
            if (mq_safedep_hold(c, at, v)) return;  // (a guest's own safe deposit open here: the host's put aside)
            // (each console's own; and while an event runs here, the host's alone as its script wrote them: a scene
            // that reads back its own stage write sees it, and the host's values are back once it has ended)
            if (const uint8_t keep = uint8_t(mine[at] | (busy ? host_vars()[at] : 0)))
                v = uint8_t((v & ~keep) | (ntr_rd8(c, kTown + at) & keep));
            if (ntr_rd8(c, kTown + at) == v) return;
            ntr_wr8(c, kTown + at, v);
            if (written++ < 8) {
                char b[16];
                std::snprintf(b, sizeof b, " +%x=%u", at, v);
                where += b;
            }
        });
        if (written) call(c, kChecksum, kTown, kTownSize);
        if (written || kept)
            std::fprintf(stderr, "coop sync: town from host%s: %u bytes changed:%s%s\n", d[0] ? " (whole block)" : "",
                         written, where.c_str(),
                         kept ? (", " + std::to_string(kept) + " of ours kept until answered").c_str() : "");
        return;
    }
    case kRequest: {
        if (!st.hosting || len < 4 + kTownIdSize) return;
        if (!same_town(c, d + 4)) {
            std::fprintf(stderr, "coop sync: a request for another town, left alone\n");
            return;
        }
        uint32_t written = 0, refused = 0;
        const auto& own = town_own();
        const auto &mine = own_vars(), &hosts = host_vars();
        each_byte(d + 4 + kTownIdSize, end, kTownSize, [&](uint32_t at, uint8_t v) {
            if (own[at]) return;
            // (each console's own and the host's alone: the guest's bits are not taken)
            if (const uint8_t keep = mine[at] | hosts[at]) {
                const uint8_t cur = uint8_t(ntr_rd8(c, kTown + at)), nv = uint8_t((v & ~keep) | (cur & keep));
                refused += (v & keep) != (cur & keep);
                v = nv;
            }
            // (sent to everyone at the next comparison, whatever it is then: if the host's game changes the byte again
            // before that (g1040 after a talk, docs/COOP_PLAYTEST.md "Soak"), the guest that asked would otherwise
            // keep its own value, as the comparison saw no change against what it had last sent)
            if (at < st.town.size()) st.town[at] = uint8_t(~v);
            if (ntr_rd8(c, kTown + at) == v) return;
            ntr_wr8(c, kTown + at, v);
            written++;
        });
        if (refused) std::fprintf(stderr, "coop sync: request from slot %d: %u bytes of variables kept per console or the host's, not taken\n", peer.slot, refused);
        peer.applied = get32(d);
        std::fprintf(stderr, "coop sync: request %u from slot %d: %u bytes written (%s)\n", peer.applied, peer.slot,
                     written, offsets(std::vector<uint8_t>(d + 4 + kTownIdSize, end), 0).c_str());
        return;
    }
    case kRecordBytes: {
        if (len < 1) return;
        const int slot = d[0];
        const uint32_t s = session(c);
        if (slot < 0 || slot > 3 || slot == st.own || !ram(s)) return;
        const uint32_t copy = ntr_rd32(c, s + kSessionPlayers + uint32_t(slot) * kSessionPlayerSize + 0x18);
        if (!ram(copy) || copy == kRecord) return;
        uint32_t written = 0;
        const auto& own = record_own();
        each_byte(d + 1, end, kRecordSize, [&](uint32_t at, uint8_t v) {
            if (own[at] || ntr_rd8(c, copy + at) == v) return;
            ntr_wr8(c, copy + at, v);
            written++;
        });
        if (written) {
            call(c, kChecksum, copy, kRecordSummed);
            std::fprintf(stderr, "coop sync: record of slot %d (%08x): %u bytes changed\n", slot, copy, written);
        }
        return;
    }
    default:
        if (auto it = listeners().find(type); it != listeners().end() && it->second) it->second(c, from, d, len);
        return;
    }
}

// (test driver, MQ_COOP_POKE_FILE=<path>: when <path> appears, holding lines "<hex address>=<value>[/<bytes>]", they
// are written into RAM at the end of that frame and the file is removed: a change to the town made on purpose;
// "var <n>=<value>" sets script variable n through the game's setter, main 0x0201D9F8, as an event would)
void poke_file(NtrCpu* c) {
    static const char* path = std::getenv("MQ_COOP_POKE_FILE");
    if (!path) return;
    FILE* f = std::fopen(path, "rb");
    if (!f) return;
    unsigned addr = 0, value = 0, bytes = 1;
    char line[128];
    while (std::fgets(line, sizeof line, f)) {
        bytes = 1;
        if (unsigned var = 0; std::sscanf(line, "var %u=%u", &var, &value) == 2) {
            const uint32_t ctx = ntr_rd32(c, 0x020B8AC0);  // (the event script context)
            uint32_t saved[16];
            std::memcpy(saved, c->r, sizeof saved);
            c->r[0] = ctx, c->r[1] = var, c->r[2] = value, c->r[3] = 0xFFFFFFFF;
            ntr_call(c, 0x0201D9F8 | 1);
            std::memcpy(c->r, saved, sizeof saved);
            std::fprintf(stderr, "poke: variable %u = %u\n", var, value);
            continue;
        }
        if (std::sscanf(line, "%x=%u/%u", &addr, &value, &bytes) < 2) continue;
        if (bytes == 4) ntr_wr32(c, addr, value);
        else if (bytes == 2) ntr_wr16(c, addr, uint16_t(value));
        else ntr_wr8(c, addr, uint8_t(value));
        std::fprintf(stderr, "poke: %08x = %u (%u bytes)\n", addr, value, bytes);
    }
    std::fclose(f);
    std::remove(path);
}

// A visit never counts as turning off the power without saving (docs/COOP.md, "Turning off without saving").
//
// The game punishes a student whose game was turned off without saving (the warden's toilet cleaning, events 801-804
// on maps 68-69, while the student's s500 is 1). How it knows: loading a game at the title (ov000 0x020C7ACC) sets
// s500 from the slot's bit of the save's header byte 0x020AD821 (main 0x020106A8) and then sets that bit and saves
// (main 0x02010670), so the bit stays set in the save while the game is played; only the Gateway's Save, which ends at
// the title, and the visit's start clear it (seen: the header byte 0 in a session; a game stopped while connected
// loaded with s500 = 0). The visit's end sets it again (ov001 0x020F2874, ov023 0x02110CC8, ov024 0x021104BC) and
// saves, also after "Unable to receive data" (seen: that save loaded with the cleaning). So a game closed after a
// visit ended and before it was saved is punished at its next load: one closed during the end's or an error's
// messages, or in the new day's wait at home before connecting again (coop_session.cpp), or one that crashed then.
// So while a visit is under way a marker file beside the save (<save>.visit) says so; it goes once the student has
// stood free to move at home (no session) for 10 seconds; the day's own end and new connection keep it. At a load
// that finds the marker the punishment is not given (s500 written 0 instead of 1) and the marker goes. A game turned
// off without saving outside a visit is punished as the game has it.
constexpr uint32_t kLoadPunish = 0x020C7B18;  // ov000: the load's store write of s500 (returns here)
constexpr uint32_t kPunishVar = 500;
constexpr uint32_t kControlled = 0x020FE264;  // ov001: the actor the player controls (null outside the field)
constexpr int kVisitOverFrames = 600;
struct Visit {
    bool checked = false;  // the marker's file looked for (a visit of an earlier run)
    bool marked = false;   // the marker is there
    int free = 0;          // frames free to move at home since the visit
} visit;

std::string visit_path() {
    const std::string& pairs = cfie::coop_pairs_path;  // (<save>.coop; empty without a save that can be written)
    if (pairs.empty()) return {};
    const size_t dot = pairs.rfind('.');
    return (dot == std::string::npos ? pairs : pairs.substr(0, dot)) + ".visit";
}

void visit_check() {
    if (visit.checked) return;
    const std::string path = visit_path();
    if (path.empty()) return;
    visit.checked = true;
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
        std::fclose(f);
        visit.marked = true;
        std::fprintf(stderr, "coop visit: the last run ended during a co-op visit (%s)\n", path.c_str());
    }
}

void visit_frame(NtrCpu* c) {
    visit_check();
    const std::string path = visit_path();
    if (!visit.checked || path.empty()) return;
    if (ram(session(c)) && cfie::coop_anytime) {
        visit.free = 0;
        if (visit.marked) return;
        if (FILE* f = std::fopen(path.c_str(), "wb")) {
            std::fputs("A co-op visit is under way (Magician's Quest recomp): turning off now is not punished.\n", f);
            std::fclose(f);
            visit.marked = true;
            std::fprintf(stderr, "coop visit: under way (%s)\n", path.c_str());
        }
        return;
    }
    if (!visit.marked) return;
    const uint32_t field = ntr_rd32(c, kField), ctx = ntr_rd32(c, kScriptContext);
    const bool free = ram(field) && ram(ctx) && ntr_rd8(c, field + 0x3EAE) == 0 && ram(ntr_rd32(c, kControlled)) &&
                      ntr_rd8(c, ctx + 8) == 0 && ntr_rd8(c, ctx + 0x23BC) == 0;
    visit.free = free ? visit.free + 1 : 0;
    if (visit.free < kVisitOverFrames) return;
    std::remove(path.c_str());
    visit.marked = false;
    std::fprintf(stderr, "coop visit: over, the student free at home (turning off without saving counts again)\n");
}

}  // namespace

namespace coop_sync {
void listen(uint8_t type, Handler handler) { listeners()[type] = handler; }
int own_slot() { return st.active ? st.own : -1; }
bool hosting() { return st.active && st.hosting; }
bool host_var(uint32_t var) { return std::find(std::begin(kHostVars), std::end(kHostVars), var) != std::end(kHostVars); }
uint8_t host_var_bits(uint32_t store_byte) { return store_byte < kStoreSize ? host_vars()[kStore + store_byte] : 0; }
}  // namespace coop_sync

// Before the variable setter (main 0x0201D9F8, r0 context, r1 variable, r2 value): who asked, for the store write
// it makes.
void mq_coop_sync_set(NtrCpu* c) {
    if (st.active && !st.hosting) st.setter_lr = c->r[14] & ~1u;
}

bool mq_in_remote_act();  // (game/src/coop_acts.cpp: the partner's student copy's act is running)

// Before a variable store's write (main 0x0201E070, r0 the store, r1 the index, r2 the value): a guest notes the
// global store's bytes that its own student's actions write, which are the only ones it sends to the host.
void mq_coop_sync_store(NtrCpu* c) {
    // The load of a game at the title giving the punishment for turning off without saving (see visit_frame).
    if ((c->r[14] & ~1u) == kLoadPunish && c->r[1] == kPunishVar && c->r[2] != 0) {
        visit_check();
        if (visit.marked) {
            c->r[2] = 0;
            std::remove(visit_path().c_str());
            visit.marked = false;
            std::fprintf(stderr, "coop visit: the game was turned off during a co-op visit: not punished\n");
        } else {
            std::fprintf(stderr, "coop visit: the game was turned off without saving (not in a visit): punished\n");
        }
    }
    if (!st.active || st.hosting) return;
    if (mq_in_remote_act()) return;  // (the partner's copy's act: not this console's own action; put back after it)
    uint32_t who = c->r[14] & ~1u;
    if (std::find(std::begin(kSetterReturns), std::end(kSetterReturns), who) != std::end(kSetterReturns)) who = st.setter_lr;
    st.setter_lr = 0;
    if (c->r[0] != kTown + kStore) return;
    if (std::find(std::begin(kClassroomRolls), std::end(kClassroomRolls), who) != std::end(kClassroomRolls) ||
        (who >= kWeather && who < kWeatherEnd) || (who >= kSkyKind && who < kSkyKindEnd) || who == kMushroomDay)
        return;  // (a roll: the host's is the town's)
    const int at = store_byte(c->r[1]);
    if (at >= 0) st.acted[kStore + uint32_t(at)] = 1;
    else std::fill(st.acted.begin() + kStore, st.acted.begin() + kStore + kStoreSize, 1);  // (an index not known)
}

// Once a frame (from mq_frame_end, between the game's frames).
void mq_coop_sync_frame(NtrCpu* c) {
    poke_file(c);
    visit_frame(c);
    // Alone in its own town: note its own values of kOwnVars (given back when it arrives in a host's town as a guest).
    if (cfie::coop_anytime && !ram(session(c)) && ram(ntr_rd32(c, kField))) {
        home.bytes.resize(kept_vars_at().size());
        for (size_t k = 0; k < kept_vars_at().size(); k++) home.bytes[k] = uint8_t(ntr_rd8(c, kTown + kept_vars_at()[k]));
        home.valid = true;
    }
    static const bool off = std::getenv("MQ_COOP_NO_SYNC") != nullptr;  // (research: the game's own visit)
    if (off || !cfie::coop_anytime || !ntr::multiplayer_active()) {
        if (st.session) stop("the session ended");
        return;
    }
    const uint32_t s = session(c), field = ntr_rd32(c, kField);
    if (!ram(s)) {
        if (st.session) stop("the session ended");
        return;
    }
    if (s != st.session) {
        stop("a new session");
        st.session = s;
    }
    // Not standing in the host's town (still connecting, or between areas): wait; what arrives waits in the link.
    if (!ram(field) || ntr_rd8(c, field + kFieldInSession) == 0) return;
    if (!st.active) {
        const int own = own_slot(c, s);
        if (own < 0 || own > 3) return;
        st.active = true;
        st.own = own;
        st.hosting = own == 0;
        st.town = read_town(c);
        std::memcpy(st.town_id, &st.town[kTownId], kTownIdSize);
        st.record.assign(kRecordSize, 0);
        read_ram(c, kRecord, st.record.data(), kRecordSize);
        if (!st.hosting && home.valid) {  // (a guest: its own values of kOwnVars, from its own town)
            std::string given;
            for (size_t k = 0; k < kept_vars_at().size(); k++) {
                const uint32_t at = kept_vars_at()[k];
                const uint8_t m = own_vars()[at];
                const uint8_t v = uint8_t((st.town[at] & ~m) | (home.bytes[k] & m));
                if (!m || v == st.town[at]) continue;
                const uint8_t bits = uint8_t((v ^ st.town[at]) & m);
                st.town[at] = v;
                ntr_wr8(c, kTown + at, v);
                given += vars_in(at, bits, st.town);
            }
            if (!given.empty()) call(c, kChecksum, kTown, kTownSize);
            std::fprintf(stderr, "coop sync: this console's own lesson variables, from its own town:%s\n",
                         given.empty() ? " (the same as the host's)" : given.c_str());
        }
        st.pending.assign(kTownSize, 0);
        st.acted.assign(kTownSize, 0);
        st.date = ntr_rd32(c, kTown + kTownTime);
        std::fprintf(stderr, "coop sync: in the host's town as slot %d%s (node %08x)\n", own,
                     st.hosting ? ", hosting: this town is the truth" : "", ntr::coop_node());
    } else if (!same_town(c, st.town_id)) {
        stop("the town block is another town now");
        return;
    }

    // Hellos to the consoles that came into the session; those that left are forgotten.
    uint32_t nodes[8];
    const size_t n = ntr::coop_peers(nodes, 8);
    for (auto it = st.peers.begin(); it != st.peers.end();)
        it = std::find(nodes, nodes + n, it->first) == nodes + n ? st.peers.erase(it) : std::next(it);
    for (size_t i = 0; i < n; i++) {
        Peer& p = st.peers[nodes[i]];
        if (p.greeted) continue;
        const uint8_t hello[2] = {uint8_t(st.own), uint8_t(st.hosting)};
        p.greeted = ntr::coop_send(kHello, hello, sizeof hello, nodes[i]);
    }

    // A guest: its own changes to the town, as a request to the host (before what the host sent is written in).
    if (!st.hosting && st.host_node && st.peers.count(st.host_node)) {
        std::vector<uint8_t> now = read_town(c);
        const uint32_t date = ntr_rd32(c, kTown + kTownTime);
        if (date != st.date) {  // (its own day change: its rolls are its own; the host's come in its bytes)
            st.date = date;
            st.quiet = kQuietFrames;
            std::fprintf(stderr, "coop sync: the day changed here; the host's town will be asked for again\n");
        }
        if (st.quiet > 0) {
            st.town = now;
            if (--st.quiet == 0) ntr::coop_send(kResend, nullptr, 0, st.host_node);
        } else {
            std::vector<uint8_t> m;
            put32(m, st.requests + 1);
            m.insert(m.end(), st.town_id, st.town_id + kTownIdSize);
            const size_t runs_at = m.size();
            keep_vars(c, now);  // (each console's own variables and the host's alone: not sent)
            // (what the guest's game changed by itself stays here, as the guest's, until the host's values come)
            static const bool all = std::getenv("MQ_COOP_SYNC_ALL") != nullptr;  // (research: send every change)
            uint32_t local = 0;
            std::string where;
            // Over the internet a telegram written here (the post office: the entry goes into the town's waiting list,
            // town block +0x7AA8, ten entries of 0xE4 bytes) reaches the host's town only this way: the game hands the
            // entry to the host console it is linked with, which is this PC's stand-in for the host, not the host.
            // So the waiting list's changes made here are asked of the host as any other (docs/COOP_INTERNET.md).
            const bool internet = ntr::net_config().internet;
            constexpr uint32_t kWaitingList = 0x7AA8, kWaitingListSize = 10 * 0xE4;
            // The town's list of dated entries at +0x6F0C (0xE4 bytes each, the newest first; up to the waiting list;
            // event op 0x73 adds one, e.g. the other world's return scene 10204) is the host's: a scene played on both
            // consoles adds its entry on each, and the host's (sent here) and the guest's own made it twice on the
            // guest for the rest of the visit (docs/COOP_PLAYTEST.md, "Soak"). The guest's own changes to it are not
            // kept: once no event runs here the host's list is put back (as kHostVars; until then they are not sent).
            constexpr uint32_t kDatedList = 0x6F0C, kDatedListEnd = kWaitingList;
            const bool busy = event_busy(c);
            uint32_t list_back = 0;
            std::string list_where;
            for (uint32_t i = kDatedList; i < kDatedListEnd; i++) {
                if (all || now[i] == st.town[i] || town_own()[i]) continue;
                if (!busy) {
                    ntr_wr8(c, kTown + i, st.town[i]);
                    if (list_back++ < 8) {
                        char b[16];
                        std::snprintf(b, sizeof b, " +%x=%u", i, st.town[i]);
                        list_where += b;
                    }
                }
                now[i] = st.town[i];
            }
            if (list_back) {
                call(c, kChecksum, kTown, kTownSize);
                std::fprintf(stderr, "coop sync: the town's dated list as the host has it again (%u bytes this console's "
                             "scene wrote):%s\n", list_back, list_where.c_str());
            }
            if (!all) classmate_needs(c, now, busy);
            for (uint32_t i = 0; i < kTownSize; i++) {
                if (all || st.acted[i] || now[i] == st.town[i] || town_own()[i]) continue;
                if (internet && i >= kWaitingList && i < kWaitingList + kWaitingListSize) continue;
                if (local++ < 8) {
                    char b[16];
                    std::snprintf(b, sizeof b, " +%x=%u", i, now[i]);
                    where += b;
                }
                st.town[i] = now[i];
            }
            // (a telegram this guest wrote is the host's as the host's game would have made it from the transfer, ov001
            // 0x020F1470 kind 4: +0xDE 0xFA "another player", +0xE0 1; the entry here says 0xFB + its own slot)
            if (internet) {
                const uint8_t mine = uint8_t(0xFB + ntr_rd8(c, kRecord + 2));
                for (uint32_t e = kWaitingList; e < kWaitingList + kWaitingListSize; e += 0xE4)
                    if (now[e + 0xDE] == mine && std::memcmp(&now[e], &st.town[e], 0xE4) != 0) {
                        now[e + 0xDE] = 0xFA, now[e + 0xE0] = 1;
                        // (+0xC4, the sender's name the cat reads out ("From: ..."): here the addressee's; the
                        // transfer carries the sender's, this student's name in the session)
                        const uint32_t name = session(c) + 0x49E8 + uint32_t(st.own) * 0x50;
                        for (uint32_t k = 0; k < 12; k++) {
                            const uint8_t ch = ram(name) ? uint8_t(ntr_rd8(c, name + k)) : 0;
                            now[e + 0xC4 + k] = ch;
                            if (ch == 0) break;
                        }
                        for (uint32_t k = 0xC4; k < 0xE4; k++) ntr_wr8(c, kTown + e + k, now[e + k]);  // (and so here)
                        call(c, kChecksum, kTown, kTownSize);
                        std::fprintf(stderr, "coop sync: the telegram written here (waiting list +%x) goes to the host\n", e);
                    }
            }
            std::fill(st.acted.begin(), st.acted.end(), 0);
            if (local) std::fprintf(stderr, "coop sync: %u bytes this console changed by itself, kept here:%s\n", local, where.c_str());
            const uint32_t bytes = diff(now, st.town, town_own(), false, m);
            if (bytes) {
                st.requests++;
                each_byte(m.data() + runs_at, m.data() + m.size(), kTownSize,
                          [&](uint32_t at, uint8_t) { st.pending[at] = st.requests; });
                ntr::coop_send(kRequest, m.data(), m.size(), st.host_node);
                std::fprintf(stderr, "coop sync: request %u -> host: %u bytes (%s)\n", st.requests, bytes,
                             offsets(m, runs_at).c_str());
            }
        }
    }

    ntr::coop_poll(&received, c);

    if (++st.frames % kPeriod == 0 && !st.peers.empty()) {
        if (st.hosting) send_town(c, 0, false);
        send_record(c, 0, false);
    }
}
