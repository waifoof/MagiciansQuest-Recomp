// Magician's Quest: two co-op players talking to each other, and the relationship between them (docs/
// COOP_RELATIONSHIPS.md): a pair record of ours that chatting, hanging out and the game's two-player incantations
// raise, and the milestones the game gives classmates (an invitation to Lovers Point at a chosen time, a confession
// there) played between the two students.
//
// The game has no talk between players: the talk-target finder (ov001 0x020C85E4) only looks at NPCs. In a co-op
// session, when the local student presses A, no NPC is in front and the other student is, our event 90002 starts
// (a greeting from the other student, then a menu) and the other console is told, so it shows its side (event
// 90003) and keeps the same pair record. The game keeps its consoles together by sending each player's state in
// 16-byte packets, not by running every console from everyone's inputs: what one student does in an event is decided
// on their console alone, as the game's own talk to an NPC is. So the console of the student who acts decides, and a
// message of ours on the co-op side channel (ntr/coop.hpp, coop_sync::listen) carries the decision and the pair's
// new values to the other console, which shows and stores them. Each console keeps its copy beside its own save.
//
// Outside a session nothing here runs: every entry returns at once.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "cfie_game.hpp"
#include "coop_content.hpp"
#include "coop_sync.hpp"
#include "ntr/coop.hpp"
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
constexpr uint32_t kTown = 0x020AF020;           // the town save block: +0x2CC the town's hour, +0x2CD its minute
                                                 // (what the start records' time windows test, ov001 0x020F44E0)
constexpr uint32_t kQueueEvent = 0x0201DAF0;     // (file, block, 0xFF): queue an event as the town's triggers do
constexpr uint32_t kTalkFinder = 0x020C85E4;     // (field, &angle): the NPC the local student would talk to
constexpr uint32_t kEventRunning = 0x0201DF58;   // (2): an event runs or is queued
constexpr uint32_t kAtan = 0x02086A04;           // (x, z): FX_Atan2Idx, the direction of (x, z)
constexpr uint32_t kWrapAngle = 0x0200D31C;      // (angle): an angle in degrees (20.12) brought into [0, 360)
// The incantations' results (main 0x02045500): the record of a result (main 0x02045E0C with the table 0x020B8E18),
// 36 bytes: +0x11 + stage the multiplier for the caster's stage in that incantation, +0x16 what it adds to a
// classmate's value toward the caster, +0x17 to the classmate's relation slot. The stage is the caster's store field
// read with main 0x0201DFC0 (0x89 Friendship, 0x8A Declare love).
constexpr uint32_t kResultRecord = 0x02045E0C, kResultTable = 0x020B8E18;
constexpr uint32_t kStoreRead = 0x0201DFC0;
constexpr uint32_t kOwnStore = 0x020ADCFA;       // the local student's variable store (record 0x020AD824 + 0x4D6)
constexpr uint32_t kSetStudentReturn = 0x02112EDE;  // where event command 0x3B (set a student variable) calls the
                                                    // store write (ov028 0x02112EDA)
// Items. A student record (the local player's +0x4FC) holds the bag at +0x636 (15 slots, s16 item values, -1 empty)
// and the item in the student's hand at +0x65E (-1 none): holding one (dragging it from the bag onto the student, or
// picking it in the bag that the game's "Give item" to a classmate opens, event 705's query 0x03) moves it from its
// slot to the hand, and the classmate is given what is in the hand.
constexpr uint32_t kPlayerRecord = 0x4FC, kBag = 0x636, kHeld = 0x65E;
constexpr int kBagSlots = 15;
constexpr uint32_t kControlled = 0x020FE264;  // the actor the player controls (null outside the field)
constexpr int kRestoreFreeFrames = 60;          // free to move this long after a session before the look
constexpr uint32_t kBagFreeSlot = 0x02039948;  // (bag): the first empty slot, or -1
constexpr uint32_t kBagPut = 0x0203A1C0;       // (bag, slot, item): puts an item in a slot
constexpr uint32_t kItemKind = 0x020399B8;     // (item id): its kind (the item table's +5; 0 for no item)
constexpr uint32_t kSetHeld = 0x0202DF74;      // (player, value): the held item (player vtable +0x84)
// What event 705 won't let anyone give away (its test_player checks, then system message 1017).
constexpr uint16_t kKeptItems[] = {392, 101, 122, 103, 332, 114, 2106, 398, 112, 123};

// The field: +0x383C the local player (its position at +0x2C, so field+0x3868, and facing at +0x68, field+0x38A4,
// degrees 20.12), +0x3E4C the other players' objects (3 x 0x638; +0x2C position, +0xA2 their action, +0x5A0 set while
// in this area, +0x62C session slot), +0x3EB4 the area (map) the field shows.
constexpr uint32_t kLocalPlayer = 0x383C, kRemotePlayers = 0x3E4C, kRemoteSize = 0x638, kRemoteSlot = 0x62C;
constexpr uint32_t kFieldMap = 0x3EB4;
// The session: each slot's entry at +0x49E8 + slot * 0x50: +0 the student's name (24 bytes), +0x18 their student
// record (the local one, or the copy the session received).
constexpr uint32_t kSessionPlayers = 0x49E8, kSessionPlayerSize = 0x50;
// A remote player's packet queue (4 x 16 bytes at +0xE4, an empty slot holds type 0x0E), read by its update (main
// 0x02031404). Earlier builds sent our messages there as type 0x27; they go on the side channel now.
constexpr uint32_t kQueue = 0xE4;
constexpr uint8_t kOldPacketType = 0x27, kPacketDone = 0x0E;

// Talk range: the finder's for classmates (ov001 0x020C875C, 2.25 units), within 60 degrees of where the player
// faces (as the finder), and about level with them.
constexpr int32_t kTalkRange = 0x2400, kTalkHeight = 0x1000;

// The pair's values (0..100) and stages (the game's kinds, event command 0x26: 2 friends, 3 best friends, 4 couple).
constexpr int kChatFriendship = 4, kChatAffection = 1;  // a chat, once a day per pair
constexpr int kHangFriendship = 3;                      // hanging out, once a day (affection +1, a couple's +2)
constexpr int kGiftFriendship = 3, kGiftAffection = 1;  // a gift, once a day per pair (the item always goes)
constexpr int kFriendsAt = 30;                          // friends from here (on any rise)
constexpr int kBestAt = 60;                             // an invitation to Lovers Point: best friends ...
constexpr int kCoupleAffection = 40;                    // ... or, with this much affection too, a couple
constexpr int kLoversPoint = 49;                        // the area (mc_ss block 49 starts event 701 there)
constexpr int kMeetHours[5] = {10, 12, 15, 18, 20};     // the time choices (event 700, s213 1-5; event 701's start
                                                        // records take hh:00-hh:59)

// The incantations (their result ids, t199) and the action a remote player shows while casting them with you
// (+0xA2: 27 and 28, what query 0xB5 waits for in a session).
constexpr int kFriendshipSpell = 30, kLoveSpell = 32;

// Our messages on the co-op side channel.
constexpr uint8_t kMsgType = 40;
enum Op : uint8_t {
    kOpTalk = 1,    // the sender's student came over to talk
    kOpRecord = 2,  // the pair's new values, and what to show (a Show)
    kOpInvite = 3,  // an invitation to Lovers Point (arg: the time)
    kOpAnswer = 4,  // its answer (arg: 1 yes, 2 no)
    kOpMeet = 5,    // the meeting starts (arg: 1 best friends, 2 a couple too)
    kOpChoice = 6,  // the asked student's choice there (arg: 1 couple, 2 best friends, 3 not yet)
    kOpCast = 7,    // an incantation cast with the other student (arg: the Show; amount: what it adds)
    kOpHello = 8,   // the sender's copy of the record, when two students meet in a session (the one ahead wins)
    // A gift, as an exchange: nothing is taken before the receiver has room, and the giver gets it back unless the
    // receiver says it is in their bag. (item: arg | amount << 8)
    kOpGiftAsk = 9,    // is there room in your bag?
    kOpGiftRoom = 10,  // its answer (arg: 1 room, 0 full)
    kOpGift = 11,      // the item, taken from the giver's hand; the pair's new values come with it
    kOpGiftGot = 12,   // the receiver's answer (arg 1: in their bag, 0: refused (no room), so it goes back)
    kOpGiftCheck = 13,   // a gift still pending on the giver's side, asked about when the two meet again
    kOpGiftChecked = 14, // its answer (arg 1: the receiver has it, 0: never received)
    // (the gift's id travels after the fixed fields: a length byte and the id, "<giver identity>#<counter>")
};

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

