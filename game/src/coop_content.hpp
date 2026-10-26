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

// Talking to the other player (game/src/coop_talk.cpp fills these before it starts the events below). Bytes 141-146, 148
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
// The pair's milestones (bytes 149-151, 153, 155, 156: also unused by every retail script, start record and text).
constexpr uint32_t kTalkOffer = 10149;     // t149: an invitation to Lovers Point is on offer (1 best friends, 2 the
                                           // pair could be a couple too); in the meeting, which of the two it is
constexpr uint32_t kTalkTime = 10150;      // t150: the meeting's time (1-5: 10 a.m., 12 p.m., 3 p.m., 6 p.m., 8 p.m.)
constexpr uint32_t kTalkAnswer = 10151;    // t151: the other's answer (an invitation: 1 yes, 2 no; the meeting:
                                           // 1 couple, 2 best friends, 3 not yet; 0 while waiting)
constexpr uint32_t kTalkHung = 10153;      // t153: 1 if the pair has already hung out today
constexpr uint32_t kTalkShow = 10155;      // t155: what the other console's event shows (a Show)
constexpr uint32_t kTalkMeeting = 10156;   // t156: 1 if the pair has agreed to meet at Lovers Point
// Giving an item (byte 158: unused by every retail script, start record and text; 157 is a name tag in bank 32).
constexpr uint32_t kTalkGift = 10158;      // t158: how a gift went (a Gift; 0 while coop_talk is still finding out)
enum Gift : uint8_t {
    kGiftNothing = 1,    // nothing picked (the bag was closed without choosing)
    kGiftKept = 2,       // an item the game won't let anyone give away (event 705's list)
    kGiftFull = 3,       // the other student's bag is full (asked before anything was taken)
    kGiftGiven = 4,      // the other console put it in their bag
    kGiftBack = 5,       // the other console couldn't: it is back in the giver's bag
    kGiftNoAnswer = 6,   // no answer from the other console (nothing was taken)
    kGiftReady = 7,      // there is room: the script hands the item over (the game's query 0x12), then asks again
    kGiftPending = 8,    // the other student left before confirming: the gift stays pending until they meet again
};
// What a script asks of coop_talk through t145 (kTalkChat), applied at the end of the frame.
enum Action : uint8_t {
    kActChat = 1,         // a chat (raises the pair once a day)
    kActHangOut = 2,      // hanging out (best friends and couples, once a day)
    kActInvite = 3,       // an invitation to Lovers Point at t150
    kActBreakUp = 4,      // back to being friends (the game's breakup, kind 11)
    kActInviteYes = 5,    // the invited student said yes
    kActInviteNo = 6,     // ... or no
    kActMeetAnswer = 7,   // the asked student's choice at Lovers Point (t151)
    kActGiveAsk = 8,      // "Give an item": check the picked item and ask the other console for room (answer in t158)
    kActGiveHand = 9,     // the item was handed over: take it from the hand and send it (answer in t158)
};
// What the other console is shown (t155, events 90004 and 90008).
enum Show : uint8_t {
    kShowChat = 1, kShowHangOut = 2, kShowBreakUp = 3, kShowFriendship = 4, kShowLove = 5,
};

// The events of ours (event file 900): started as 900 * 100 + block.
constexpr int kEventPlayerTalk = 90002;    // the talking student's side: a greeting and the menu
constexpr int kEventTalkedTo = 90003;      // the other student's side: "<name> came over to talk with you."
constexpr int kEventChatShared = 90004;    // the other student's side of a chat, a hang-out, a breakup, a spell
constexpr int kEventConnect = 90005;       // Connect now (game/src/coop_connect.cpp): the communications room's
                                           // process in the dorm, then to the dorm as event 271
constexpr int kEventInvited = 90006;       // the invited student's side: the invitation and a Yes/No
constexpr int kEventInviteAnswer = 90007;  // the inviting student hears the answer
constexpr int kEventCastResult = 90008;    // Friendship or Declare love reached the other student (the caster's side)
constexpr int kEventMeetInviter = 90009;   // Lovers Point: the one who invited confesses and waits for the answer
constexpr int kEventMeetInvited = 90010;   // Lovers Point: the one who was invited hears it and chooses
constexpr int kEventToDorm = 90011;        // Connect now from elsewhere: first to the dorm by the Gateway, where the
                                           // communications room's code can run (coop_connect.cpp)
constexpr int kEventGiftReceived = 90012;  // the other student's side of a gift: "<name> gave you the <item>!"
constexpr int kEventGiftKept = 90013;      // after a failed session: a gift that went through stays where it went
constexpr int kEventClassWait = 90014;     // a lesson waits for the other student (game/src/coop_class.cpp)
constexpr int kEventClassBegins = 90015;   // ... and begins: into the classroom again, as Principal Sol's event does
constexpr int kEventMystAsk = 90016;       // the other student's mystery scene: "Go and help?" (game/src/coop_myst.cpp;
                                           // the answer in t159: 1 yes, 2 no)
