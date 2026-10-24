// Magician's Quest: two co-op players talking to each other (docs/COOP_RELATIONSHIPS.md, "Talking to the other
// player"), and the pair's record of ours that chatting raises.
//
// The game has no talk between players: the talk-target finder (ov001 0x020C85E4) only looks at NPCs. In a co-op
// session, when the local student presses A, no NPC is in front and the other student is, our event 90002 starts
// (a greeting from the other student, then a menu) and the other console is told with a packet of ours, so it shows
// its side (event 90003) and keeps the same pair record. The game keeps its consoles together by sending each
// player's state (position, action) in 16-byte packets, one outgoing slot per console, not by running every console
// from everyone's inputs: what one student does in an event is decided on their console alone, as the game's own
// talk to an NPC is. So the talking console decides, and its packets carry the decision and the pair's new values
// to the other console, which only shows and stores them.
//
// Outside a session nothing here runs: every entry returns at once.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"

namespace cfie {
std::string coop_pairs_path;
}

namespace {

namespace content = mq::content;

// The game's addresses (USA, YNNE).
constexpr uint32_t kField = 0x020AD7EC;          // the town/field object
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the wireless session, null when alone)
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context (+0x2254: the flag bytes t0-t255)
constexpr uint32_t kClock = 0x020AD704;          // the clock object: +4 year, +8 month, +0xC day (the DS's RTC date)
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kSendPacket = 0x0202C4CC;     // (player, packet): send a 16-byte packet to the other consoles
constexpr uint32_t kTalkFinder = 0x020C85E4;     // (field, &angle): the NPC the local student would talk to
constexpr uint32_t kEventRunning = 0x0201DF58;   // (2): an event runs or is queued
constexpr uint32_t kAtan = 0x02086A04;           // (x, z): FX_Atan2Idx, the direction of (x, z)
constexpr uint32_t kWrapAngle = 0x0200D31C;      // (angle): an angle in degrees (20.12) brought into [0, 360)

// The field: +0x383C the local player (its position at +0x2C, so field+0x3868, and facing at +0x68, field+0x38A4,
// degrees 20.12), +0x3E4C the other players' objects (3 x 0x638; +0x2C position, +0x62C session slot).
constexpr uint32_t kLocalPlayer = 0x383C, kRemotePlayers = 0x3E4C, kRemoteSize = 0x638, kRemoteSlot = 0x62C;
// The session: each slot's entry at +0x49E8 + slot * 0x50: +0 the student's name (24 bytes), +0x18 their student
// record (the local one, or the copy the session received).
constexpr uint32_t kSessionPlayers = 0x49E8, kSessionPlayerSize = 0x50;
// A remote player's packet queue (4 x 16 bytes at +0xE4, an empty slot holds type 0x0E), read by its update (main
// 0x02031404), which ignores type 0x27 (its jump table's default).
constexpr uint32_t kQueue = 0xE4;
constexpr uint8_t kPacketType = 0x27, kPacketDone = 0x0E;
constexpr uint8_t kMagic0 = 'M', kMagic1 = 'Q';
enum PacketKind : uint8_t { kTalk = 1, kResult = 2 };

// Talk range: the finder's for classmates (ov001 0x020C875C, 2.25 units), within 60 degrees of where the player
// faces (as the finder), and about level with them.
constexpr int32_t kTalkRange = 0x2400, kTalkHeight = 0x1000;

// Chatting: how much one chat (at most one a day per pair) raises the pair's values, and where friends begin.
constexpr int kChatFriendship = 4, kChatAffection = 1, kFriendsAt = 30;

bool in_session() {
    static const bool solo = std::getenv("MQ_COOP_SOLO") != nullptr;
    return cfie::coop_anytime && (solo || ntr::multiplayer_active());
}

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

// Calls a game function with up to 4 arguments, keeping every register as it was; returns r0.
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

// This console's slot in the session (the session's own answer, its vtable +0x30, as the game's packets carry it).
int own_slot(NtrCpu* c, uint32_t s) {
    const uint32_t vt = ntr_rd32(c, s);
    if (!ram(vt)) return -1;
    return int(call(c, ntr_rd32(c, vt + 0x30), s) & 0xFF);
}

// ------------------------------------------------------------------------------------------------ pair records