bool slot_used(NtrCpu* c, uint32_t s, int slot) {
    return slot >= 0 && slot < 4 && ntr_rd8(c, s + kSessionPlayers + uint32_t(slot) * kSessionPlayerSize) != 0;
}

// The other player's object for a session slot, if it stands in this area (else 0).
uint32_t remote_here(NtrCpu* c, uint32_t field, int slot) {
    const uint32_t remotes = ntr_rd32(c, field + kRemotePlayers);
    if (!ram(remotes)) return 0;
    for (uint32_t i = 0; i < 3; i++) {
        const uint32_t o = remotes + i * kRemoteSize;
        if (ntr_rd8(c, o + kRemoteSlot) == slot && ntr_rd32(c, o + 0x20) != 0 && ntr_rd8(c, o + 0x5A0) != 0) return o;
    }
    return 0;
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
    int stage = 0;          // 0 none, 2 friends, 3 best friends, 4 couple (the game's own kinds, event command 0x26)
    int friendship = 0;     // 0..100
    int affection = 0;      // 0..100
    uint32_t last_day = 0;  // the day of the last chat that counted (year * 372 + month * 31 + day)
    int chats = 0;
    uint32_t last_hang = 0;  // the day of the last hang-out that counted
    int hangs = 0;
    int invite = 0;          // an invitation to Lovers Point: 0 none, 1 asked (no answer yet), 2 agreed
    int invite_time = 0;     // its time, 1-5 (kMeetHours)
    std::string inviter = "-";  // who asked (an identity)
    uint32_t invite_day = 0;    // the day of the last invitation or meeting (offered again on another day)
    uint32_t last_gift = 0;     // the day of the last gift that counted
    int gifts = 0;
};

// Which copy is further along (two copies that met again after one of them missed something).
bool ahead(const Pair& a, const Pair& b) {
    return std::make_tuple(a.chats + a.hangs + a.gifts, a.stage, a.friendship, a.affection, a.invite_day) >
           std::make_tuple(b.chats + b.hangs + b.gifts, b.stage, b.friendship, b.affection, b.invite_day);
}

std::map<std::string, Pair> pairs;  // by key (the two identities, in order)
// Gifts as recorded transfers (in the same file, lines starting with '@'): the giver keeps each gift it sent as
// pending until the receiver confirms it; the receiver keeps the ids of the gifts it put in its bag. Per pair key.
struct Gifts {
    // An entry: the item, and how many of it the student had (bag and hand) just after the gift left or arrived. A
    // session that fails ("Unable to receive data") can give a console back an earlier copy of its student's things,
    // so when the two meet again each side looks at what it really has before settling.
    struct Entry {
        uint16_t item = 0;
        int count = -1;  // (-1: not known)
        bool operator==(const Entry& o) const { return item == o.item && count == o.count; }
    };
    std::map<std::string, Entry> pending;                // sent by this console's student, not confirmed yet
    std::deque<std::pair<std::string, Entry>> got;       // received by this console's student, newest last
    // Gifts that went through in a session, kept until this console's game has saved (outside a session) with them
    // in place: a failed session can still give the console an earlier copy of its student's things, which brings
    // a given item back to the giver (`gave`: count is how many the giver had after it left) or takes a received one
    // from the receiver (`recv`: how many the receiver had after it arrived).
    std::map<std::string, Entry> gave, recv;
    bool operator==(const Gifts& o) const {
        return pending == o.pending && got == o.got && gave == o.gave && recv == o.recv;
    }
    bool operator!=(const Gifts& o) const { return !(*this == o); }
};
// The live records, and what was on disk when the game last saved: a console's gift records are written together
// with its own save (the bag the game saves), so after a crash the two agree: an item put in a bag but never saved is
// not recorded as received either, and the giver gets it back when the two meet again.
std::map<std::string, Gifts> gifts, gifts_saved;
std::filesystem::file_time_type save_stamp{};
bool save_stamp_known = false;
std::map<std::string, unsigned> gift_counter;  // per giver identity: the last gift number used
constexpr size_t kGotKept = 64;                // received ids kept per pair
std::filesystem::file_time_type pairs_stamp{};
bool pairs_loaded = false;

std::string pair_key(const std::string& a, const std::string& b) { return a < b ? a + " " + b : b + " " + a; }

// Read when first needed, and again whenever the file changed on disk (e.g. edited between sessions).
void load_pairs() {
    if (cfie::coop_pairs_path.empty()) return;
    std::error_code ec;
    const auto stamp = std::filesystem::last_write_time(cfie::coop_pairs_path, ec);
    if (pairs_loaded && (ec || stamp == pairs_stamp)) return;
    pairs_loaded = true;
    if (ec) return;
    pairs_stamp = stamp;
    pairs.clear();
    gifts.clear();
    gifts_saved.clear();
    gift_counter.clear();
    std::ifstream f(cfie::coop_pairs_path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        if (line[0] == '@') {  // @pending a b id item / @got a b id / @counter identity n
            std::string kind, a, b, id;
            in >> kind;
            if (kind == "@counter") {
                unsigned n = 0;
                if (in >> a >> n) gift_counter[a] = n;
            } else if (in >> a >> b >> id) {
                unsigned item = 0;
                int count = -1;
                in >> item >> count;
                if (kind == "@pending") gifts_saved[pair_key(a, b)].pending[id] = {uint16_t(item), count};
                else if (kind == "@got") gifts_saved[pair_key(a, b)].got.push_back({id, {uint16_t(item), count}});
                else if (kind == "@gave") gifts_saved[pair_key(a, b)].gave[id] = {uint16_t(item), count};
                else if (kind == "@recv") gifts_saved[pair_key(a, b)].recv[id] = {uint16_t(item), count};
            }
            continue;
        }
        std::string a, b;
        Pair p;
        if (!(in >> a >> b >> p.stage >> p.friendship >> p.affection >> p.last_day >> p.chats)) continue;
        in >> p.last_hang >> p.hangs >> p.invite >> p.invite_time >> p.inviter >> p.invite_day;  // (newer fields)
        in >> p.last_gift >> p.gifts;
        if (p.inviter.empty()) p.inviter = "-";
        pairs[pair_key(a, b)] = p;
    }
    gifts = gifts_saved;
}

// Each console keeps its own copy beside its save (cfie::coop_pairs_path, set by the frontend), written whenever a
// record changes.
void save_pairs() {
    if (cfie::coop_pairs_path.empty()) return;
    const std::string tmp = cfie::coop_pairs_path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        f << "# Magician's Quest recompilation: co-op pair records, one per pair of students who played together.\n"
             "# student student stage friendship affection last-chat-day chats last-hang-out-day hang-outs "
             "invitation(0 none, 1 asked, 2 agreed) time(1-5) inviter last-invitation-day last-gift-day gifts\n"
             "# (stage: 0 none, 2 friends, 3 best friends, 4 couple)\n";
        for (const auto& [key, p] : pairs)
            f << key << ' ' << p.stage << ' ' << p.friendship << ' ' << p.affection << ' ' << p.last_day << ' '
              << p.chats << ' ' << p.last_hang << ' ' << p.hangs << ' ' << p.invite << ' ' << p.invite_time << ' '
              << p.inviter << ' ' << p.invite_day << ' ' << p.last_gift << ' ' << p.gifts << '\n';
        f << "# gifts: @pending student student id item count (sent, not confirmed yet) / @got student student id "
             "item count (received) / @gave, @recv student student id item count (given / received in a session, kept "
             "until the game saved with it in place) / @counter student n (count: how many of the item the student "
             "had afterwards)\n";
        for (const auto& [key, g] : gifts_saved) {
            for (const auto& [id, e] : g.pending)
                f << "@pending " << key << ' ' << id << ' ' << e.item << ' ' << e.count << '\n';
            for (const auto& [id, e] : g.got) f << "@got " << key << ' ' << id << ' ' << e.item << ' ' << e.count << '\n';
            for (const auto& [id, e] : g.gave) f << "@gave " << key << ' ' << id << ' ' << e.item << ' ' << e.count << '\n';
            for (const auto& [id, e] : g.recv) f << "@recv " << key << ' ' << id << ' ' << e.item << ' ' << e.count << '\n';
        }
        for (const auto& [who, n] : gift_counter) f << "@counter " << who << ' ' << n << '\n';
        f.flush();
        if (!f) return;
    }
    std::rename(tmp.c_str(), cfie::coop_pairs_path.c_str());
    std::error_code ec;
    pairs_stamp = std::filesystem::last_write_time(cfie::coop_pairs_path, ec);
}

