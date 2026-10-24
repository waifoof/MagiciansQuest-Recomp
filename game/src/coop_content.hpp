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

// Talking to the other player (game/src/coop_talk.cpp fills these before it starts the events below). Bytes 141-148
// are ones no retail script reads or writes, as a variable, an indirect actor or a text tag (all of event/ and
// start/ and the text banks were scanned).
constexpr uint32_t kTalkOther = 10141;     // t141: the other student's actor (110 + their session slot)
constexpr int16_t kOtherActor = 641;       // the actor held in t141 (500 + n: the actor in byte n), for speakers and
                                           // the name tag $503'641;
constexpr uint32_t kTalkStage = 10142;     // t142: the pair's stage (0 none, 2 friends, 3 best friends, 4 couple)
constexpr uint32_t kTalkTopic = 10143;     // t143: which chat (the pair's chats so far, 0-3 in turn)
constexpr uint32_t kTalkToday = 10144;     // t144: 1 if the pair has already chatted today
constexpr uint32_t kTalkChat = 10145;      // t145: set to 1 by the script when "Chat" is picked; coop_talk applies it
constexpr uint32_t kTalkBand = 10146;      // t146: how well they get along, 0-4 (friendship / 25)
constexpr uint32_t kTalkPromote = 10148;   // t148: 1 if this chat makes them friends

// The events of ours (event file 900): started as 900 * 100 + block.
constexpr int kEventPlayerTalk = 90002;    // the talking student's side: a greeting and the menu
constexpr int kEventTalkedTo = 90003;      // the other student's side: "<name> came over to talk with you."
constexpr int kEventChatShared = 90004;    // the other student's side of a chat

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
// A menu: options as (label, the line each goes on to; -1: the box closes there, as the game's own menus do). The
// pick is in kMenuChoice when the text box closes.
std::vector<std::string> choice(std::initializer_list<std::pair<const char*, int>> options);

// The file the game opens at `path`, if it is one of ours (else null). Built once, the same on every PC.
const std::vector<uint8_t>* virtual_file(const char* path);

}  // namespace mq::content
