// Magician's Quest: event scripts and text lines of our own, served through the game's own loaders (docs/
// COOP_RELATIONSHIPS.md, "Content of our own"). New scenes are written in the game's script language with the
// assembler below and their lines in a text bank of ours; the game loads both as if they were on the card.
#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace mq::content {

// The event file and the text bank no retail data uses. An event is started as file * 100 + block (e.g. 90001).
constexpr int kEventFile = 900;
constexpr int kTextBank = 60;

// Script variables (event commands 0x36/0x38 read them; docs/COOP.md). 10000 + n is the script context's byte n.
constexpr uint32_t kQueryResult = 10000;  // t0: the result of query 0x53, has_item 0x63, random 0x70
constexpr uint32_t kMenuChoice = 10100;   // t100: the option picked in the last menu ($055;), 0 for the first
constexpr uint32_t kDemoAnswer = 10140;   // t140: the demo's answer (1 yes, 2 no); no retail script or code uses it

// Actors (event commands' speaker and actor fields; ov001 0x020C8814).
constexpr int16_t kPlayer = 0;
constexpr int16_t kNarrator = 250;  // a text box with no one speaking (retail notes, announcements)
constexpr int16_t kOtherPlayer0 = 110;  // the session's other players are 110 + their slot

// Comparisons of 0x36/0x38 (the jump is taken when the comparison holds).
enum class Cmp : uint8_t { Eq = 0, Ne, Gt, Lt, Ge, Le, Or, And };

// One event block: commands `op u8, size u8, arguments`, jumps by command index (resolved from named labels).
class Script {
public:
    Script& label(const std::string& name);
    Script& talk(int16_t speaker, uint16_t msg, int bank = kTextBank, bool wait = true);  // 0x30: a text box
    Script& wait(uint16_t frames);                                                       // 0x27
    Script& if_var(uint32_t var, Cmp op, int32_t value, const std::string& to);          // 0x38
    Script& go(const std::string& to);                                                   // 0x37
    Script& set_local(uint32_t var, uint8_t value);  // 0x3D: a context byte (var 10000 + n)
    Script& add_local(uint32_t var, int16_t delta);  // 0x3E: (kept in 0..255)
    Script& face(int16_t actor, int16_t target, uint16_t frames = 15);  // 0x14
    Script& end();                                                      // 0x42
    Script& raw(std::initializer_list<uint8_t> bytes);  // any other command, size byte included
    std::vector<uint8_t> build() const;

private:
    struct Command {
        std::vector<uint8_t> bytes;
        std::string jump;  // a label whose command index goes at +jump_at
        size_t jump_at = 0;
    };
    std::vector<Command> commands_;
    std::vector<std::pair<std::string, size_t>> labels_;
};

// One line of the text bank: its pages (one text box each; '\n' starts a new line in the box). Tags as the game's
// own text: $500; the player's name, $019'<id>; go on with line <id>, a page "$055;" followed by option pages makes
// a menu (see choice()).
struct Line {
    uint16_t id;
    std::vector<std::string> pages;
};
// A menu: options as (label, the line each goes on to). The pick is in kMenuChoice when the text box closes.
std::vector<std::string> choice(std::initializer_list<std::pair<const char*, int>> options);

// The file the game opens at `path`, if it is one of ours (else null). Built once, the same on every PC.
const std::vector<uint8_t>* virtual_file(const char* path);

}  // namespace mq::content