constexpr int kEventMystWarp = 90017;      // ... Yes: to the other student's map (a warp_map coop_myst fills in)
constexpr int kEventMystPlace = 90018;     // ... or, in the same map, a step beside them (actor_set_pos, filled in)
constexpr int kEventMystEntry = 90019;     // ... or into the town at an arrival point (warp_local, filled in)
constexpr int kEventMystOver = 90020;      // ... Yes, but it was over already
constexpr int kEventMagicHere = 90021;    // another student's incantation (game/src/coop_magic.cpp): a sparkle at
                                           // them in this area, then "<name> cast the <incantation> incantation!"
constexpr int kEventMagicAway = 90022;     // ... cast elsewhere: the line only
// The incantations those lines name (t155 = kShowMagic + their index; lines kMagicLine + index).
constexpr const char* kMagicNames[] = {
    "Rainbow",   "Flower rain", "Rainmaking", "Friendship",   "Declare love",  "World Tree", "Phantasm",
    "Candyman",  "Werewolf",    "Homunculus", "Familiar",     "Golem",         "Charm",      "Connect doors",
    "Sweet dreams", "Make peace", "Sit by me", "Honor student", "Mushroom",
};
constexpr uint8_t kShowMagic = 20;
constexpr uint16_t kMagicLine = 440;
// The festival the other student is at (t155 = kShowFestival + its event file - 500, event 90016: 500 the tea party,
// 501 the test of courage, 502 the hotpot cookoff, 503 the rainbow snow festival, 504 New Year's Eve; lines
// kFestivalLine + that; game/src/coop_myst.cpp). 0 there: a mystery's question.
constexpr uint8_t kShowFestival = 40;
constexpr uint16_t kFestivalLine = 460;
// The other student in the auditorium (t155 = kShowHall: the evaluation, + 1 the magic tournament; event 90016, lines
// kHallLine + that, kHallLine + 2 "already finished"; game/src/coop_hall.cpp).
constexpr uint8_t kShowHall = 50;
constexpr uint16_t kHallLine = 490;
// The staff roll a session's evaluation could not play, played at home (game/src/coop_hall.cpp): the scene's music,
// query 0xA0 (overlay 25), 0xA1 until it is over.
constexpr int kEventStaffRoll = 90040;       // (first to the auditorium, where the roll's screens are as in the scene)
constexpr int kEventStaffRollLater = 90041;  // ... in the session: "The staff roll will play once you're home."
constexpr int kEventStaffRollPlay = 90042;   // ... there: the roll, then out to the hall
// Its marks, tests of t0 no value passes (the byte is 0-255), seen by coop_connect where the room's process starts
// and where it has ended.
constexpr int32_t kConnectBegin = 0x4D510001, kConnectEnd = 0x4D510002;
// The mark before the command coop_myst fills in (events 90017, 90018).
constexpr int32_t kMystMark = 0x4D510003;

// The Gateway's destinations in a session (game/src/coop_gate.cpp; events 90030-90034, lines kGateLine..+9). Bytes
// 193 and 194 of the script context: no retail script or start record reads them.
constexpr int kEventGateMenu = 90030;   // "communications room / <destination> / Exit" (t193: 1 Anywhere, 2 The other
                                        // world); the pick in t194 (1 the room, 2 the destination, 3 Exit)
constexpr int kEventGateAsk = 90031;    // the other student: "Go with <name>?" (t193 1: "Go back with <name>?"); t194
                                        // 1 yes, 2 no
constexpr int kEventGateWarp = 90032;   // ... taken there: warp_map (coop_gate fills it in)
constexpr int kEventGateEntry = 90033;  // ... or into the town at an arrival point (warp_local)
constexpr int kEventGateNote = 90034;   // t193 0: "<name> didn't go through after all."; 1: "You go back with <name>."
constexpr uint32_t kGateKind = 10193, kGateAnswer = 10194;
constexpr uint16_t kGateLine = 470;
constexpr int32_t kGateMark = 0x4D510004;  // the mark before the warp coop_gate fills in

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

// The file the game opens at `path`, if it is one of ours (else null). Built once, the same on every PC (the text
// bank once per pair of gift items below; a file already open keeps its bytes).
const std::vector<uint8_t>* virtual_file(const char* path);

// The items the gift lines name (the game's tag $507'<item>; takes a number, not a variable): `given` in the giver's
// lines, `received` in the receiver's. Set before the event that shows them starts.
void set_gift_items(uint16_t given, uint16_t received);

}  // namespace mq::content
