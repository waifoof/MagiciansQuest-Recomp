# Player-to-player friendship and romance: research and design

Status: research done (two passes); groundwork built (content of our own, plan step 1); the players talk to each
other, with a pair record of ours that chatting raises (plan step 2 and the start of step 3). [V] = verified in the ROM's data, scripts or code; [I] = inferred.

## How the game models relationships with classmates

- **Rule tables** (`npc/relation.bin`, `npc/consider.bin`, LZ10; loaded by overlay 1 at 0x020F609C / 0x020F6C70) hold
  28-byte rules that drive what classmates decide to do (walk up to the player, invite them) [V].
- **The levels** live in 164-byte classmate records [V]: +0x60+slot stage toward each of 4 player slots, +0x64+slot and
  +0x68+slot two 0..100 values toward that player (probably friendship and affection [I]), +0x6C+j / +0x78+j stage
  and value toward each of the 12 classmates, +0x30 three 8-byte relationship slots (0x020F6780). What fills the 4
  player slots is not traced (possibly the session's player numbers [I]).
- **Event command 0x26** `26 08 <actorA> <actorB> <kind>` sets a relationship (ov028 0x02112849 -> 0x020D1090 ->
  0x020F69C0 / 0x020F6780) [V]. Kinds: 2 friends, 3 best friends, 4 couple, 11 breakup (a value of 100+ drops to 70,
  anything else loses 32).
- **Script variables** (player record): 211/212/213 who invited you to Lovers Point, invite pending, time slot (1..5);
  217/218/219 the same for the best-friend invitation; 203 counts accepted confessions [V].
- **What raises them:** the talk menu (talk, give an item, hang out, go to school), telegrams from the post office,
  incantations (make peace; a love incantation at Lovers Point) [V]. How much each action adds is not traced [I].
- **Milestones:** event 700 (a classmate's invitation and the time choice), event 701 (the meeting at Lovers Point:
  confession -> couple, or -> best friends), letters, newspaper items, breakup choices [V]. The US release splits
  couple (opposite sex) and best friends (same sex).

## Between two human players today

Visits only. There is no player-to-player relationship in the data: classmate records hold values toward 4 player
slots and 12 classmates, nothing between players [V]. Classmates can be sent between towns as visiting or transfer
students.

## Proposal

1. **State of our own**, saved beside the game's save in both players' data (co-op milestone 3): one record per pair
   of students, keyed by both identities (friend code + name): stage (0 none, 2 friends, 3 best friends, 4 couple, the
   game's own kinds), two 0..100 values with the game's breakup rule, a pending invitation (who, time slot),
   counters, last day.
2. **Raise it with the game's own actions**, aimed at the other student: talking (a menu on the other student laid out
   like the classmate talk menu), giving an item, hanging out (the follow state), telegrams with the other player as a
   recipient, the Lovers Point incantation.
3. **Milestones from the game's own events**: forks of 700 (invitation, time choice) and 701 (the meeting, with its
   camera and walks), the other student in place of the classmate, the pair record set instead of command 0x26, the
   other player answering with the game's yes/no box. Couple or best friends chosen by the players, not by gender.
4. **Text**: letters, the newspaper, compatibility readings and system message styles can be reused with names filled
   in; classmates' lines are in their own voices, so lines between players need a new text bank of our own.
5. **Keeping the consoles together**: the game is not lockstep (docs/COOP.md, "How the consoles keep together"):
   each console runs its own student, and the others learn what they need from packets. So a change is decided on
   the console of the student who acts, sent in a packet with the values it wrote, and stored as received by the
   other console; it is a record named by both students, never the local student's record (player -1 is a
   different student on each console). Each player's copy is kept beside their own save.

## Second pass (static reading)

- **Facing the other player and pressing A does nothing** [V]: the talk-target finder (ov001 0x020C85E4) only looks at
  NPCs (field+0x3E48, stride 0x2B0); the other players' objects (field+0x3E4C, 3 x 0x638) are never candidates. Their
  record getter returns 0 and their relationship methods are empty. Items can only be given to classmates (msg 1674).
  Classmates can still walk up to a guest (0x020E4808 includes the other players).
- **Friendship (result id 30) and Declare love (32) between consoles** [V]: query 0xB5 sends packet 0x45, then waits
  up to 30 frames for another player's byte +0xA2 to be 27 or 28 (probably both casting at once [I]). In a session
  event 320 sets s481 = 1 (message 3064) and event 344 sets s482 = 1: only the incantations' learned flags in the
  caster's own record. Their real effect (main 0x02045874, result records 29 and 31) needs a classmate target (t202):
  aimed at the other player nothing is written.