// A student as both consoles know them: their name and the 8 bytes at their record's +0x0C, which the game keeps
// with the save (they look like a value made when the save was started: the same in two saves copied from one).
std::string identity(NtrCpu* c, uint32_t s, int slot) {
    const uint32_t entry = s + kSessionPlayers + uint32_t(slot) * kSessionPlayerSize;
    std::string id;
    char hex[4];
    for (uint32_t i = 0; i < 24; i++) {
        const uint8_t b = ntr_rd8(c, entry + i);
        if (b == 0) break;
        std::snprintf(hex, sizeof hex, "%02x", b);
        id += hex;
    }
    id += '-';
    const uint32_t record = ntr_rd32(c, entry + 0x18);
    for (uint32_t i = 0; i < 8; i++) {
        std::snprintf(hex, sizeof hex, "%02x", ram(record) ? ntr_rd8(c, record + 0x0C + i) : 0);
        id += hex;
    }
    return id;
}

struct Pair {
    int stage = 0;       // 0 none, 2 friends, 3 best friends, 4 couple (the game's own kinds, event command 0x26)
    int friendship = 0;  // 0..100
    int affection = 0;   // 0..100
    uint32_t last_day = 0;  // the day of the last chat that counted (year * 372 + month * 31 + day)
    int chats = 0;
};

std::map<std::string, Pair> pairs;  // by key (the two identities, in order)
bool pairs_loaded = false;

std::string pair_key(const std::string& a, const std::string& b) { return a < b ? a + " " + b : b + " " + a; }

void load_pairs() {
    if (pairs_loaded) return;
    pairs_loaded = true;
    if (cfie::coop_pairs_path.empty()) return;
    std::ifstream f(cfie::coop_pairs_path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        std::string a, b;
        Pair p;
        if (in >> a >> b >> p.stage >> p.friendship >> p.affection >> p.last_day >> p.chats) pairs[pair_key(a, b)] = p;
    }
}

// Each console keeps its own copy beside its save (cfie::coop_pairs_path, set by the frontend), written whenever a
// record changes.
void save_pairs() {
    if (cfie::coop_pairs_path.empty()) return;
    const std::string tmp = cfie::coop_pairs_path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        f << "# Magician's Quest recompilation: co-op pair records, one per pair of students who played together.\n"
             "# student student stage friendship affection last-chat-day chats (stage: 0 none, 2 friends, 3 best "
             "friends, 4 couple)\n";
        for (const auto& [key, p] : pairs)
            f << key << ' ' << p.stage << ' ' << p.friendship << ' ' << p.affection << ' ' << p.last_day << ' '
              << p.chats << '\n';
        if (!f) return;
    }
    std::rename(tmp.c_str(), cfie::coop_pairs_path.c_str());
}

uint32_t today(NtrCpu* c) {
    const uint32_t clock = ntr_rd32(c, kClock);
    if (!ram(clock)) return 0;
    return ntr_rd32(c, clock + 4) * 372 + ntr_rd32(c, clock + 8) * 31 + ntr_rd32(c, clock + 0xC);
}

// ------------------------------------------------------------------------------------------------ packets, events

struct Outgoing {
    uint8_t bytes[16];
};
std::deque<Outgoing> outgoing;  // sent one per frame, each until the game takes it (its one slot may be busy)
uint8_t next_seq = 1;
uint8_t last_seq[4] = {};  // the last packet taken from each slot (a packet may be seen in more than one frame)

struct PendingEvent {
    int id;
    uint8_t other;  // the other student's slot
    uint8_t promote;
};
std::deque<PendingEvent> pending;  // our events for this console, queued when no other event runs

// The conversation this console's student started: whose, and the pair.
struct Talk {
    bool active = false;
    bool started = false;  // the event has been seen running
    int other = 0;
    std::string key;
} talk;

void set_flag(NtrCpu* c, uint32_t var, uint8_t value) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + (var - 10000), value);
}
uint8_t flag(NtrCpu* c, uint32_t var) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (var - 10000)) : 0;
}

// Queues one of our events the way the town's triggers do (main 0x0201DAF0, then start kind 2 at the context's
// +0x23BC); false while another event runs.
bool queue_event(NtrCpu* c, int id) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) == 0) return false;
    ntr_wr8(c, ctx + 0x23BC, 2);
    return true;
}