int item_count(NtrCpu* c, uint16_t item);
uint32_t local_record(NtrCpu* c);
// The look at the student's things after a session ended (and once after loading), see check_restored: due until
// done; the `gave` / `recv` records are not cleared by a save before it.
bool restore_check_due = true;

// When the game's save reaches the disk (the frontend writes it a moment after the game does), the gift records go
// with it. A save outside a session, after the look at the student's things, with a given item still gone (or a
// received one still there) ends its `gave` (`recv`) record.
void gifts_follow_save(NtrCpu* c) {
    if (cfie::coop_pairs_path.empty()) return;
    static int frames = 0;
    if (++frames % 30 != 0) return;
    std::error_code ec;
    const auto stamp =
        std::filesystem::last_write_time(std::filesystem::path(cfie::coop_pairs_path).replace_extension(".sav"), ec);
    if (ec) return;
    if (save_stamp_known && stamp != save_stamp) {
        load_pairs();
        if (!in_session() && !restore_check_due && local_record(c))
            for (auto& [key, g] : gifts) {
                for (auto it = g.gave.begin(); it != g.gave.end();)
                    if (item_count(c, it->second.item) <= it->second.count) {
                        std::fprintf(stderr, "coop gift: gift %s (item %u) saved as given\n", it->first.c_str(),
                                     it->second.item);
                        it = g.gave.erase(it);
                    } else ++it;
                for (auto it = g.recv.begin(); it != g.recv.end();)
                    if (item_count(c, it->second.item) >= it->second.count) {
                        std::fprintf(stderr, "coop gift: gift %s (item %u) saved as received\n", it->first.c_str(),
                                     it->second.item);
                        it = g.recv.erase(it);
                    } else ++it;
            }
        if (gifts != gifts_saved) {
            gifts_saved = gifts;
            save_pairs();
            std::fprintf(stderr, "coop gift: records saved with the game\n");
        }
    }
    save_stamp = stamp, save_stamp_known = true;
}

uint32_t today(NtrCpu* c) {
    const uint32_t clock = ntr_rd32(c, kClock);
    if (!ram(clock)) return 0;
    return ntr_rd32(c, clock + 4) * 372 + ntr_rd32(c, clock + 8) * 31 + ntr_rd32(c, clock + 0xC);
}

// A rise of the pair's values, and friends from kFriendsAt (true if this rise made them friends).
bool raise(Pair& p, int friendship, int affection) {
    p.friendship = std::clamp(p.friendship + friendship, 0, 100);
    p.affection = std::clamp(p.affection + affection, 0, 100);
    if (p.stage < 2 && p.friendship >= kFriendsAt) {
        p.stage = 2;
        return true;
    }
    return false;
}

// The invitation a talk offers (t149): 2 the pair could become a couple, 1 best friends, 0 none.
int offer(const Pair& p, uint32_t day) {
    if (p.invite != 0 || p.invite_day == day || p.stage < 2 || p.stage >= 4 || p.friendship < kBestAt) return 0;
    if (p.affection >= kCoupleAffection) return 2;
    return p.stage == 2 ? 1 : 0;
}

// ------------------------------------------------------------------------------------------------ messages, events

struct Msg {
    uint8_t op = 0, from = 0, to = 0, arg = 0, promote = 0, amount = 0;
    Pair p;
    bool inviter_is_sender = false, inviter_is_receiver = false;
};

void put32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; i++) b.push_back(uint8_t(v >> (8 * i)));
}
uint32_t get32(const uint8_t* d) { return uint32_t(d[0]) | uint32_t(d[1]) << 8 | uint32_t(d[2]) << 16 | uint32_t(d[3]) << 24; }
constexpr size_t kMsgSize = 27;      // (the first version's; the gift fields follow in newer builds)
constexpr size_t kMsgSizeGifts = 32;

// To the other student's console (the message names the student it is for; the others pass it over).
void send(int op, int own, int to, const Pair& p, const std::string& own_id, uint8_t arg = 0, bool promote = false,
          uint8_t amount = 0, const std::string& gift_id = "") {
    std::vector<uint8_t> b = {uint8_t(op), uint8_t(own), uint8_t(to), arg, uint8_t(promote ? 1 : 0), amount,
                              uint8_t(p.inviter == "-" ? 0 : p.inviter == own_id ? 1 : 2),
                              uint8_t(p.stage), uint8_t(p.friendship), uint8_t(p.affection), uint8_t(p.chats),
                              uint8_t(p.hangs), uint8_t(p.invite), uint8_t(p.invite_time)};
    put32(b, p.last_day);
    put32(b, p.last_hang);
    put32(b, p.invite_day);
    b.push_back(0);
    put32(b, p.last_gift);
    b.push_back(uint8_t(p.gifts));
    if (!gift_id.empty()) {
        b.push_back(uint8_t(std::min<size_t>(gift_id.size(), 255)));
        b.insert(b.end(), gift_id.begin(), gift_id.begin() + std::min<size_t>(gift_id.size(), 255));
    }
    ntr::coop_send(kMsgType, b.data(), b.size());
}

struct PendingEvent {
    int id;
    int other;  // the other student's slot
    std::vector<std::pair<uint32_t, uint8_t>> flags;
};
std::deque<PendingEvent> pending;  // our events for this console, queued when no other event runs

// The event of ours this console runs with the other student: whose, and the pair.
struct Scene {
    bool active = false;
    bool started = false;  // the event has been seen running
    int other = 0;
    int event = 0;
    std::string key;
} scene;

// Things to do at the end of the frame (seen by hooks inside the game's own code).
struct Cast {
    bool due = false;
    int spell = 0;  // kFriendshipSpell or kLoveSpell
} cast;

// A gift this console is giving (one at a time). Stage 1: asked the other console for room; 2: there is room, the
// script runs the hand-over; 3: the item has left the hand and was sent (held here until the other console says it
// is in their bag, else it goes back into this student's bag).
struct Giving {
    int stage = 0;
    int other = -1;
    uint16_t item = 0;   // the held item's value, as the bag keeps it
    int frames = 0;      // (waiting for the answer to the question)
    Pair next;           // the pair's values once the gift is taken (sent with it)
    bool promote = false, counted = false;
    std::string key;
    std::string id;      // the gift's id (once sent)
} giving;
uint16_t gift_given = 0, gift_received = 0;  // the items the gift lines name (coop_content)

uint32_t last_session = 0;
bool hello_sent[4] = {};
int last_map = -1;
int cast_test_frames = 0, cast_test_spell = 0;  // (test driver, MQ_COOP_CAST_FILE)

void set_flag(NtrCpu* c, uint32_t var, uint8_t value) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (ram(ctx)) ntr_wr8(c, ctx + 0x2254 + (var - 10000), value);
}
uint8_t flag(NtrCpu* c, uint32_t var) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) ? ntr_rd8(c, ctx + 0x2254 + (var - 10000)) : 0;
}

// An event runs (the script context's +8) or is being started (+0x23BC, the start kind), as Connect now tests it.
// (main 0x0201DF58, the talk's own test, answers 0 at the end of a frame while an event of ours runs.)
bool event_busy(NtrCpu* c) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    return ram(ctx) && (ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0);
}

// The local student's record (the local player's +0x4FC), or 0.
uint32_t local_record(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField);
    if (!ram(field)) return 0;
    const uint32_t rec = ntr_rd32(c, field + kLocalPlayer + kPlayerRecord);
    return ram(rec) ? rec : 0;
}

// The item in the local student's hand (-1: none).
int16_t held_item(NtrCpu* c) {
    const uint32_t rec = local_record(c);
    return rec ? int16_t(ntr_rd16(c, rec + kHeld)) : int16_t(-1);
}

bool bag_has_room(NtrCpu* c) {
    const uint32_t rec = local_record(c);
    if (!rec) return false;
    const int32_t slot = int32_t(call(c, kBagFreeSlot, rec + kBag));
    return slot >= 0 && slot < kBagSlots;
}

// Into the local student's bag with the game's own calls; true once the slot holds it.
bool put_in_bag(NtrCpu* c, uint16_t item) {
    const uint32_t rec = local_record(c);
    if (!rec) return false;
    const int32_t slot = int32_t(call(c, kBagFreeSlot, rec + kBag));
    if (slot < 0 || slot >= kBagSlots) return false;
    call(c, kBagPut, rec + kBag, uint32_t(slot), item);
    return ntr_rd16(c, rec + kBag + uint32_t(slot) * 2) == item;
}

