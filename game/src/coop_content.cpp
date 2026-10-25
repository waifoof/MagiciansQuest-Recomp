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
std::vector<Line> lines(uint16_t given, uint16_t received) {
    const std::string gave = "$507'" + std::to_string(given) + ";", got = "$507'" + std::to_string(received) + ";";
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
        {102, choice({{"Chat", -1}, {"Give an item", -1}, {"How are we getting along?", -1}, {"Never mind", -1}})},
        // (-1: the box closes)
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

        // Best friends and couples: their own menu (Hang out, and a gentle way back to friends).
        {103, {"What would you like to do with\n$503'641;?", "$019'104;"}},
        {104, choice({{"Chat", -1}, {"Hang out", -1}, {"Give an item", -1}, {"Talk about us", -1}, {"Never mind", -1}})},
        // (the game's menus have at most five options: the last two of the first version under "Talk about us")
        {105, {"What would you like to talk\nabout?", "$019'106;"}},
        {106, choice({{"How are we getting along?", -1}, {"Go back to being friends", -1}, {"Never mind", -1}})},
        // The other console's side (event 90004, t155).
        {142, {"You spent some time with\n$503'641;. That was fun!"}},
        {143, {"You spent a lovely time with\n$503'641;."}},
        {144, {"$503'641; would like to go back to\nbeing friends.", "That's okay. You two will always\nbe friends."}},
        {145, {"$503'641; cast the Friendship\nincantation with you!", "You feel a little closer to\n$503'641;."}},
        {146, {"$503'641; cast the Declare love\nincantation with you!", "Your heart skips a beat."}},
        // The caster's side (event 90008).
        {147, {"Your Friendship incantation\nreached $503'641;!", "You feel a little closer to\n$503'641;."}},
        {148, {"Your Declare love incantation\nreached $503'641;!", "$503'641; is blushing a little."}},

        // An invitation to Lovers Point (as a classmate's, event 700), offered once the pair is close enough.
        {150, {"There's something you've been\nwanting to ask $503'641;...", "$019'151;"}},
        {151, choice({{"Meet at Lovers Point?", -1}, {"Not today", -1}})},
        {152, {"When should you meet?", "$019'153;"}},
        {153, choice({{"10 a.m.", -1}, {"12 p.m.", -1}, {"3 p.m.", -1}, {"6 p.m.", -1}, {"8 p.m.", -1}})},
        {154, {"You ask $503'641; to meet you at\nLovers Point at 10 a.m."}},
        {155, {"You ask $503'641; to meet you at\nLovers Point at 12 p.m."}},
        {156, {"You ask $503'641; to meet you at\nLovers Point at 3 p.m."}},
        {157, {"You ask $503'641; to meet you at\nLovers Point at 6 p.m."}},
        {158, {"You ask $503'641; to meet you at\nLovers Point at 8 p.m."}},
        {159, {"Now you just have to wait for an\nanswer..."}},
        // The invited student's side (event 90006).
        {160, {"$503'641; comes over, looking a\nlittle nervous."}},
        {161, {"Um... would you meet me at Lovers\nPoint at 10 a.m.?", "There's something I want to tell\nyou.", "$019'166;"}},
        {162, {"Um... would you meet me at Lovers\nPoint at 12 p.m.?", "There's something I want to tell\nyou.", "$019'166;"}},
        {163, {"Um... would you meet me at Lovers\nPoint at 3 p.m.?", "There's something I want to tell\nyou.", "$019'166;"}},
        {164, {"Um... would you meet me at Lovers\nPoint at 6 p.m.?", "There's something I want to tell\nyou.", "$019'166;"}},
        {165, {"Um... would you meet me at Lovers\nPoint at 8 p.m.?", "There's something I want to tell\nyou.", "$019'166;"}},
        {166, choice({{"Sure, I'll be there!", -1}, {"Sorry, I can't.", -1}})},
        {167, {"$503'641; looks so happy!\nSee you at Lovers Point."}},
        {168, {"$503'641; nods. \"Okay! Maybe\nanother time, then.\""}},
        // The inviting student hears the answer (event 90007).
        {170, {"$503'641; said yes! You'll meet at\nLovers Point at 10 a.m."}},
        {171, {"$503'641; said yes! You'll meet at\nLovers Point at 12 p.m."}},
        {172, {"$503'641; said yes! You'll meet at\nLovers Point at 3 p.m."}},
        {173, {"$503'641; said yes! You'll meet at\nLovers Point at 6 p.m."}},
        {174, {"$503'641; said yes! You'll meet at\nLovers Point at 8 p.m."}},
        {175, {"$503'641; can't make it this time.\nMaybe another day."}},
        // "How are we getting along?" while a meeting is planned.
        {180, {"You're meeting at Lovers Point\nat 10 a.m."}},
        {181, {"You're meeting at Lovers Point\nat 12 p.m."}},
        {182, {"You're meeting at Lovers Point\nat 3 p.m."}},
        {183, {"You're meeting at Lovers Point\nat 6 p.m."}},
        {184, {"You're meeting at Lovers Point\nat 8 p.m."}},
        // Back to being friends (the talking student's side).
        {190, {"Do you really want to go back to\nbeing friends with $503'641;?", "$019'191;"}},
        {191, choice({{"Yes", -1}, {"No", -1}})},
        {192, {"You talk it over with $503'641;,\ngently.", "You'll always be friends."}},
        {193, {"You decide things are just fine\nthe way they are."}},

        // Hanging out: best friends...
        {200, {"You and $503'641; wander around\ntown, finding funny-shaped clouds."}},
        {201, {"You and $503'641; race each other\nacross the square. It's a tie!"}},
        {202, {"You and $503'641; sit by the water\nand swap snacks."}},
        {203, {"You and $503'641; practice your\nwand twirls together."}},
        {204, {"You and $503'641; are closer than\never!"}},
        {205, {"You've had a great time with\n$503'641; today."}},
        // ... and couples.
        {210, {"You and $503'641; take a slow walk\ntogether, side by side."}},
        {211, {"$503'641; picks a little flower\nand tucks it in your hat."}},
        {212, {"You and $503'641; watch the clouds\ndrift by. It's nice and quiet."}},
        {213, {"$503'641; holds your hand all the\nway around the square."}},
        {214, {"Your heart feels warm."}},

        // Lovers Point. The one who invited (event 90009)...
        {300, {"$503'641; came to Lovers Point,\nright on time."}},
        {301, {"A cool breeze blows, and the whole\ntown sparkles down below."}},
        {302, {"You take a deep breath and tell\n$503'641; how you feel."}},
        {303, {"You tell $503'641; you'd love to\nbe best friends."}},
        {304, {"$503'641; is thinking it over..."}},
        {310, {"$503'641; smiles and says yes!"}},
        {311, {"You and $503'641; are a couple now!"}},
        {312, {"$503'641; grins. \"Best friends\nforever!\""}},
        {313, {"You and $503'641; are best friends\nnow!"}},
        {314, {"$503'641; would like a little more\ntime. That's okay!"}},
        {315, {"You're still great friends."}},
        // ... and the one who was invited (event 90010): the other student speaks.
        {320, {"$503'641; is waiting for you at\nLovers Point."}},
        {321, {"Thanks for coming, $500;.", "I really like spending time with\nyou...", "Would you go out with me?", "$019'323;"}},
        {322, {"Thanks for coming, $500;!", "You're my favorite person to hang\nout with.", "Will you be my best friend?", "$019'324;"}},
        {323, choice({{"Let's be a couple!", -1}, {"Let's be best friends!", -1}, {"Can we wait a little?", -1}})},
        {324, choice({{"Best friends!", -1}, {"Can we wait a little?", -1}})},
        {330, {"$503'641; beams at you."}},
        {331, {"You and $503'641; share a big grin."}},
        {332, {"$503'641; nods. \"Okay. I'll wait\nas long as you need.\""}},

        // Giving an item (picked from the bag, as the game's own "Give item" to a classmate, event 705). The
        // giver's side (event 90002)...
        {400, {"Maybe another time, then."}},
        {401, {"$503'641;'s bag is full, so you\nkeep it for now."}},
        {402, {"$503'641; doesn't seem to hear\nyou. Maybe later."}},
        {404, {"You gave the " + gave + "\nto $503'641;!"}},
        {405, {"$503'641; couldn't take it after\nall. It's back in your bag."}},
        {406, {"$503'641; looks really happy!"}},
        {407, {"Your friend left in a hurry.", "You'll sort out the gift the next\ntime you meet."}},
        // ... and the other student's (event 90012).
        {410, {"$503'641; gave you the\n" + got + "!"}},
        {411, {"You put it in your bag."}},
        // After a failed session gave this console an earlier copy of its student's things (event 90013).
        {412, {"Your friend still has the\n" + gave + " you gave."}},
        {413, {"The " + got + " your friend\ngave you is back in your bag."}},
        // A lesson waits in the classroom for the other student (event 90014, game/src/coop_class.cpp).
        {420, {"$503'641; is on the way.\nClass starts when you're both here."}},
    };
}