- **The 4 player slots** in classmate records are the 4 students of one town (one save): a student's slot is at its
  record +2 (picked at creation, 0x020106C8). Every relationship write uses the local student's own slot. A guest in
  the host's town writes the host's classmates at its own slot index, on its own console only, lost on leaving [I].
- **Telegrams to another player work** [V]: the post office (event 216 block 5) offers "Other player" (999:1100) in a
  session; sent as packet 0x42 and posted as mail; no relationship change.
- **No** "met" list, player-to-player field or exchange-notes feature.

### Plan (smallest hook set)

The other player is actor 110 + their slot (the resolver ov001 0x020C8814 already maps it to their object); the
pair's state is a record of ours, written the same way on every console.
1. Groundwork: serve event scripts and text lines of our own through the game's loaders (an event file number and a
   text bank no retail data uses), so new scenes are written in the game's own script language. **Built**: see
   "Content of our own" below.
2. Talk to the other player: when the talk-target finder finds no NPC and the other player is in front, start our
   player-talk event (t141 = 110 + slot); the other console shows its side. **Built**: see "Talking to the other
   player" below.
3. New queries (ov028 0x0211343C) to get and set the pair's stage and values, with the game's kinds (2 friends,
   3 best friends, 4 couple, 11 breakup) and its breakup rule; command 0x26 (0x020D1090) writes the pair when an actor
   is a player.
4. Friendship and Declare love aimed at the other player (0xB5 remembers who matched; 0x02045874 applies to the pair).
5. Forks of events 700 and 701 (the invitation, the time, Lovers Point) with the other player in the classmate's
   place; couple or best friends chosen by the players.
6. New text: the talk menu and its answers, the incantations' results between players, invitations, confession,
   answers, breakup (about 40-60 lines).

Open: what fills the 4 player slots; whether a guest's values toward the host's classmates are kept after a visit;
the amounts; whether the "exchange notes" feature mentioned in interviews exists in the US release.

## Content of our own (plan step 1, built)

New scenes are written in the game's own script language and shown in its own text boxes; the game loads them
through its normal loaders as if they were files on the card. Everything is authored in `game/src/coop_content.cpp`
(no game data); the formats are rebuilt around it at start-up.