// A gift that didn't arrive goes back: into the bag (where holding took it from), else into the hand.
bool give_back(NtrCpu* c, uint16_t item, const char* why) {
    if (put_in_bag(c, item)) {
        std::fprintf(stderr, "coop gift: item %u back in the bag (%s)\n", item, why);
        return true;
    }
    const uint32_t field = ntr_rd32(c, kField);
    if (ram(field) && held_item(c) == -1) {
        call(c, kSetHeld, field + kLocalPlayer, item);
        std::fprintf(stderr, "coop gift: item %u back in the hand (%s; the bag is full)\n", item, why);
        return true;
    }
    std::fprintf(stderr, "coop gift: item %u could not be given back yet (%s; kept pending)\n", item, why);
    return false;
}

// How many of an item the local student has (bag and hand).
int item_count(NtrCpu* c, uint16_t item) {
    const uint32_t rec = local_record(c);
    if (!rec) return 0;
    int n = ntr_rd16(c, rec + kHeld) == item ? 1 : 0;
    for (int i = 0; i < kBagSlots; i++) n += ntr_rd16(c, rec + kBag + uint32_t(i) * 2) == item ? 1 : 0;
    return n;
}

// Takes one of an item from the local student (bag first, then hand).
bool take_one(NtrCpu* c, uint16_t item) {
    const uint32_t rec = local_record(c);
    if (!rec) return false;
    for (int i = 0; i < kBagSlots; i++)
        if (ntr_rd16(c, rec + kBag + uint32_t(i) * 2) == item) {
            ntr_wr16(c, rec + kBag + uint32_t(i) * 2, 0xFFFF);
            return true;
        }
    if (ntr_rd16(c, rec + kHeld) == item) {
        call(c, kSetHeld, ntr_rd32(c, kField) + kLocalPlayer, 0xFFFF);
        return true;
    }
    return false;
}

// A pending gift settled. Received: the entry goes (and if a failed session gave this student an earlier copy of
// their things, with the item, it is taken again: the other student has it). Never arrived or refused: the item
// comes back (unless such a copy already brought it back), and the entry goes once it is back.
void settle_gift(NtrCpu* c, const std::string& key, const std::string& id, bool received, const char* why) {
    auto& g = gifts[key];
    const auto it = g.pending.find(id);
    if (it == g.pending.end()) return;
    const Gifts::Entry e = it->second;
    const bool restored = e.count >= 0 && item_count(c, e.item) > e.count;
    bool done = true;
    if (received && restored) {
        take_one(c, e.item);
        std::fprintf(stderr, "coop gift: item %u was given back to this student by the game; taken again (%s)\n",
                     e.item, id.c_str());
    } else if (!received && restored) {
        std::fprintf(stderr, "coop gift: item %u is already back (%s)\n", e.item, id.c_str());
    } else if (!received) {
        done = give_back(c, e.item, why);
    }
    if (received) g.gave[id] = e;  // (until the game saved with it gone: see check_restored)
    if (done) g.pending.erase(it);
    save_pairs();
}

// Right after a session ended (and once after loading), once the student is free to move: a session that failed
// gives the console an earlier copy of its student's things (the game's own rollback). A gift that went through
// comes back to the giver with it (one item would become two once the receiver saves): taken again. A gift the
// receiver had put in its bag goes with it although the giver's console counts it as given (one item would be
// lost): put back. The counts recorded after each gift tell; the highest given (lowest received) first, so several
// gifts of one item are each seen. Returns the item for the line to the player, or 0.
uint16_t check_restored(NtrCpu* c, bool& given) {
    uint16_t told = 0;
    std::vector<std::pair<std::string, Gifts::Entry>> gave, recv;
    for (const auto& [key, g] : gifts) {
        gave.insert(gave.end(), g.gave.begin(), g.gave.end());
        recv.insert(recv.end(), g.recv.begin(), g.recv.end());
    }
    std::stable_sort(gave.begin(), gave.end(), [](const auto& a, const auto& b) { return a.second.count > b.second.count; });
    std::stable_sort(recv.begin(), recv.end(), [](const auto& a, const auto& b) { return a.second.count < b.second.count; });
    for (const auto& [id, e] : gave) {
        const int n = item_count(c, e.item);
        if (e.count < 0 || n <= e.count) continue;
        if (take_one(c, e.item)) {
            std::fprintf(stderr, "coop gift: gift %s: the game gave item %u back to this student (%d, %d after the "
                                 "gift); taken again: the other student has it\n", id.c_str(), e.item, n, e.count);
            told = e.item, given = true;
        }
    }
    for (const auto& [id, e] : recv) {
        const int n = item_count(c, e.item);
        if (e.count < 0 || n >= e.count) continue;
        if (give_back(c, e.item, "the game took a received gift away")) {
            std::fprintf(stderr, "coop gift: gift %s: the game took item %u from this student (%d, %d after the gift); "
                                 "put back: the other student gave it\n", id.c_str(), e.item, n, e.count);
            if (!told) told = e.item, given = false;
        }
    }
    return told;
}

// Queues one of our events the way the town's triggers do (main 0x0201DAF0, then start kind 2 at the context's
// +0x23BC); false while another event runs.
bool queue_event(NtrCpu* c, int id) {
    const uint32_t ctx = ntr_rd32(c, kScriptContext);
    if (!ram(ctx) || call(c, kQueueEvent, uint32_t(id / 100), uint32_t(id % 100), 0xFF) == 0) return false;
    ntr_wr8(c, ctx + 0x23BC, 2);
    return true;
}

// The values the talking student's menu shows (flag bytes t141-t156), from the pair's record.
void fill_flags(NtrCpu* c, int other, const Pair& p, uint32_t day) {
    set_flag(c, content::kTalkOther, uint8_t(content::kOtherPlayer0 + other));
    set_flag(c, content::kTalkStage, uint8_t(p.stage));
    set_flag(c, content::kTalkTopic, uint8_t((p.chats + p.hangs) % 4));
    set_flag(c, content::kTalkToday, p.last_day == day ? 1 : 0);
    set_flag(c, content::kTalkChat, 0);
    set_flag(c, content::kTalkBand, uint8_t(std::min(p.friendship / 25, 4)));
    set_flag(c, content::kTalkPromote,
             p.last_day != day && p.stage < 2 && p.friendship + kChatFriendship >= kFriendsAt ? 1 : 0);
    set_flag(c, content::kTalkOffer, uint8_t(offer(p, day)));
    set_flag(c, content::kTalkTime, uint8_t(p.invite == 2 ? p.invite_time : 0));
    set_flag(c, content::kTalkAnswer, 0);
    set_flag(c, content::kTalkHung, p.last_hang == day ? 1 : 0);
    set_flag(c, content::kTalkMeeting, p.invite == 2 ? 1 : 0);
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
        if (slot >= 4 || slot == own || !slot_used(c, s, slot)) continue;  // (no student in that slot)
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

// Shopkeepers in a session. Out of one, a shopkeeper walks about behind the counter (the full brain) and comes within
// the finder's reach of a student at the counter (2.25 units for Piquard, Daisy, Vivian and Roley; ov001 0x020C875C).
// In a session every NPC gets the online brain, whose think does nothing (docs/COOP.md, "The town's people"), and
// the shops' events skip the shopkeeper's walk to the counter (query 0x22), so the shopkeeper stays where the shop
// placed them, a little past the finder's reach from the counter's front (Piquard: 2.5 units): nobody could talk to
// them, so nothing could be bought. So, in a session only, when the finder finds no one, a shopkeeper (NPC 126-133)
// within the finder's own tests and its reach plus kCounterReach is talked to as the talk call does it (main
// 0x02028100 after the finder).
constexpr int32_t kCounterReach = 0xC00;  // 0.75 units

uint32_t shopkeeper_in_reach(NtrCpu* c, uint32_t field, int32_t& angle) {
    const uint32_t npcs = ntr_rd32(c, field + 0x3E48);
    const int count = int8_t(ntr_rd8(c, field + 0x3EAD));
    if (!ram(npcs)) return 0;
    const int32_t px = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x2C)),
                  py = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x30)),
                  pz = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x34));
    const int32_t facing = int32_t(ntr_rd32(c, field + kLocalPlayer + 0x68));
    uint32_t best = 0;
    double best_d = 0;
    for (int i = 0; i < count; i++) {
        const uint32_t o = npcs + uint32_t(i) * 0x2B0;
        const uint32_t trig = ntr_rd32(c, o + 0x20);
        if (!ram(trig) || call(c, 0x0201EF48, trig) == 0) continue;  // (the finder's: a trigger, shown)
        const uint32_t id = call(c, 0x020140E4, o);
        if (id < 126 || id > 133 || call(c, 0x020C8708, field, id) != 0) continue;
        if (int32_t(ntr_rd32(c, o + 0x30)) != py) continue;  // (the finder's height test, for these)
        const int32_t dx = int32_t(ntr_rd32(c, o + 0x2C)) - px, dz = int32_t(ntr_rd32(c, o + 0x34)) - pz;
        const double d = std::hypot(double(dx), double(dz));
        if (d >= double(int32_t(call(c, 0x020C875C, field, id)) + kCounterReach) || (best && d >= best_d)) continue;
        const int32_t dir = int32_t((int64_t(int32_t(call(c, kAtan, uint32_t(dx), uint32_t(dz)))) * 360) >> 4);
        if (call(c, kWrapAngle, uint32_t(dir - facing + (15 << 14))) >= uint32_t(30 << 14)) continue;
        if (call(c, ntr_rd32(c, ntr_rd32(c, o) + 0x13C), o) == 0) continue;  // (the NPC's own "can talk")
        best = o, best_d = d, angle = dir;
    }
    return best;
}