// Shows one line of a run (base + value - first) for the value of `var` in first..first + count - 1, as `speaker`,
// then goes on after them. `tag` keeps the labels apart.
void talk_by(Script& s, uint32_t var, int first, int count, int16_t speaker, uint16_t base, const std::string& tag) {
    for (int i = 1; i < count; i++) s.if_var(var, Cmp::Eq, first + i, tag + "#" + std::to_string(i));
    s.talk(speaker, base).go(tag + " done");
    for (int i = 1; i < count; i++) {
        s.label(tag + "#" + std::to_string(i)).talk(speaker, uint16_t(base + i));
        if (i + 1 < count) s.go(tag + " done");
    }
    s.label(tag + " done");
}

// The talking student's side (event 90002): the other student greets them, then a menu. "Chat" asks coop_talk to
// raise the pair's values (t145) and shows one of four chats in turn; at most once a day it counts. "How are we
// getting along?" reads the pair's stage, or before they are friends, how far along they are. Once the pair is close
// enough (t149), an invitation to Lovers Point is offered first, as a classmate's (event 700): the time is chosen
// here and the other student answers on their console. Best friends and couples get their own menu: Hang out, and
// a gentle way back to being friends.
Script player_talk() {
    Script s;
    s.face(kPlayer, kOtherActor, 10)
        .talk(kOtherActor, 100)
        .if_var(kTalkOffer, Cmp::Eq, 0, "menu")
        // The invitation
        .talk(kNarrator, 150)
        .if_var(kMenuChoice, Cmp::Ne, 0, "menu")
        .talk(kNarrator, 152);
    for (int i = 0; i < 5; i++) s.if_var(kMenuChoice, Cmp::Eq, i, "time " + std::to_string(i + 1));
    for (int i = 1; i <= 5; i++) s.label("time " + std::to_string(i)).set_local(kTalkTime, uint8_t(i)).go("invite");
    s.label("invite").set_local(kTalkChat, kActInvite);
    talk_by(s, kTalkTime, 1, 5, kNarrator, 154, "asked");
    s.talk(kNarrator, 159)
        .go("end")
        // The menu
        .label("menu")
        .if_var(kTalkStage, Cmp::Ge, 3, "close menu")
        .talk(kNarrator, 101)
        .if_var(kMenuChoice, Cmp::Eq, 0, "chat")
        .if_var(kMenuChoice, Cmp::Eq, 1, "give")
        .if_var(kMenuChoice, Cmp::Eq, 2, "along")
        .go("never")
        .label("close menu")
        .talk(kNarrator, 103)
        .if_var(kMenuChoice, Cmp::Eq, 0, "chat")
        .if_var(kMenuChoice, Cmp::Eq, 1, "hang")
        .if_var(kMenuChoice, Cmp::Eq, 2, "give")
        .if_var(kMenuChoice, Cmp::Ne, 3, "never")
        .talk(kNarrator, 105)
        .if_var(kMenuChoice, Cmp::Eq, 0, "along")
        .if_var(kMenuChoice, Cmp::Eq, 1, "apart")
        .label("never")
        .talk(kNarrator, 130)
        .go("end")
        // Chat
        .label("chat")
        .if_var(kTalkToday, Cmp::Eq, 1, "chat lines")
        .set_local(kTalkChat, kActChat)
        .label("chat lines");
    talk_by(s, kTalkTopic, 0, 4, kNarrator, 110, "topic");
    s.if_var(kTalkToday, Cmp::Eq, 1, "already")
        .talk(kNarrator, 114)
        .if_var(kTalkPromote, Cmp::Ne, 1, "end")
        .talk(kNarrator, 116)
        .go("end")
        .label("already")
        .talk(kNarrator, 115)
        .go("end")
        // Hang out (best friends and couples)
        .label("hang")
        .if_var(kTalkHung, Cmp::Eq, 1, "hang lines")
        .set_local(kTalkChat, kActHangOut)
        .label("hang lines")
        .if_var(kTalkStage, Cmp::Eq, 4, "couple hang");
    talk_by(s, kTalkTopic, 0, 4, kNarrator, 200, "friends hang");
    s.go("after hang").label("couple hang");
    talk_by(s, kTalkTopic, 0, 4, kNarrator, 210, "couple hang");
    s.label("after hang")
        .if_var(kTalkHung, Cmp::Eq, 1, "hung")
        .if_var(kTalkStage, Cmp::Eq, 4, "warm")
        .talk(kNarrator, 204)
        .go("end")
        .label("warm")
        .talk(kNarrator, 214)
        .go("end")
        .label("hung")
        .talk(kNarrator, 205)
        .go("end")
        // Back to being friends (the game's breakup, kind 11), with a confirm
        .label("apart")
        .talk(kNarrator, 190)
        .if_var(kMenuChoice, Cmp::Ne, 0, "keep")
        .set_local(kTalkChat, kActBreakUp)
        .talk(kNarrator, 192)
        .go("end")
        .label("keep")
        .talk(kNarrator, 193)
        .go("end")
        // Give an item, as the game's own "Give item" to a classmate (event 705 block 1): the bag opens to pick
        // the item (query 0x03, which waits until it closes; the pick goes to the student's hand), then it is held
        // out (0x04, 0x05). coop_talk checks it and asks the other console for room first (t158); only then does
        // the game's hand-over run (query 0x12 with 1, as 705 after a classmate takes a gift), and coop_talk takes
        // the item from the hand and sends it; the other console puts it in its bag and says so, or it comes back.
        // Whenever nothing is given, the item goes back into the bag as 705 does it (query 0x12 with 0).
        .label("give")
        .set_local(10108, 1)
        .raw({0x53, 8, 0x03, 0, 0, 0, 0, 0})  // query 0x03: the bag, to pick an item
        .raw({0x53, 8, 0x04, 0, 1, 0, 0, 0})  // query 0x04 (1)
        .raw({0x53, 8, 0x05, 0, 0, 0, 0, 0})  // query 0x05: the student holds it out
        .set_local(10108, 0)
        .set_local(kTalkGift, 0)
        .set_local(kTalkChat, kActGiveAsk)
        .label("give wait")
        .if_var(kTalkGift, Cmp::Ne, 0, "give asked")
        .wait(2)
        .go("give wait")
        .label("give asked")
        .if_var(kTalkGift, Cmp::Eq, kGiftReady, "give ready")
        .if_var(kTalkGift, Cmp::Eq, kGiftKept, "give kept")
        .if_var(kTalkGift, Cmp::Eq, kGiftFull, "give full")
        .if_var(kTalkGift, Cmp::Eq, kGiftNoAnswer, "give no answer")
        .talk(kNarrator, 400)
        .go("give end")
        .label("give kept")
        .raw({0x67, 8, 0xF9, 0x03, 0, 1, 0xFF, 0xFF})  // the game's "You cannot give this item away as a present."
        .go("give end")
        .label("give full")
        .talk(kNarrator, 401)
        .go("give end")
        .label("give no answer")
        .talk(kNarrator, 402)
        .go("give end")
        .label("give ready")
        .raw({0x53, 8, 0x12, 0, 1, 0, 0, 0})  // query 0x12 (1): the game's hand-over
        .set_local(kTalkGift, 0)
        .set_local(kTalkChat, kActGiveHand)
        .label("give sent")
        .if_var(kTalkGift, Cmp::Ne, 0, "give answered")
        .wait(2)
        .go("give sent")
        .label("give answered")
        .if_var(kTalkGift, Cmp::Eq, kGiftGiven, "given")
        .if_var(kTalkGift, Cmp::Ne, kGiftPending, "give back")
        .talk(kNarrator, 407)
        .go("give end")
        .label("give back")
        .talk(kNarrator, 405)
        .go("give end")
        .label("given")
        .talk(kNarrator, 404)
        .if_var(kTalkToday, Cmp::Eq, 1, "give end")
        .talk(kNarrator, 406)
        .if_var(kTalkPromote, Cmp::Ne, 1, "give end")
        .talk(kNarrator, 116)
        .label("give end")
        .raw({0x53, 8, 0x12, 0, 0, 0, 0, 0})  // query 0x12 (0): anything still in the hand back into the bag
        .wait(15)
        .raw({0x1E, 12, 0, 0, 1, 0, 0xFF, 0xFF, 0, 0xFF, 0, 0})  // the student's motion 1, as 705 ends
        .go("end")
        // How are we getting along?
        .label("along")
        .if_var(kTalkStage, Cmp::Eq, 4, "couple")
        .if_var(kTalkStage, Cmp::Eq, 3, "best friends")
        .if_var(kTalkStage, Cmp::Eq, 2, "friends")
        .if_var(kTalkBand, Cmp::Eq, 0, "band 0")
        .if_var(kTalkBand, Cmp::Eq, 1, "band 1")
        .talk(kNarrator, 122)
        .go("meeting")
        .label("band 0")
        .talk(kNarrator, 120)
        .go("meeting")
        .label("band 1")
        .talk(kNarrator, 121)
        .go("meeting")
        .label("friends")
        .if_var(kTalkBand, Cmp::Ge, 3, "close friends")
        .talk(kNarrator, 123)
        .go("meeting")
        .label("close friends")
        .talk(kNarrator, 126)
        .go("meeting")
        .label("best friends")
        .talk(kNarrator, 124)
        .go("meeting")
        .label("couple")
        .talk(kNarrator, 125)
        .label("meeting")
        .if_var(kTalkMeeting, Cmp::Ne, 1, "end");
    talk_by(s, kTalkTime, 1, 5, kNarrator, 180, "planned");
    s.label("end").end();
    return s;
}

