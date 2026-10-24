// Magician's Quest: event scripts and text lines of our own (see coop_content.hpp). Everything here is written by
// us; the game's own formats are rebuilt around it at start-up.
#include "coop_content.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>

namespace mq::content {
namespace {

void put16(std::vector<uint8_t>& b, size_t at, uint32_t v) {
    b[at] = uint8_t(v), b[at + 1] = uint8_t(v >> 8);
}
void put32(std::vector<uint8_t>& b, size_t at, uint32_t v) {
    for (int i = 0; i < 4; i++) b[at + i] = uint8_t(v >> (8 * i));
}

// The game's loaders unpack every block of an event file or a text bank as LZ10 (flags 0xC/0xD in main 0x02004620).
// Literal runs only: a zero flag byte, then eight bytes as they are.
std::vector<uint8_t> lz10(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out = {0x10, uint8_t(data.size()), uint8_t(data.size() >> 8), uint8_t(data.size() >> 16)};
    for (size_t i = 0; i < data.size(); i += 8) {
        out.push_back(0);
        for (size_t j = i; j < i + 8 && j < data.size(); j++) out.push_back(data[j]);
    }
    while (out.size() % 4) out.push_back(0);
    return out;
}

// A container as event files and text banks are: u32 count, count x (u32 offset, u32 size), the blocks (an empty
// one has size 0).
std::vector<uint8_t> container(const std::vector<std::vector<uint8_t>>& blocks) {
    std::vector<uint8_t> out(4 + 8 * blocks.size(), 0);
    put32(out, 0, uint32_t(blocks.size()));
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].empty()) continue;
        const std::vector<uint8_t> packed = lz10(blocks[i]);
        put32(out, 4 + 8 * i, uint32_t(out.size()));
        put32(out, 8 + 8 * i, uint32_t(packed.size()));
        out.insert(out.end(), packed.begin(), packed.end());
    }
    return out;
}

// A text bank: block n holds lines n*100 .. n*100+99 as u32 100, 100 x (u32 offset, u32 size), the lines. A line is
// "<id>\t", each of its 16 pages followed by a tab (unused pages empty), then "\r\n", as the game's own; a new line
// in a box is U+25BC.
std::vector<uint8_t> text_bank(const std::vector<Line>& lines) {
    std::map<int, std::vector<const Line*>> by_block;
    int last = -1;
    for (const Line& l : lines) {
        by_block[l.id / 100].push_back(&l);
        last = std::max(last, l.id / 100);
    }
    std::vector<std::vector<uint8_t>> blocks(size_t(last + 1));
    for (auto& [n, in] : by_block) {
        std::vector<uint8_t>& b = blocks[size_t(n)];
        b.assign(4 + 8 * 100, 0);
        put32(b, 0, 100);
        for (const Line* l : in) {
            std::string s = std::to_string(l->id) + "\t";
            for (size_t p = 0; p < 16; p++) {
                if (p < l->pages.size())
                    for (char ch : l->pages[p]) s += ch == '\n' ? std::string("\xE2\x96\xBC") : std::string(1, ch);
                s += '\t';
            }
            s += "\r\n";
            put32(b, 4 + 8 * (l->id % 100), uint32_t(b.size()));
            put32(b, 8 + 8 * (l->id % 100), uint32_t(s.size()));
            b.insert(b.end(), s.begin(), s.end());
        }
    }
    return container(blocks);
}

std::vector<uint8_t> event_file(const std::vector<Script>& blocks) {
    std::vector<std::vector<uint8_t>> built;
    for (const Script& s : blocks) built.push_back(s.build());
    return container(built);
}

// ---------------------------------------------------------------------------------------------- our content

// The demo (event 90001, MQ_EVENT_FILE): two text boxes of ours, a yes/no menu, the answer kept in a script variable
// and read back.
std::vector<Line> lines() {
    return {
        {0, {"Hello, $500;! This box comes from\nthe recompilation, not the card."}},
        {1, {"New scenes can be written in the\ngame's own script language.", "Shall we try one out?", "$019'2;"}},
        {2, choice({{"Yes", 3}, {"No", 4}})},
        {3, {"Wonderful! I'll remember that."}},
        {4, {"All right, another time then."}},
        {5, {"(The script kept your answer: yes.)"}},
        {6, {"(The script kept your answer: no.)"}},

        // Talking to the other player (events 90002-90004). $503'641; is the other student's name (the actor in
        // t141), $500; your own.
        {100, {"Oh, hi, $500;!"}},
        {101, {"What would you like to do with\n$503'641;?", "$019'102;"}},
        {102, choice({{"Chat", -1}, {"How are we getting along?", -1}, {"Never mind", -1}})},  // (-1: the box closes)
        {110, {"You chat with $503'641; about\nclasses. Time flies by!"}},
        {111, {"You and $503'641; swap stories\nabout the town's strange creatures."}},
        {112, {"$503'641; tells you about a secret\nspot in town. You both laugh a lot."}},
        {113, {"You talk with $503'641; about the\nincantations you'd like to learn."}},
        {114, {"You feel a little closer to\n$503'641;."}},
        {115, {"You've already had a good long\nchat with $503'641; today."}},
        {116, {"You and $503'641; are friends now!"}},
        {120, {"You and $503'641; have only just\nmet."}},
        {121, {"You and $503'641; are getting to\nknow each other."}},
        {122, {"You and $503'641; get along well."}},
        {123, {"You and $503'641; are good friends."}},
        {124, {"You and $503'641; are best friends."}},
        {125, {"You and $503'641; are a couple."}},
        {126, {"You and $503'641; get along\nwonderfully."}},
        {130, {"Maybe later, then."}},
        {140, {"$503'641; came over to talk with\nyou."}},
        {141, {"You had a nice chat with\n$503'641;."}},
    };
}