// The talk call's own steps once its finder has an NPC (main 0x02028132-0x0202819A); r0 = 1 as it returns.
int talk_to_shopkeeper(NtrCpu* c, uint32_t field) {
    int32_t angle = 0;
    const uint32_t npc = shopkeeper_in_reach(c, field, angle);
    if (!npc) return 0;
    const uint32_t player = c->r[0];
    call(c, ntr_rd32(c, ntr_rd32(c, player) + 0x90), player);
    const uint32_t a = call(c, ntr_rd32(c, ntr_rd32(c, npc) + 0x74), npc);
    const uint32_t b = call(c, ntr_rd32(c, ntr_rd32(c, npc) + 0x78), npc);
    ntr_wr32(c, player + 0x6C, uint32_t(angle));
    ntr_wr32(c, player + 0x14, npc);
    ntr_wr8(c, player + 0x5F0, uint8_t(a));
    ntr_wr8(c, player + 0x5F1, uint8_t(b));
    ntr_wr8(c, player + 0x5F2, ntr_rd8(c, npc + 0x2A6));
    call(c, 0x0201DEAC, 0xEA, call(c, ntr_rd32(c, ntr_rd32(c, npc) + 0x7C), npc));  // (t234)
    call(c, 0x02020134, player, 1, 0, 0);
    call(c, 0x0201E4DC, player, 2);
    ntr_wr8(c, player + 0xBD, 0xFF);
    ntr_wr8(c, player + 0x528, 3);
    std::fprintf(stderr, "coop talk: to shopkeeper %u across the counter\n", call(c, 0x020140E4, npc));
    c->r[0] = 1;
    return 1;
}

const char* stage_name(int stage) {
    return stage == 4 ? "couple" : stage == 3 ? "best friends" : stage == 2 ? "friends" : "none";
}

void log_pair(const char* what, int other, const Pair& p) {
    std::fprintf(stderr, "coop pair: %s (slot %d: stage %d %s, friendship %d, affection %d, invitation %d at %d)\n",
                 what, other, p.stage, stage_name(p.stage), p.friendship, p.affection, p.invite, p.invite_time);
}

// A message of ours from another console (coop_sync's handlers run at the end of a frame, in the host's town).
void on_message(NtrCpu* c, uint32_t /*node*/, const uint8_t* d, size_t len) {
    if (len < kMsgSize || !in_session()) return;
    const uint32_t s = session(c);
    if (!ram(s)) return;
    const int own = own_slot(c, s);
    Msg m;
    m.op = d[0], m.from = d[1], m.to = d[2], m.arg = d[3], m.promote = d[4], m.amount = d[5];
    if (m.from >= 4 || m.from == own || m.to != own) return;
    const std::string own_id = identity(c, s, own), other_id = identity(c, s, m.from);
    m.p.inviter = d[6] == 1 ? other_id : d[6] == 2 ? own_id : "-";
    m.p.stage = d[7], m.p.friendship = d[8], m.p.affection = d[9], m.p.chats = d[10], m.p.hangs = d[11];
    m.p.invite = d[12], m.p.invite_time = d[13];
    m.p.last_day = get32(d + 14), m.p.last_hang = get32(d + 18), m.p.invite_day = get32(d + 22);
    load_pairs();
    const std::string key = pair_key(own_id, other_id);
    Pair& p = pairs[key];
    if (len >= kMsgSizeGifts) m.p.last_gift = get32(d + 27), m.p.gifts = d[31];
    else m.p.last_gift = p.last_gift, m.p.gifts = p.gifts;  // (an older build's message)
    std::string gift_id;
    if (len > kMsgSizeGifts && len >= kMsgSizeGifts + 1 + d[kMsgSizeGifts])
        gift_id.assign(reinterpret_cast<const char*>(d + kMsgSizeGifts + 1), d[kMsgSizeGifts]);
    const int from = m.from;
    switch (m.op) {
    case kOpTalk:
        pending.push_back({content::kEventTalkedTo, from, {}});
        std::fprintf(stderr, "coop pair: slot %d came over to talk\n", from);
        return;
    case kOpHello:
        if (ahead(m.p, p)) {
            p = m.p;
            save_pairs();
            log_pair("a newer copy from the other console", from, p);
        }
        return;
    case kOpRecord:
        p = m.p;
        save_pairs();
        log_pair("new values from the other console", from, p);
        pending.push_back({content::kEventChatShared, from,
                           {{content::kTalkShow, m.arg}, {content::kTalkPromote, m.promote},
                            {content::kTalkStage, uint8_t(p.stage)}}});
        return;
    case kOpInvite:
        p = m.p;
        save_pairs();
        log_pair("invited to Lovers Point", from, p);
        pending.push_back({content::kEventInvited, from, {{content::kTalkTime, uint8_t(p.invite_time)}}});
        return;
    case kOpAnswer:
        p = m.p;
        save_pairs();
        log_pair(m.arg == 1 ? "the invitation was accepted" : "the invitation was declined", from, p);
        pending.push_back({content::kEventInviteAnswer, from,
                           {{content::kTalkAnswer, m.arg}, {content::kTalkTime, uint8_t(m.p.invite_time)}}});
        return;
    case kOpMeet:
        std::fprintf(stderr, "coop pair: slot %d is waiting at Lovers Point (%s)\n", from,
                     m.arg == 2 ? "a confession" : "best friends");
        pending.push_back({content::kEventMeetInvited, from,
                           {{content::kTalkOffer, m.arg}, {content::kTalkAnswer, 0}, {content::kTalkChat, 0}}});
        return;
    case kOpChoice: {
        p = m.p;
        save_pairs();
        log_pair("the answer at Lovers Point", from, p);
        bool given = false;
        for (PendingEvent& e : pending)
            if (e.id == content::kEventMeetInviter)
                for (auto& [var, value] : e.flags)
                    if (var == content::kTalkAnswer) value = m.arg, given = true;
        if (!given && scene.active && scene.event == content::kEventMeetInviter) set_flag(c, content::kTalkAnswer, m.arg);
        return;
    }
    case kOpGiftAsk: {
        const bool room = bag_has_room(c);
        send(kOpGiftRoom, own, from, p, own_id, room ? 1 : 0);
        std::fprintf(stderr, "coop gift: slot %d asks to give item %u: %s\n", from, m.arg | m.amount << 8,
                     room ? "there is room" : "the bag is full");
        return;
    }
    case kOpGiftRoom:
        if (giving.stage != 1 || giving.other != from) return;
        std::fprintf(stderr, "coop gift: slot %d %s\n", from, m.arg ? "has room" : "has a full bag");
        if (m.arg) {
            giving.stage = 2;
            set_flag(c, content::kTalkGift, content::kGiftReady);
        } else {
            giving = {};
            set_flag(c, content::kTalkGift, content::kGiftFull);
        }
        return;
    case kOpGift: {
        const uint16_t item = uint16_t(m.arg | m.amount << 8);
        auto& got = gifts[key].got;
        const bool had = !gift_id.empty() && std::find_if(got.begin(), got.end(), [&](const auto& g) {
                             return g.first == gift_id;
                         }) != got.end();
        const bool ok = had || put_in_bag(c, item);
        if (ok && !had) {
            p = m.p;
            if (!gift_id.empty()) {  // (recorded with the item, in the same write)
                got.push_back({gift_id, {item, item_count(c, item)}});
                while (got.size() > kGotKept) got.pop_front();
                gifts[key].recv[gift_id] = {item, item_count(c, item)};
            }
            for (auto& [k, g] : gifts)  // (one more of an item this student gave earlier: not the game's doing)
                for (auto& [id, e] : g.gave)
                    if (e.item == item && e.count >= 0) e.count++;
            save_pairs();
            gift_received = item;
            // (test, MQ_COOP_GIFT_STOP=1: the receiving copy stops right after it put the item in the bag, before its
            // answer leaves: the session drops with the gift unconfirmed)
            if (std::getenv("MQ_COOP_GIFT_STOP")) {
                std::fprintf(stderr, "coop gift: item %u in the bag; stopping before the answer (test)\n", item);
                std::fflush(stderr);
                std::_Exit(0);
            }
            pending.push_back({content::kEventGiftReceived, from, {{content::kTalkPromote, m.promote}}});
        }
        send(kOpGiftGot, own, from, p, own_id, ok ? 1 : 0, false, 0, gift_id);
        std::fprintf(stderr, "coop gift: item %u from slot %d (%s) %s\n", item, from, gift_id.c_str(),
                     had ? "was already in the bag" : ok ? "is in the bag" : "didn't fit: refused");
        if (ok && !had) log_pair("after the gift", from, p);
        return;
    }
    case kOpGiftCheck: {
        auto& got = gifts[key].got;
        const auto g = std::find_if(got.begin(), got.end(), [&](const auto& e) { return e.first == gift_id; });
        bool has = g != got.end();
        // (received, but a failed session gave this student an earlier copy of their things, without it: not had)
        if (has && g->second.count >= 0 && item_count(c, g->second.item) < g->second.count) {
            has = false;
            got.erase(g);
            std::fprintf(stderr, "coop gift: gift %s was received but is no longer here (the game went back to an "
                                 "earlier copy)\n", gift_id.c_str());
        }
        send(kOpGiftChecked, own, from, p, own_id, has ? 1 : 0, false, 0, gift_id);
        std::fprintf(stderr, "coop gift: slot %d asks about gift %s: %s\n", from, gift_id.c_str(),
                     has ? "received" : "never received");
        return;
    }
    case kOpGiftChecked:
        std::fprintf(stderr, "coop gift: gift %s %s\n", gift_id.c_str(),
                     m.arg ? "was received: no longer pending" : "never arrived: it comes back");
        settle_gift(c, key, gift_id, m.arg == 1, "it never arrived");
        return;
    case kOpGiftGot:
        if (giving.stage != 3 || giving.other != from || (!gift_id.empty() && gift_id != giving.id)) {
            if (!gift_id.empty()) settle_gift(c, key, gift_id, m.arg == 1, "refused");  // (an earlier gift)
            return;
        }
        settle_gift(c, key, giving.id, m.arg == 1, "the other bag was full");
        if (m.arg == 1) {
            pairs[giving.key] = giving.next;
            save_pairs();
            gift_given = giving.item;
            set_flag(c, content::kTalkToday, giving.counted ? 0 : 1);
            set_flag(c, content::kTalkPromote, giving.promote ? 1 : 0);
            set_flag(c, content::kTalkGift, content::kGiftGiven);
            std::fprintf(stderr, "coop gift: item %u is in slot %d's bag\n", giving.item, from);
            log_pair("after the gift", from, giving.next);
        } else {
            set_flag(c, content::kTalkGift, content::kGiftBack);
        }
        giving = {};
        return;
    case kOpCast: {
        const bool love = m.arg == content::kShowLove;
        const bool promote = raise(p, love ? 0 : m.amount, love ? m.amount : 0);
        save_pairs();
        log_pair(love ? "Declare love from the other student" : "Friendship from the other student", from, p);
        pending.push_back({content::kEventChatShared, from,
                           {{content::kTalkShow, m.arg}, {content::kTalkPromote, uint8_t(promote ? 1 : 0)},
                            {content::kTalkStage, uint8_t(p.stage)}}});
        return;
    }
    default:
        return;
    }
}