// The other student's side (events 90003 and 90004): they turn to the one who came over, and later share what
// happened (t155: a chat, a hang-out, going back to friends, an incantation).
Script talked_to() {
    Script s;
    s.face(kPlayer, kOtherActor, 10).talk(kNarrator, 140).end();
    return s;
}

Script chat_shared() {
    Script s;
    s.if_var(kTalkShow, Cmp::Eq, kShowHangOut, "hang")
        .if_var(kTalkShow, Cmp::Eq, kShowBreakUp, "apart")
        .if_var(kTalkShow, Cmp::Eq, kShowFriendship, "friendship")
        .if_var(kTalkShow, Cmp::Eq, kShowLove, "love")
        .talk(kNarrator, 141)
        .go("promote")
        .label("hang")
        .if_var(kTalkStage, Cmp::Eq, 4, "couple hang")
        .talk(kNarrator, 142)
        .go("end")
        .label("couple hang")
        .talk(kNarrator, 143)
        .go("end")
        .label("apart")
        .talk(kNarrator, 144)
        .go("end")
        .label("friendship")
        .talk(kNarrator, 145)
        .go("promote")
        .label("love")
        .talk(kNarrator, 146)
        .label("promote")
        .if_var(kTalkPromote, Cmp::Ne, 1, "end")
        .talk(kNarrator, 116)
        .label("end")
        .end();
    return s;
}