**How it is served** [V]. Both loaders open their files by path through the NitroSDK: events with
`/script/event/%03d.bin` (main 0x0201D204 builds it; 0x0201D158 loads block N with flags 0xC, container + LZ10),
text with `/talkfigs/<language>/data/%04d` (main 0x0201AC34 picks the language folder; the text box loads the bank's
block id/100 at 0x020157B0 / 0x02015898). Both go through main 0x020047FC (size) and 0x02004620 (the loader thread's
request): FS_OpenFile 0x0208D154, FS_SeekFile 0x0208D24C, FS_ReadFile 0x0208D240, FS_CloseFile 0x0208D180, with the
FSFile's span at +0x24 top, +0x28 bottom, +0x2C position. `mq_fs_open` (game/src/cfie_hooks.cpp) sees a path of ours
and opens a stand-in file instead (`/talk/onemsg_LZ.bin`, a string in main's data), so the SDK has a real open file;
at its first seek or read the span becomes [0, our size), reads are copied from our bytes (`mq_fs_read`, instead of
the card), and the stand-in's span is put back before the SDK closes it (`mq_fs_close`). Any other path is left
alone (only a string compare), so without our content the game runs as it does.

**Numbers of ours**: event file **900** (retail files stop at 804, then 970-999; the loader takes files up to 999
and blocks up to 99) and text bank **60** (retail banks: 0-12, 20-33, 40-53, 998, 999; the
dialogue commands carry the bank in a byte). The same lines are served for every language folder.

**Formats** (as the game's own): a container is u32 count, count x (u32 offset, u32 size), blocks LZ10 (both loaders
ask for unpacking; we write literal-only LZ10). A text bank's block n holds lines n*100..n*100+99: u32 100, 100 x
(offset, size), each line `"<id>\t"` + 16 pages each followed by a tab + `"\r\n"`; a new line within a box is U+25BC.

**Writing a scene** (`game/src/coop_content.cpp`, `lines()` and `events()`; the API is `coop_content.hpp`):

```cpp
// lines of bank 60: pages are text boxes, '\n' a new line in the box, the game's tags work ($500; the player's name)
{1, {"Shall we try one out?", "$019'2;"}},        // $019'<id>; goes on with line <id> in the same box
{2, choice({{"Yes", 3}, {"No", 4}})},              // a menu ($055;); each option goes on with its line
// event 900, block 1 (started as 90001)
Script demo;
demo.talk(kNarrator, 1)                             // 0x30: a text box (speaker, line; bank 60 by default)
    .if_var(kMenuChoice, Cmp::Eq, 0, "yes")         // t100: the option picked in the last menu, 0 the first [V]
    .set_local(kDemoAnswer, 2).go("done")
    .label("yes").set_local(kDemoAnswer, 1)
    .label("done").end();
```

`Script` has `talk`, `wait`, `if_var`, `go`, `set_local`, `add_local`, `face`, `end`, labels for jumps (resolved to
command indexes as 0x36-0x38 want them), and `raw` for any other command (`mqscript.py`'s opcode table). Speakers:
`kNarrator` (250, a box with no one speaking), `kPlayer` (0), the other players 110 + slot (not tried yet). A new
block is one more entry in `events()`, started as 900 * 100 + its index.

**Starting one** (test driver): `MQ_EVENT_FILE=<path>`; writing an event number to the file (e.g. `echo 90001 >
$d/event`) queues it the way the town's triggers do (main 0x0201DAF0: file, block, 0xFF, then start kind 2 at script
context +0x23BC), retried each frame while another event runs. In the game proper our events will be started by the
hooks of steps 2-5 (talking to the other player, the incantations).

**Verified** (one game in the dorm hall, `MQ_EVENT_FILE`): event 90001 showed our first box with the player's name
filled in, the second box, the Yes/No menu, the answer line chosen by the menu, and the read-back line chosen by the
script variable (t140) it had just set; tried with No, then again with Yes. The player walks normally afterwards.

Open: the demo's
variable t140 is a script context byte no retail script or code uses, and whether those bytes are saved is unknown
(only the demo writes it). The pair's own state (step 3) will need a store of ours rather than context bytes.

## Talking to the other player (plan step 2, built)

Walk up to the other student, face them and press A: if no NPC would answer, they greet you (a box titled with
their name) and a menu of ours opens: **Chat**, **How are we getting along?**, **Never mind**. On their console their
student turns to you and a box says "<name> came over to talk with you."; after a chat, "You had a nice chat with
<name>." Everything is in `game/src/coop_talk.cpp` (hooks) and `game/src/coop_content.cpp` (events 90002-90004,
lines 100-141 of bank 60).

**Starting it** [V]. The local student's try at talking is main 0x02028100 (called with A from the free state at
0x020272F8; it asks the finder ov001 0x020C85E4 for an NPC, then enters the talk state). `mq_coop_talk` (instead, only
for that call, only in a session) runs the finder itself; if it finds no NPC, it looks at the other players' objects
(field+0x3E4C, 3 x 0x638; +0x62C the session slot, +0x5A0 set while in this area, +0x2C the position) for one within
2.25 units (the finder's classmate range, ov001 0x020C875C), level, and within 60 degrees of where the student faces
(field+0x38A4; the finder's own angle test, FX_Atan2Idx 0x02086A04 and 0x0200D31C). Then it queues event 90002 as
the town's triggers do (main 0x0201DAF0 + start kind 2) and returns "no NPC" (the A press does nothing else; the
event starts next frame). Without the other student in front, the call runs as the game's. (The finder's own "shown"
test, 0x0201EF48, is always false for a player object: it has no model link at +4.)

**Names** [V]. `$503'641;` prints the other student's name: 641 is "the actor in flag byte 141" (500 + n), which holds
110 + their slot, and the name function (ov001 0x020C89C8, through the resolver 0x020C8814) takes a player's name from
their object (vtable +0x94: +0x500, the session's copy). The title of a box spoken by a player is another matter: the
game takes it from the speaker's classmate record (main 0x0201403C, the object's vtable +0x28), which no player object
has (0x0200FE60 returns 0), so it would read "[?]". Worse, the talk command keeps only the speaker's NPC number in
the box (+0x1474), which is 0 for every player object. `mq_coop_speaker` (instead, in a session) titles a box of
speaker 0 while our talk (event 90002) runs with the other student's name (our scripts give the local student no
lines), and a box of speaker 110 + slot or 446 + slot with that player's, from the session's entry (session+0x49E8 +
slot * 0x50: +0 the name, 24 bytes; +0x18 their student record, the local one or the copy the session received);
any other box is titled as the game does it.