// What the script of ours that runs asked for (t145), applied here: the pair's new values are written and sent.
void apply_action(NtrCpu* c, uint32_t s, int own, uint8_t action) {
    const std::string own_id = identity(c, s, own);
    Pair& p = pairs[scene.key];
    const uint32_t day = today(c);
    const int other = scene.other;
    switch (action) {
    case content::kActChat: {
        bool promote = false;
        if (p.last_day != day) {
            p.last_day = day;
            p.chats++;
            promote = raise(p, kChatFriendship, kChatAffection);
            save_pairs();
        }
        send(kOpRecord, own, other, p, own_id, content::kShowChat, promote);
        log_pair("a chat", other, p);
        return;
    }
    case content::kActHangOut:
        if (p.last_hang != day) {
            p.last_hang = day;
            p.hangs++;
            raise(p, kHangFriendship, p.stage == 4 ? 2 : 1);
            save_pairs();
        }
        send(kOpRecord, own, other, p, own_id, content::kShowHangOut);
        log_pair("hanging out", other, p);
        return;
    case content::kActInvite:
        p.invite = 1;
        p.invite_time = std::clamp<int>(flag(c, content::kTalkTime), 1, 5);
        p.inviter = own_id;
        p.invite_day = day;
        save_pairs();
        send(kOpInvite, own, other, p, own_id, uint8_t(p.invite_time));
        log_pair("an invitation to Lovers Point", other, p);
        return;
    case content::kActBreakUp:
        // (the game's breakup, command 0x26 kind 11: a value of 100 or more drops to 70, any other loses 32)
        p.friendship = p.friendship >= 100 ? 70 : std::max(0, p.friendship - 32);
        p.affection = p.affection >= 100 ? 70 : std::max(0, p.affection - 32);
        p.stage = p.friendship >= kFriendsAt ? 2 : 0;
        p.invite = 0, p.inviter = "-", p.invite_day = day;
        save_pairs();
        send(kOpRecord, own, other, p, own_id, content::kShowBreakUp);
        log_pair("back to being friends", other, p);
        return;
    case content::kActInviteYes:
    case content::kActInviteNo: {
        const bool yes = action == content::kActInviteYes;
        p.invite = yes ? 2 : 0;
        if (!yes) p.inviter = "-";
        save_pairs();
        send(kOpAnswer, own, other, p, own_id, yes ? 1 : 2);
        log_pair(yes ? "said yes" : "said no", other, p);
        return;
    }
    case content::kActGiveAsk: {
        const int16_t held = held_item(c);  // (picked from the bag by the game's own picker, query 0x03)
        const uint16_t id = uint16_t(held) & 0xFFF;
        if (held == -1 || id == 0 || call(c, kItemKind, id) == 0) {
            set_flag(c, content::kTalkGift, content::kGiftNothing);
            std::fprintf(stderr, "coop gift: nothing in the hand\n");
            return;
        }
        if (std::find(std::begin(kKeptItems), std::end(kKeptItems), id) != std::end(kKeptItems)) {
            set_flag(c, content::kTalkGift, content::kGiftKept);
            std::fprintf(stderr, "coop gift: item %u can't be given away\n", id);
            return;
        }
        if (giving.stage != 0) {  // (one at a time)
            set_flag(c, content::kTalkGift, content::kGiftNoAnswer);
            return;
        }
        giving = {};
        giving.stage = 1, giving.other = other, giving.item = uint16_t(held), giving.key = scene.key;
        send(kOpGiftAsk, own, other, p, own_id, uint8_t(giving.item), false, uint8_t(giving.item >> 8));
        std::fprintf(stderr, "coop gift: asking slot %d for room for item %u\n", other, giving.item);
        return;
    }
    case content::kActGiveHand: {
        if (giving.stage != 2 || giving.other != other) {
            giving = {};
            set_flag(c, content::kTalkGift, content::kGiftNoAnswer);
            return;
        }
        const int16_t held = held_item(c);
        if (held == int16_t(giving.item)) {
            call(c, kSetHeld, ntr_rd32(c, kField) + kLocalPlayer, 0xFFFF);
        } else if (held != -1) {  // (another item in the hand: nothing is taken)
            std::fprintf(stderr, "coop gift: the hand holds %d now, not %u: not given\n", held, giving.item);
            giving = {};
            set_flag(c, content::kTalkGift, content::kGiftNoAnswer);
            return;
        }
        if (held_item(c) != -1) {
            std::fprintf(stderr, "coop gift: the hand still holds item %u: not given\n", giving.item);
            giving = {};
            set_flag(c, content::kTalkGift, content::kGiftNoAnswer);
            return;
        }
        Pair next = p;
        giving.counted = next.last_gift != day;
        if (giving.counted) {
            next.last_gift = day;
            next.gifts++;
            giving.promote = raise(next, kGiftFriendship, kGiftAffection);
        }
        giving.next = next;
        giving.stage = 3;
        giving.id = own_id + "#" + std::to_string(++gift_counter[own_id]);
        gifts[scene.key].pending[giving.id] = {giving.item, item_count(c, giving.item)};  // (before it is sent)
        for (auto& [k, g] : gifts)  // (one fewer of an item this student received earlier: not the game's doing)
            for (auto& [id, e] : g.recv)
                if (e.item == giving.item && e.count > 0) e.count--;
        save_pairs();
        send(kOpGift, own, other, next, own_id, uint8_t(giving.item), giving.promote, uint8_t(giving.item >> 8),
             giving.id);
        std::fprintf(stderr, "coop gift: item %u taken from the hand and sent to slot %d\n", giving.item, other);
        return;
    }
    case content::kActMeetAnswer: {
        const uint8_t answer = flag(c, content::kTalkAnswer);
        if (answer == 1) p.stage = 4;
        else if (answer == 2) p.stage = std::max(p.stage, 3);
        p.invite = 0, p.inviter = "-", p.invite_day = day;
        save_pairs();
        send(kOpChoice, own, other, p, own_id, answer);
        log_pair(answer == 1 ? "a couple" : answer == 2 ? "best friends" : "not yet", other, p);
        return;
    }
    default:
        return;
    }
}