// The invited student's side (event 90006): the other student asks them to Lovers Point at t150; Yes or No goes back
// to the one who asked (t145).
Script invited() {
    Script s;
    s.face(kPlayer, kOtherActor, 10).talk(kNarrator, 160);
    talk_by(s, kTalkTime, 1, 5, kOtherActor, 161, "ask");
    s.if_var(kMenuChoice, Cmp::Ne, 0, "no")
        .set_local(kTalkChat, kActInviteYes)
        .talk(kNarrator, 167)
        .go("end")
        .label("no")
        .set_local(kTalkChat, kActInviteNo)
        .talk(kNarrator, 168)
        .label("end")
        .end();
    return s;
}

// The inviting student hears the answer (event 90007, t151: 1 yes, 2 no).
Script invite_answer() {
    Script s;
    s.if_var(kTalkAnswer, Cmp::Ne, 1, "no");
    talk_by(s, kTalkTime, 1, 5, kNarrator, 170, "yes");
    s.go("end").label("no").talk(kNarrator, 175).label("end").end();
    return s;
}

// Friendship or Declare love reached the other student: the caster's side (event 90008, t155).
Script cast_result() {
    Script s;
    s.if_var(kTalkShow, Cmp::Eq, kShowLove, "love")
        .talk(kNarrator, 147)
        .go("promote")
        .label("love")
        .talk(kNarrator, 148)
        .label("promote")
        .if_var(kTalkPromote, Cmp::Ne, 1, "end")
        .talk(kNarrator, 116)
        .label("end")
        .end();
    return s;
}