void send(int kind, int target, const Pair& p, bool promote) {
    Outgoing o = {};
    o.bytes[0] = kPacketType;
    o.bytes[2] = uint8_t(kind);
    o.bytes[3] = uint8_t(target);
    o.bytes[4] = next_seq;
    next_seq = next_seq == 255 ? 1 : next_seq + 1;  // (never 0: a slot's last sequence starts at 0)
    o.bytes[5] = uint8_t(p.stage);
    o.bytes[6] = uint8_t(p.friendship);
    o.bytes[7] = uint8_t(p.affection);
    for (int i = 0; i < 4; i++) o.bytes[8 + i] = uint8_t(p.last_day >> (8 * i));
    o.bytes[12] = uint8_t(p.chats);
    o.bytes[13] = promote ? 1 : 0;
    o.bytes[14] = kMagic0;
    o.bytes[15] = kMagic1;
    outgoing.push_back(o);
}

// The values the talking student's menu shows (flag bytes t141-t148), from the pair's record.
void fill_flags(NtrCpu* c, int other, const Pair& p, uint32_t day) {
    set_flag(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + other));
    set_flag(c, content::kTalkStage, uint8_t(p.stage));
    set_flag(c, content::kTalkTopic, uint8_t(p.chats % 4));
    set_flag(c, content::kTalkToday, p.last_day == day ? 1 : 0);
    set_flag(c, content::kTalkChat, 0);
    set_flag(c, content::kTalkBand, uint8_t(std::min(p.friendship / 25, 4)));
    set_flag(c, content::kTalkPromote,
             p.last_day != day && p.stage < 2 && p.friendship + kChatFriendship >= kFriendsAt ? 1 : 0);
}

// The other student in front of the local one, as the talk-target finder would pick an NPC: here, within reach
// and within 60 degrees of where the local student faces. Their session slot, or -1.
int other_in_front(NtrCpu* c, uint32_t field, uint32_t s, int own) {
    const uint32_t remotes = ntr_rd32(c, field + kRemotePlayers);
    if (!ram(remotes)) return -1;
    const int32_t px = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x2C)),
                  py = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x30)),
                  pz = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x34));
    const int32_t facing = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x68));
    int best = -1;
    double best_d = kTalkRange;
    for (uint32_t i = 0; i < 3; i++) {
        const uint32_t o = remotes + i * kRemoteSize;
        const int slot = ntr_rd8(c, o + kRemoteSlot);
        if (slot >= 4 || slot == own || ntr_rd8(c, s + kSessionPlayers + uint32_t(slot) * kSessionPlayerSize) == 0)
            continue;  // (no student in that slot)
        // (in this area: +0x5A0, set each frame by the player's own test of its place, main 0x0202C8F8; a player
        // object has no model link at +4, so the finder's shown test, 0x0201EF48, is always false for one)
        if (ntr_rd32(c, o + 0x20) == 0 || ntr_rd8(c, o + 0x5A0) == 0 || int8_t(ntr_rd8(c, o + 0xBC)) == 1) continue;
        const int32_t dx = int32_t(ntr_rd32(c, o + 0x2C)) - px, dz = int32_t(ntr_rd32(c, o + 0x34)) - pz;
        const int32_t dy = int32_t(ntr_rd32(c, o + 0x30)) - py;
        const double d = std::hypot(double(dx), double(dz));
        if (d >= best_d || dy > kTalkHeight || dy < -kTalkHeight) continue;
        // (as the finder: the direction in degrees, 20.12, against the facing, within +-60)
        const int32_t dir = int32_t((int64_t(int32_t(call(c, kAtan, uint32_t(dx), uint32_t(dz)))) * 360) >> 4);
        if (call(c, kWrapAngle, uint32_t(dir - facing + (15 << 14))) >= uint32_t(30 << 14)) continue;
        best = slot, best_d = d;
    }
    return best;
}

}  // namespace