// Friendship or Declare love succeeded in a session (the game's events 320 / 344 set the incantation's learned flag
// there, s481 / s482, and nothing else): query 0xB5 let it through only because another student was casting it with
// this one (their action +0xA2: 27 Friendship, 28 Declare love). The game's own rule for a classmate target (main
// 0x02045874) is applied to the pair: the result record's +0x17 (Friendship) or +0x16 (Declare love) times the
// multiplier for the caster's stage in it; friendship or affection rises by that, and the other console is told.
void apply_cast(NtrCpu* c, uint32_t field, uint32_t s, int own) {
    const bool love = cast.spell == kLoveSpell;
    cast = {};
    const uint8_t action = love ? 28 : 27;
    int other = -1;
    for (int slot = 0; slot < 4 && other < 0; slot++)  // (the student casting it with this one)
        if (slot != own && slot_used(c, s, slot))
            if (const uint32_t o = remote_here(c, field, slot); o && ntr_rd8(c, o + 0xA2) == action) other = slot;
    for (int slot = 0; slot < 4 && other < 0; slot++)  // (else the other student here)
        if (slot != own && slot_used(c, s, slot) && remote_here(c, field, slot)) other = slot;
    if (other < 0) {
        std::fprintf(stderr, "coop pair: %s succeeded, but no other student is here\n", love ? "Declare love" : "Friendship");
        return;
    }
    const uint32_t level = std::min<uint32_t>(call(c, kStoreRead, kOwnStore, love ? 0x8A : 0x89, 0), 5);
    const uint32_t rec = call(c, kResultRecord, kResultTable, love ? 31 : 29);
    int amount = 2;
    if (ram(rec)) amount = int(int8_t(ntr_rd8(c, rec + 0x11 + level))) * int(ntr_rd8(c, rec + (love ? 0x16 : 0x17)));
    amount = std::clamp(amount, 1, 100);
    load_pairs();
    const std::string own_id = identity(c, s, own);
    Pair& p = pairs[pair_key(own_id, identity(c, s, other))];
    const bool promote = raise(p, love ? 0 : amount, love ? amount : 0);
    save_pairs();
    const uint8_t show = love ? content::kShowLove : content::kShowFriendship;
    send(kOpCast, own, other, p, own_id, show, promote, uint8_t(amount));
    std::fprintf(stderr, "coop pair: %s reached slot %d (level %u, +%d)\n", love ? "Declare love" : "Friendship", other,
                 level, amount);
    log_pair("after the incantation", other, p);
    pending.push_back({content::kEventCastResult, other,
                       {{content::kTalkShow, show}, {content::kTalkPromote, uint8_t(promote ? 1 : 0)}}});
}

// The meeting: when a pair agreed to meet at Lovers Point, the one who asked stands there with the other student at
// the chosen hour (hh:00-hh:59, as event 701's start records), and no event runs, the scene starts on both consoles.
void check_meeting(NtrCpu* c, uint32_t field, uint32_t s, int own) {
    const int map = ntr_rd8(c, field + kFieldMap);
    if (map != last_map) {
        last_map = map;
        std::fprintf(stderr, "coop pair: in area %d (%02d:%02d)\n", map, ntr_rd8(c, kTown + 0x2CC), ntr_rd8(c, kTown + 0x2CD));
    }
    if (map != kLoversPoint || scene.active || !pending.empty() || event_busy(c)) return;
    const int hour = int8_t(ntr_rd8(c, kTown + 0x2CC));
    const std::string own_id = identity(c, s, own);
    for (int other = 0; other < 4; other++) {
        if (other == own || !slot_used(c, s, other) || !remote_here(c, field, other)) continue;
        const std::string key = pair_key(own_id, identity(c, s, other));
        auto it = pairs.find(key);
        if (it == pairs.end()) continue;
        const Pair& p = it->second;
        if (p.invite != 2 || p.inviter != own_id || p.invite_time < 1 || p.invite_time > 5 ||
            hour != kMeetHours[p.invite_time - 1])
            continue;
        const uint8_t kind = p.affection >= kCoupleAffection && p.friendship >= kBestAt && p.stage < 4 ? 2 : 1;
        send(kOpMeet, own, other, p, own_id, kind);
        pending.push_back({content::kEventMeetInviter, other,
                           {{content::kTalkOffer, kind}, {content::kTalkAnswer, 0}, {content::kTalkTime, uint8_t(p.invite_time)}}});
        std::fprintf(stderr, "coop pair: meeting slot %d at Lovers Point (%s)\n", other, kind == 2 ? "a confession" : "best friends");
        return;
    }
}

// (test driver, MQ_COOP_CAST_FILE=<path>: when <path> appears, holding 30 (Friendship) or 32 (Declare love), the
// incantation's event is started as the magic menu would after the words were said (320 or 344 block 1, with t199,
// the incantation's id), while the other student here is shown casting it with this one (their +0xA2, 27 or 28, as
// query 0xB5 looks for it). Casting itself needs the touch screen's incantation input.)
void cast_test(NtrCpu* c, uint32_t field, uint32_t s, int own) {
    static const char* path = std::getenv("MQ_COOP_CAST_FILE");
    if (!path) return;
    if (cast_test_spell == 0) {
        FILE* f = std::fopen(path, "rb");
        if (!f) return;
        int id = 0;
        if (std::fscanf(f, "%d", &id) != 1) id = 0;
        std::fclose(f);
        std::remove(path);
        if (id != kFriendshipSpell && id != kLoveSpell) return;
        cast_test_spell = id;
    }
    if (cast_test_frames == 0) {
        set_flag(c, 10199, uint8_t(cast_test_spell));
        if (!queue_event(c, cast_test_spell == kLoveSpell ? 34401 : 32001)) return;
        cast_test_frames = 120;
        std::fprintf(stderr, "coop pair: test cast of %d\n", cast_test_spell);
    }
    for (int slot = 0; slot < 4; slot++)
        if (slot != own)
            if (const uint32_t o = remote_here(c, field, slot)) ntr_wr8(c, o + 0xA2, cast_test_spell == kLoveSpell ? 28 : 27);
    (void)s;
    if (--cast_test_frames == 0) cast_test_spell = 0;
}

}  // namespace

// main 0x02028100 (player): the local student's try at talking, when A is pressed while free to walk (the call at
// 0x020272F8; it asks the finder for an NPC and enters the talk state). In a co-op session, if no NPC would answer
// and the other student is in front, our talk starts instead: the event, and a message that tells the other console.
extern "C" int mq_coop_talk(NtrCpu* c) {
    if (!in_session() || (c->r[14] & ~1u) != 0x020272FC) return 0;
    const uint32_t player = c->r[0], field = ntr_rd32(c, kField), s = session(c);
    if (!ram(field) || !ram(s) || player != field + kLocalPlayer) return 0;
    if (int8_t(ntr_rd8(c, player + 0xBC)) == 1 || call(c, kEventRunning, 2) != 0) return 0;  // (the call's own tests)
    if (scene.active || !pending.empty()) return 0;  // (one of ours is about to start)
    c->r[13] -= 8;
    const uint32_t npc = call(c, kTalkFinder, field, c->r[13]);
    c->r[13] += 8;
    if (npc != 0) return 0;
    const int own = own_slot(c, s);
    if (own < 0 || own >= 4) return 0;
    const int other = other_in_front(c, field, s, own);
    if (other < 0) return talk_to_shopkeeper(c, field);
    load_pairs();
    const std::string own_id = identity(c, s, own);
    const std::string key = pair_key(own_id, identity(c, s, other));
    Pair& p = pairs[key];
    const uint32_t day = today(c);
    if (p.invite == 1 && p.invite_day != day) p.invite = 0, p.inviter = "-";  // (an invitation never answered)
    if (!queue_event(c, content::kEventPlayerTalk)) return 0;
    fill_flags(c, other, p, day);
    scene = {true, false, other, content::kEventPlayerTalk, key};
    send(kOpTalk, own, other, p, own_id);
    std::fprintf(stderr, "coop talk: slot %d talks to slot %d (stage %d, friendship %d, affection %d, offer %d) [%s]\n",
                 own, other, p.stage, p.friendship, p.affection, offer(p, day), key.c_str());
    c->r[0] = 0;  // (no NPC: the free state goes on to its other buttons; the event starts next frame)
    return 1;
}