// The result both students see at the end of the meeting (t151: 1 couple, 2 best friends, 3 not yet).
void meeting_result(Script& s, uint16_t couple_line, uint16_t friends_line, uint16_t wait_line) {
    s.if_var(kTalkAnswer, Cmp::Eq, 1, "couple")
        .if_var(kTalkAnswer, Cmp::Eq, 2, "best")
        .talk(kNarrator, wait_line)
        .talk(kNarrator, 315)
        .go("end")
        .label("couple")
        .talk(kNarrator, couple_line)
        .talk(kNarrator, 311)
        .go("end")
        .label("best")
        .talk(kNarrator, friends_line)
        .talk(kNarrator, 313)
        .label("end")
        .end();
}

// Lovers Point, the one who invited (event 90009): they say how they feel (t149: 2 the pair could be a couple, 1
// best friends) and wait for the other student's choice, made on the other console (t151, set by coop_talk when it
// arrives).
Script meet_inviter() {
    Script s;
    s.face(kPlayer, kOtherActor, 20)
        .talk(kNarrator, 300)
        .talk(kNarrator, 301)
        .if_var(kTalkOffer, Cmp::Eq, 2, "love")
        .talk(kNarrator, 303)
        .go("wait")
        .label("love")
        .talk(kNarrator, 302)
        .label("wait")
        .talk(kNarrator, 304)
        .label("loop")
        .if_var(kTalkAnswer, Cmp::Ne, 0, "answered")
        .wait(10)
        .go("loop")
        .label("answered");
    meeting_result(s, 310, 312, 314);
    return s;
}