// The talking student's side (event 90002): the other student greets them, then a menu. "Chat" asks coop_talk to
// raise the pair's values (t145) and shows one of four chats in turn; at most once a day it counts. "How are we
// getting along?" reads the pair's stage, or before they are friends, how far along they are.
Script player_talk() {
    Script s;
    s.face(kPlayer, kOtherActor, 10)
        .talk(kOtherActor, 100)
        .talk(kNarrator, 101)
        .if_var(kMenuChoice, Cmp::Eq, 0, "chat")
        .if_var(kMenuChoice, Cmp::Eq, 1, "along")
        .talk(kNarrator, 130)
        .go("end")
        // Chat
        .label("chat")
        .if_var(kTalkToday, Cmp::Eq, 1, "chat lines")
        .set_local(kTalkChat, 1)
        .label("chat lines")
        .if_var(kTalkTopic, Cmp::Eq, 1, "topic 1")
        .if_var(kTalkTopic, Cmp::Eq, 2, "topic 2")
        .if_var(kTalkTopic, Cmp::Eq, 3, "topic 3")
        .talk(kNarrator, 110)
        .go("after chat")
        .label("topic 1")
        .talk(kNarrator, 111)
        .go("after chat")
        .label("topic 2")
        .talk(kNarrator, 112)
        .go("after chat")
        .label("topic 3")
        .talk(kNarrator, 113)
        .label("after chat")
        .if_var(kTalkToday, Cmp::Eq, 1, "already")
        .talk(kNarrator, 114)
        .if_var(kTalkPromote, Cmp::Ne, 1, "end")
        .talk(kNarrator, 116)
        .go("end")
        .label("already")
        .talk(kNarrator, 115)
        .go("end")
        // How are we getting along?
        .label("along")
        .if_var(kTalkStage, Cmp::Eq, 4, "couple")
        .if_var(kTalkStage, Cmp::Eq, 3, "best friends")
        .if_var(kTalkStage, Cmp::Eq, 2, "friends")
        .if_var(kTalkBand, Cmp::Eq, 0, "band 0")
        .if_var(kTalkBand, Cmp::Eq, 1, "band 1")
        .talk(kNarrator, 122)
        .go("end")
        .label("band 0")
        .talk(kNarrator, 120)
        .go("end")
        .label("band 1")
        .talk(kNarrator, 121)
        .go("end")
        .label("friends")
        .if_var(kTalkBand, Cmp::Ge, 3, "close friends")
        .talk(kNarrator, 123)
        .go("end")
        .label("close friends")
        .talk(kNarrator, 126)
        .go("end")
        .label("best friends")
        .talk(kNarrator, 124)
        .go("end")
        .label("couple")
        .talk(kNarrator, 125)
        .label("end")
        .end();
    return s;
}

// The other student's side (events 90003 and 90004): they turn to the one who came over, and later share the chat.
Script talked_to() {
    Script s;
    s.face(kPlayer, kOtherActor, 10).talk(kNarrator, 140).end();
    return s;
}

Script chat_shared() {
    Script s;
    s.talk(kNarrator, 141).if_var(kTalkPromote, Cmp::Ne, 1, "end").talk(kNarrator, 116).label("end").end();
    return s;
}

// Connect now (event 90005, game/src/coop_connect.cpp): event 271's (the communications room's) own steps without
// the room: query 0x2A runs the room's process (its menus answered by coop_connect) and leaves 1 in t0 once
// connected; then, as 271 does, to the dorm by the Gateway (map 14), which loads the host's town for a guest and the
// session's rules for everyone.
Script connect() {
    Script s;
    s.if_var(kQueryResult, Cmp::Eq, kConnectBegin, "begin")
        .label("begin")
        .raw({0x53, 8, 0x2A, 0, 0, 0, 0, 0})  // query 0x2A (0)
        .if_var(kQueryResult, Cmp::Eq, kConnectEnd, "end mark")
        .label("end mark")
        .if_var(kQueryResult, Cmp::Eq, 0, "end")
        .raw({0x2D, 12, 14, 0xFF, 0x80, 0x07, 0x00, 0x03, 0x80, 0x00, 0xFF, 0x00})  // warp_map 14 (1920, 768, 128), as 271
        .label("end")
        .end();
    return s;
}

