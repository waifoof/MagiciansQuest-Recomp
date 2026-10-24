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
    };
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
    return {idle, demo};
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