// main 0x02028100 (player): the local student's try at talking, when A is pressed while free to walk (the call at
// 0x020272F8; it asks the finder for an NPC and enters the talk state). In a co-op session, if no NPC would answer
// and the other student is in front, our talk starts instead: the event, and a packet that tells the other console.
extern "C" int mq_coop_talk(NtrCpu* c) {
    if (!in_session() || (c->r[14] & ~1u) != 0x020272FC) return 0;
    const uint32_t player = c->r[0], field = ntr_rd32(c, kField), s = session(c);
    if (!ram(field) || !ram(s) || player != field + kLocalPlayer) return 0;
    if (int8_t(ntr_rd8(c, player + 0xBC)) == 1 || call(c, kEventRunning, 2) != 0) return 0;  // (the call's own tests)
    c->r[13] -= 8;
    const uint32_t npc = call(c, kTalkFinder, field, c->r[13]);
    c->r[13] += 8;
    if (npc != 0) return 0;
    const int own = own_slot(c, s);
    if (own < 0 || own >= 4) return 0;
    const int other = other_in_front(c, field, s, own);
    if (other < 0) return 0;
    load_pairs();
    const std::string key = pair_key(identity(c, s, own), identity(c, s, other));
    const Pair p = pairs[key];
    const uint32_t day = today(c);
    if (!queue_event(c, content::kEventPlayerTalk)) return 0;
    fill_flags(c, other, p, day);
    talk = {true, false, other, key};
    send(kTalk, other, p, false);
    std::fprintf(stderr, "coop talk: slot %d talks to slot %d (stage %d, friendship %d, affection %d)\n", own, other,
                 p.stage, p.friendship, p.affection);
    c->r[0] = 0;  // (no NPC: the free state goes on to its other buttons; the event starts next frame)
    return 1;
}

namespace {
// A packet of ours from another console: a talk aimed at this console's student, or a pair's new values.
void take_packet(NtrCpu* c, uint32_t s, const uint8_t* b) {
    const int from = b[1], own = own_slot(c, s);
    if (from >= 4 || from == own || b[3] != own || b[4] == last_seq[from]) return;
    last_seq[from] = b[4];
    Pair p;
    p.stage = b[5], p.friendship = b[6], p.affection = b[7], p.chats = b[12];
    p.last_day = uint32_t(b[8]) | uint32_t(b[9]) << 8 | uint32_t(b[10]) << 16 | uint32_t(b[11]) << 24;
    std::fprintf(stderr, "coop talk: from slot %d: %s (stage %d, friendship %d, affection %d)\n", from,
                 b[2] == kTalk ? "talks to us" : "a chat", p.stage, p.friendship, p.affection);
    if (b[2] == kTalk) {
        pending.push_back({content::kEventTalkedTo, uint8_t(from), 0});
    } else if (b[2] == kResult) {
        load_pairs();
        pairs[pair_key(identity(c, s, own), identity(c, s, from))] = p;
        save_pairs();
        pending.push_back({content::kEventChatShared, uint8_t(from), b[13]});
    }
}

bool ours(NtrCpu* c, uint32_t at) {
    return ntr_rd8(c, at) == kPacketType && ntr_rd8(c, at + 14) == kMagic0 && ntr_rd8(c, at + 15) == kMagic1;
}

void read_packet(NtrCpu* c, uint32_t at, uint8_t* b) {
    for (uint32_t k = 0; k < 16; k++) b[k] = ntr_rd8(c, at + k);
}
}  // namespace

// main 0x02031404 (remote player): its update, which takes the packets its console received from that player. Ours
// (type 0x27) are taken out first (the update itself would pass over them: its jump table's default).
extern "C" void mq_coop_remote(NtrCpu* c) {
    if (!in_session()) return;
    const uint32_t o = c->r[0], s = session(c);
    if (!ram(o) || !ram(s)) return;
    for (uint32_t i = 0; i < 4; i++) {
        const uint32_t at = o + kQueue + i * 16;
        if (!ours(c, at)) continue;
        uint8_t b[16];
        read_packet(c, at, b);
        ntr_wr8(c, at, kPacketDone);
        take_packet(c, s, b);
    }
}