std::vector<Script> events() {
    Script idle;  // block 0 is not used: it ends at once
    idle.end();
    Script demo;
    demo.talk(kNarrator, 0)
        .wait(10)
        .talk(kNarrator, 1)
        .if_var(kMenuChoice, Cmp::Eq, 0, "yes")
        .set_local(kDemoAnswer, 2)
        .go("read back")
        .label("yes")
        .set_local(kDemoAnswer, 1)
        .label("read back")
        .wait(10)
        .if_var(kDemoAnswer, Cmp::Eq, 1, "said yes")
        .talk(kNarrator, 6)
        .go("done")
        .label("said yes")
        .talk(kNarrator, 5)
        .label("done")
        .end();
    return {idle, demo, player_talk(), talked_to(), chat_shared(), connect()};
}

}  // namespace

Script& Script::label(const std::string& name) {
    labels_.emplace_back(name, commands_.size());
    return *this;
}

Script& Script::talk(int16_t speaker, uint16_t msg, int bank, bool wait) {
    // +2 speaker, +4 bank, +5 sub-bank (0), +6 line, +8 don't wait for the box to close; the rest as the game's own
    std::vector<uint8_t> b = {0x30, 12, 0, 0, uint8_t(bank), 0, 0, 0, uint8_t(wait ? 0 : 1), 0xFF, 0xFF, 0xFF};
    put16(b, 2, uint16_t(speaker));
    put16(b, 6, msg);
    commands_.push_back({b});
    return *this;
}

Script& Script::wait(uint16_t frames) {
    std::vector<uint8_t> b = {0x27, 4, 0, 0};
    put16(b, 2, frames);
    commands_.push_back({b});
    return *this;
}

Script& Script::if_var(uint32_t var, Cmp op, int32_t value, const std::string& to) {
    std::vector<uint8_t> b(16, 0);
    b[0] = 0x38, b[1] = 16, b[2] = uint8_t(op);
    put32(b, 4, var);
    put32(b, 8, uint32_t(value));
    commands_.push_back({b, to, 12});
    return *this;
}

Script& Script::go(const std::string& to) {
    commands_.push_back({{0x37, 8, 0, 0, 0, 0, 0, 0}, to, 4});
    return *this;
}

Script& Script::set_local(uint32_t var, uint8_t value) {
    std::vector<uint8_t> b = {0x3D, 8, 0, 0, value, 0, 0, 0};
    put16(b, 2, var - 10000);
    commands_.push_back({b});
    return *this;
}

Script& Script::add_local(uint32_t var, int16_t delta) {
    std::vector<uint8_t> b = {0x3E, 8, 0, 0, 0, 0, 0, 0};
    put16(b, 2, var - 10000);
    put16(b, 4, uint16_t(delta));
    commands_.push_back({b});
    return *this;
}

Script& Script::face(int16_t actor, int16_t target, uint16_t frames) {
    std::vector<uint8_t> b = {0x14, 8, 0, 0, 0, 0, 0, 0};
    put16(b, 2, uint16_t(actor));
    put16(b, 4, uint16_t(target));
    put16(b, 6, frames);
    commands_.push_back({b});
    return *this;
}

Script& Script::end() {
    commands_.push_back({{0x42, 4, 0, 0}});
    return *this;
}

Script& Script::raw(std::initializer_list<uint8_t> bytes) {
    commands_.push_back({std::vector<uint8_t>(bytes)});
    return *this;
}

std::vector<uint8_t> Script::build() const {
    std::vector<uint8_t> out;
    for (const Command& c : commands_) {
        std::vector<uint8_t> b = c.bytes;
        if (!c.jump.empty()) {
            size_t target = SIZE_MAX;
            for (const auto& [name, at] : labels_)
                if (name == c.jump) target = at;
            if (target == SIZE_MAX) std::fprintf(stderr, "coop content: no label '%s'\n", c.jump.c_str());
            put32(b, c.jump_at, uint32_t(target == SIZE_MAX ? commands_.size() - 1 : target));
        }
        out.insert(out.end(), b.begin(), b.end());
    }
    return out;
}

std::vector<std::string> choice(std::initializer_list<std::pair<const char*, int>> options) {
    std::vector<std::string> pages = {"$055;"};
    for (const auto& [label, next] : options) pages.push_back(std::string(label) + "$019'" + std::to_string(next) + ";");
    return pages;
}

const std::vector<uint8_t>* virtual_file(const char* path) {
    static const std::vector<uint8_t> event = event_file(events());
    static const std::vector<uint8_t> text = text_bank(lines());
    char name[64];
    std::snprintf(name, sizeof name, "/script/event/%03d.bin", kEventFile);
    if (std::strcmp(path, name) == 0) return &event;
    // "/talkfigs/<language>/data/<bank>": the same lines in every language's folder
    std::snprintf(name, sizeof name, "/data/%04d", kTextBank);
    const size_t n = std::strlen(path), m = std::strlen(name);
    if (std::strncmp(path, "/talkfigs/", 10) == 0 && n > m && std::strcmp(path + n - m, name) == 0) return &text;
    return nullptr;
}

}  // namespace mq::content