**The other console** [V]. The game is not lockstep (docs/COOP.md), so the talking console tells the other with a
packet of ours through the game's own send (main 0x0202C4CC with the local player): type 0x27, which the session
lets through when its outgoing slot is free or holds a movement packet (ov001 0x020F112C) and which a remote player's
update passes over (its jump table's default). `[2]` kind (1 talk, 2 a chat's result), `[3]` the student it is for,
`[4]` a sequence number (a packet can be seen in more than one frame), `[5..13]` the pair's stage, friendship,
affection, last chat day (u32), chats, "now friends", `[14..15]` "MQ". It is sent from a queue once a frame until the
game takes it. The receiving console takes it out of the sender's object's queue before its update runs
(`mq_coop_remote`, before main 0x02031404) or from the session's last packets for that slot (session+0x4A08/+0x4A18 +
slot * 0x50), queues its side's event (90003, 90004) for when no other event runs, and writes the record it was sent.
Verified with two copies: the guest's side opened while the host's greeting was up (screenshots taken one after the
other show both), and both PCs wrote the same values.

**The pair record**. A record of ours per pair of students, keyed by both identities, written beside each
player's save as `<save>.coop` (`cfie.coop` beside `cfie.sav`; set by the frontend with the save's path, nothing
when the save is read-only): `<student> <student> stage friendship affection last-chat-day chats`, a student being
`<name, hex>-<8 bytes at their student record's +0x0C, hex>` (the name from the session's entry; the 8 bytes look like
a value made when the save was started [I]: two saves copied from one share them, so there the name alone tells
them apart, and two students with the same name and copied saves share a record). Stage uses the game's kinds (0
none, 2 friends, 3 best friends, 4 couple); friendship and affection are 0..100.
- **Chat** raises friendship by 4 and affection by 1 once a day per pair (the day from the game's clock object,
  `*0x020AD704`: +4 year, +8 month, +0xC day); a second chat that day only talks. Four topics in turn (the chat
  count). At friendship 30 a pair becomes friends ("You and <name> are friends now!" on both consoles).
- **How are we getting along?** shows the stage, or before friends how far along they are (friendship / 25).
- The talking console decides and writes; its packet carries the new values, which the other console stores as they
  are. Both copies hold the same values after every chat.

**Flag bytes** t141-t148 (script context +0x2254 + n) carry the values to the scripts: t141 the other student's
actor, t142 stage, t143 topic, t144 "already chatted today", t145 set by the script when Chat is picked (the hook
applies the chat and clears it), t146 friendship / 25, t148 "this chat makes them friends". No retail script uses
these bytes as a variable, an indirect actor or a text tag (all of `script/event`, `script/start` and the text banks
scanned); t154 (event 701's partner) was avoided for that reason.

**Verified** (two day-one copies connected, host and guest outside the dorm, `/home/claude/run/live/talktest.py`):
the host's greeting titled with the guest's name while the guest's screen said the host came over; a chat from a
record at friendship 28 showed the fourth topic, "You feel a little closer", "You and <name> are friends now!" on the
host and "You had a nice chat" and "friends now" on the guest, and both `.coop` files then held stage 2, friendship
32, affection 6, day 48, 4 chats; a second chat the same day said "You've already had a good long chat" and changed
nothing; "How are we getting along?" said "good friends"; "Never mind" said "Maybe later, then." Outside a session
none of the three hooks does anything (each tests the session first).

**Risks** (the game keeps consoles together by packets, so these are about two views of one talk):
- Both students pressing A at each other in the same moment start two talks; each console shows its own menu, then
  the other's "came over" box. If both chat the same day, each console applies one chat to its own copy and sends
  it; the last packet each receives wins, so the two copies can differ by one chat until the next one.
- A record exists only where both consoles were: a PC that missed a packet (the session ended first) keeps its older
  copy; the next chat sends the talking console's values again.
- The day is the talking console's clock: two PCs on different dates count "once a day" by whoever talks.
- Test runs: the two harness saves are one save copied, so both students are "AAAAAA" with the same 8 bytes; the
  test wrote other names into each console's session entry (gdb) to see the names.

Open: telling the listening console's student what was chosen in the menu as it happens (it only hears about chats);
the other actions of the classmates' menu (give an item, hang out); best friends and couple (steps 3-5).