void mq_class_remote(NtrCpu* c, uint32_t player);  // (game/src/coop_class.cpp)
// main 0x02031404 (remote player): its update, which takes the packets its console received from that player.
// Our messages travel on the side channel now; a type 0x27 packet of ours from an earlier build is dropped here (the
// update itself would pass over it: its jump table's default).
extern "C" void mq_coop_remote(NtrCpu* c) {
    if (!in_session()) return;
    const uint32_t o = c->r[0];
    if (!ram(o)) return;
    mq_class_remote(c, o);  // (in the classroom: the other student at the desk beside)
    for (uint32_t i = 0; i < 4; i++) {
        const uint32_t at = o + kQueue + i * 16;
        if (ntr_rd8(c, at) == kOldPacketType && ntr_rd8(c, at + 14) == 'M' && ntr_rd8(c, at + 15) == 'Q')
            ntr_wr8(c, at, kPacketDone);
    }
}

// Before the store write (main 0x0201E070; r0 the store, r1 the index, r2 the value): Friendship or Declare love
// succeeded in a session when event command 0x3B sets its learned flag in the local student's store (only events
// 320 and 344 write s481 / s482, and only in a session). Applied at the end of the frame.
void mq_coop_talk_store(NtrCpu* c) {
    if (!in_session() || c->r[0] != kOwnStore || c->r[2] != 1 || (c->r[14] & ~1u) != kSetStudentReturn) return;
    if (c->r[1] == 481) cast = {true, kFriendshipSpell};
    else if (c->r[1] == 482) cast = {true, kLoveSpell};
}

// Once a frame (from mq_frame_end): applies what our scripts asked for, starts the other side's events, the meeting
// at Lovers Point, and the incantations' effect on the pair.
void mq_coop_talk_frame(NtrCpu* c) {
    gifts_follow_save(c);
    static bool listening = false;
    if (!listening) {
        coop_sync::listen(kMsgType, on_message);
        listening = true;
    }
    if (!in_session()) {
        if (scene.active && scene.event == content::kEventMeetInviter && flag(c, content::kTalkAnswer) == 0)
            set_flag(c, content::kTalkAnswer, 3);  // (the other student left: the meeting ends, nothing changes)
        // (after a session, the student's things as the game left them: see check_restored)
        if (restore_check_due) {
            static int free_frames = 0;
            const uint32_t ctx = ntr_rd32(c, kScriptContext);
            const bool free = !ram(session(c)) && local_record(c) && ram(ntr_rd32(c, kControlled)) && !event_busy(c) &&
                              call(c, kEventRunning, 2) == 0;
            free_frames = free ? free_frames + 1 : 0;
            if (free_frames >= kRestoreFreeFrames && ram(ctx)) {
                free_frames = 0;
                restore_check_due = false;
                load_pairs();
                bool given = false;
                if (const uint16_t item = check_restored(c, given)) {
                    save_pairs();
                    content::set_gift_items(given ? item : 0, given ? 0 : item);
                    set_flag(c, content::kTalkGift, given ? 1 : 2);
                    queue_event(c, content::kEventGiftKept);
                }
            }
        }
        // (a gift sent but not confirmed stays pending in the record: settled the next time the two meet)
        if (pending.empty() && !scene.active && !cast.due && last_session == 0 && giving.stage == 0) return;
        pending.clear(), scene = {}, cast = {}, last_session = 0, last_map = -1, giving = {};
        return;
    }
    const uint32_t field = ntr_rd32(c, kField), s = session(c);
    if (!ram(field) || !ram(s)) return;
    restore_check_due = true;  // (looked at again once this session is over)
    if (s != last_session) {
        last_session = s;
        std::fill(std::begin(hello_sent), std::end(hello_sent), false);
    }
    const int own = own_slot(c, s);
    if (own < 0 || own >= 4) return;
    load_pairs();
    // (the first time two students are both here in a session, each sends its copy: the one further along wins)
    if (coop_sync::own_slot() >= 0) {
        const std::string own_id = identity(c, s, own);
        for (int other = 0; other < 4; other++) {
            if (other == own || hello_sent[other] || !slot_used(c, s, other)) continue;
            hello_sent[other] = true;
            const auto it = pairs.find(pair_key(own_id, identity(c, s, other)));
            if (it != pairs.end()) send(kOpHello, own, other, it->second, own_id);
            // (gifts this student sent that were never confirmed: does the other student have them?)
            if (const auto g = gifts.find(pair_key(own_id, identity(c, s, other))); g != gifts.end())
                for (const auto& [id, e] : g->second.pending) {
                    send(kOpGiftCheck, own, other, it != pairs.end() ? it->second : Pair{}, own_id, 0, false, 0, id);
                    std::fprintf(stderr, "coop gift: asking slot %d about pending gift %s (item %u)\n", other,
                                 id.c_str(), e.item);
                }
        }
    }
    if (scene.active) {
        if (const uint8_t action = flag(c, content::kTalkChat); action != 0) {
            set_flag(c, content::kTalkChat, 0);
            apply_action(c, s, own, action);
        }
        // (the other student left while this one waits for their answer at Lovers Point)
        if (scene.event == content::kEventMeetInviter && !slot_used(c, s, scene.other) && flag(c, content::kTalkAnswer) == 0)
            set_flag(c, content::kTalkAnswer, 3);
        const bool running = event_busy(c);  // (over once the event has run and ended)
        if (running) scene.started = true;
        else if (scene.started) scene = {};
    }
    // A gift: no answer to the question (nothing was taken); the other student left after the item was sent (it
    // comes back); the talk ended before the hand-over (nothing was taken).
    if (giving.stage == 1 && ++giving.frames > 600) {
        std::fprintf(stderr, "coop gift: no answer from slot %d\n", giving.other);
        giving = {};
        if (scene.active) set_flag(c, content::kTalkGift, content::kGiftNoAnswer);
    } else if (giving.stage == 3 && !slot_used(c, s, giving.other)) {
        // (the other student left before confirming: the gift stays pending, settled when the two meet again)
        std::fprintf(stderr, "coop gift: slot %d left before confirming gift %s: kept pending\n", giving.other,
                     giving.id.c_str());
        giving = {};
        if (scene.active) set_flag(c, content::kTalkGift, content::kGiftPending);
    } else if (giving.stage == 2 && !scene.active) {
        giving = {};
    }
    content::set_gift_items(gift_given, gift_received);
    if (cast.due) apply_cast(c, field, s, own);
    cast_test(c, field, s, own);
    check_meeting(c, field, s, own);
    if (!pending.empty() && !scene.active) {
        const PendingEvent& e = pending.front();
        if (!slot_used(c, s, e.other)) {
            pending.pop_front();
        } else if (queue_event(c, e.id)) {
            const std::string key = pair_key(identity(c, s, own), identity(c, s, e.other));
            const Pair& p = pairs[key];
            fill_flags(c, e.other, p, today(c));
            for (const auto& [var, value] : e.flags) set_flag(c, var, value);
            scene = {true, false, e.other, e.id, key};
            pending.pop_front();
        }
    }
}

// main 0x0201403C (text box, &kind, &name): the speaker's name above a text box. The game takes it from the speaker's
// classmate record (their object's vtable +0x28), which a player object doesn't have, so a box spoken by the other
// student would be titled "[?]". The talk command keeps only the speaker's NPC number in the box (+0x1474), which is 0
// for any player, so while one of our events runs, a box of speaker 0 is the other student's (our scripts give the
// local student no lines); a speaker given as a player's actor (110 + slot, or 446 + slot) is theirs too. Those are
// titled with the name from the session's entry; the rest is done as the game does it.
extern "C" int mq_coop_speaker(NtrCpu* c) {
    if (!in_session()) return 0;
    const uint32_t box = c->r[0], kind = c->r[1], out = c->r[2], s = session(c);
    if (!ram(box) || !ram(s)) return 0;
    const int32_t speaker = int32_t(ntr_rd32(c, box + 0x1474));
    int slot = -1;
    if (speaker == 0 && scene.active) slot = scene.other;
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