// Once a frame (from mq_frame_end): sends what is waiting, starts the other side's events, and applies a chat the
// talking student picked (t145, set by event 90002).
void mq_coop_talk_frame(NtrCpu* c) {
    if (!in_session()) {
        if (outgoing.empty() && pending.empty() && !talk.active) return;
        outgoing.clear(), pending.clear(), talk = {};
        return;
    }
    const uint32_t field = ntr_rd32(c, kField), s = session(c);
    if (!ram(field) || !ram(s)) return;
    if (talk.active && flag(c, content::kTalkChat) == 1) {
        set_flag(c, content::kTalkChat, 0);
        Pair& p = pairs[talk.key];
        const uint32_t day = today(c);
        bool promote = false;
        if (p.last_day != day) {
            p.friendship = std::min(100, p.friendship + kChatFriendship);
            p.affection = std::min(100, p.affection + kChatAffection);
            p.last_day = day;
            p.chats++;
            if (p.stage < 2 && p.friendship >= kFriendsAt) p.stage = 2, promote = true;
            save_pairs();
        }
        send(kResult, talk.other, p, promote);
        std::fprintf(stderr, "coop talk: chat with slot %d (stage %d, friendship %d, affection %d)\n", talk.other,
                     p.stage, p.friendship, p.affection);
    }
    if (talk.active) {  // (over once the event has run and ended)
        const bool running = call(c, kEventRunning, 2) != 0;
        if (running) talk.started = true;
        else if (talk.started) talk = {};
    }
    if (!outgoing.empty()) {
        c->r[13] -= 16;
        for (uint32_t k = 0; k < 16; k++) ntr_wr8(c, c->r[13] + k, outgoing.front().bytes[k]);
        if (call(c, kSendPacket, field + kLocalPlayer, c->r[13]) == 1) outgoing.pop_front();
        c->r[13] += 16;
    }
    // (The session also keeps the last packets received from each slot, at +0x4A08 and +0x4A18 of its entry: read
    // there too, in case one is never handed to the player's queue. The sequence number keeps each one once.)
    for (uint32_t slot = 0; slot < 4; slot++)
        for (uint32_t at : {s + 0x4A08 + slot * kSessionPlayerSize, s + 0x4A18 + slot * kSessionPlayerSize})
            if (ours(c, at)) {
                uint8_t b[16];
                read_packet(c, at, b);
                take_packet(c, s, b);
            }
    if (!pending.empty()) {
        const PendingEvent e = pending.front();
        if (queue_event(c, e.id)) {
            set_flag(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + e.other));
            set_flag(c, content::kTalkPromote, e.promote);
            pending.pop_front();
        }
    }
}

// main 0x0201403C (text box, &kind, &name): the speaker's name above a text box. The game takes it from the speaker's
// classmate record (their object's vtable +0x28), which a player object doesn't have, so a box spoken by the other
// student would be titled "[?]". The talk command keeps only the speaker's NPC number in the box (+0x1474), which is 0
// for any player, so while our talk (event 90002) runs, a box of speaker 0 is the other student's (our scripts give
// the local student no lines); a speaker given as a player's actor (110 + slot, or 446 + slot) is theirs too. Those
// are titled with the name from the session's entry; the rest is done as the game does it.
extern "C" int mq_coop_speaker(NtrCpu* c) {
    if (!in_session()) return 0;
    const uint32_t box = c->r[0], kind = c->r[1], out = c->r[2], s = session(c);
    if (!ram(box) || !ram(s)) return 0;
    const int32_t speaker = int32_t(ntr_rd32(c, box + 0x1474));
    int slot = -1;
    if (speaker == 0 && talk.active) slot = talk.other;
    if (speaker >= content::kOtherPlayer0 && speaker < content::kOtherPlayer0 + 4) slot = speaker - content::kOtherPlayer0;
    if (speaker >= 446 && speaker <= 449) slot = speaker - 446;  // (the resolver's other range for the session's players)
    const uint32_t entry = slot < 0 ? 0 : s + kSessionPlayers + uint32_t(slot) * kSessionPlayerSize;
    if (slot < 0 || ntr_rd8(c, entry) == 0) return 0;
    ntr_wr32(c, kind, 3);
    ntr_wr32(c, box + 0x147C, uint32_t(speaker));
    ntr_wr32(c, out, entry);  // (the name, 24 bytes: the game's own text)
    if (ntr_rd32(c, box + 0x17BC) != 0) {  // (a name a script set for the box wins, as in the game)
        ntr_wr32(c, out, box + 0x177C);
        if (const uint32_t v = ntr_rd32(c, box + 0x17C0); v != 0) {
            ntr_wr32(c, box + 0x147C, v);
            ntr_wr32(c, box + 0x2214, 1);
        }
    }
    return 1;
}