// Lovers Point, the one who was invited (event 90010): the other student speaks, and this student chooses: a couple,
// best friends, or not yet (the players choose, not the game). The choice goes to the other console (t145).
Script meet_invited() {
    Script s;
    s.face(kPlayer, kOtherActor, 20)
        .talk(kNarrator, 320)
        .if_var(kTalkOffer, Cmp::Eq, 2, "love")
        .talk(kOtherActor, 322)
        .if_var(kMenuChoice, Cmp::Eq, 0, "pick best")
        .go("pick wait")
        .label("love")
        .talk(kOtherActor, 321)
        .if_var(kMenuChoice, Cmp::Eq, 1, "pick best")
        .if_var(kMenuChoice, Cmp::Eq, 2, "pick wait")
        .set_local(kTalkAnswer, 1)
        .go("chosen")
        .label("pick best")
        .set_local(kTalkAnswer, 2)
        .go("chosen")
        .label("pick wait")
        .set_local(kTalkAnswer, 3)
        .label("chosen")
        .set_local(kTalkChat, kActMeetAnswer);
    meeting_result(s, 330, 331, 332);
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

// Connect now away from the dorm (event 90011): to the dorm by the Gateway first, the same arrival as 271's, where
// the communications room's process (event 90005) is then asked (coop_connect.cpp: the room's code needs the dorm's
// overlay).
Script to_dorm() {
    Script s;
    s.raw({0x2D, 12, 14, 0xFF, 0x80, 0x07, 0x00, 0x03, 0x80, 0x00, 0xFF, 0x00})  // warp_map 14 (1920, 768, 128)
        .end();
    return s;
}

// The other student's side of a gift (event 90012): the item is already in this student's bag.
Script gift_received() {
    Script s;
    s.face(kPlayer, kOtherActor, 10)
        .talk(kNarrator, 410)
        .talk(kNarrator, 411)
        .if_var(kTalkPromote, Cmp::Ne, 1, "end")
        .talk(kNarrator, 116)
        .label("end")
        .end();
    return s;
}

// After a failed session (event 90013, coop_talk's check_restored; t158: 1 a given item taken again, 2 a received
// one put back). Outside a session: no name to show.
Script gift_kept() {
    Script s;
    s.if_var(kTalkGift, Cmp::Eq, 2, "received")
        .talk(kNarrator, 412)
        .go("end")
        .label("received")
        .talk(kNarrator, 413)
        .label("end")
        .end();
    return s;
}

// A lesson waits for the other student (event 90014; t141 holds their actor).
Script class_wait() {
    Script s;
    s.talk(kNarrator, 420).end();
    return s;
}

// The waiting lesson begins (event 90015): into the classroom again at the student's desk, as Principal Sol's event
// 010 block 99 sends the student there, so the lesson's start record starts it as the game does.
Script class_begins() {
    Script s;
    s.raw({0x2D, 12, 0, 0xFF, 0xC0, 0x01, 0x00, 0x00, 0x80, 0x04, 0xFF, 0x00})  // warp_map 0 (448, 0, 1152)
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
    return {idle,      demo,          player_talk(), talked_to(),    chat_shared(), connect(),
            invited(), invite_answer(), cast_result(), meet_inviter(), meet_invited(), to_dorm(),
            gift_received(), gift_kept(), class_wait(), class_begins()};
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

namespace {
uint16_t gift_given = 0, gift_received_item = 0;
}

void set_gift_items(uint16_t given, uint16_t received) { gift_given = given, gift_received_item = received; }

const std::vector<uint8_t>* virtual_file(const char* path) {
    static const std::vector<uint8_t> event = event_file(events());
    // (one bank per pair of gift items, kept: a bank the game has open keeps its bytes)
    static std::map<uint32_t, std::vector<uint8_t>> texts;
    const uint32_t key = uint32_t(gift_given) << 16 | gift_received_item;
    auto it = texts.find(key);
    if (it == texts.end()) it = texts.emplace(key, text_bank(lines(gift_given, gift_received_item))).first;
    const std::vector<uint8_t>& text = it->second;
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
