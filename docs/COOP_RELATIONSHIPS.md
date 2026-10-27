# Player-to-player friendship and romance: research and design

Status: research done (two passes); groundwork built (content of our own, plan step 1); the players talk to each
other, with a pair record of ours that chatting raises (plan step 2); friends, best friends and couples between the
players, through the game's two-player incantations, an invitation to Lovers Point and a confession there (plan steps
3-6, "Friends, best friends, a couple" below). [V] = verified in the ROM's data, scripts or code; [I] = inferred.

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
  up to 30 frames for another player's byte +0xA2 to be 27 or 28 [V]: 27 / 28 is the other student
  joining (a student in wizard form when the packet comes; main 0x02024030 then plays event 320 / 344 block 4 for
  it). One student holds the item, the other has only transformed (docs/TESTING.md, "Casting an incantation by real
  input"). In a session
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

Steps 3-6 were built without new hooks or queries: the pair's values live in our record and reach the scripts as
flag bytes, the scripts ask for changes through a flag byte, the incantations are seen where their events set the
learned flag, and the milestones are events of ours (file 900) rather than forks of 700/701 (whose camera set-up and
classmate actor would need the classmate code paths). See "Friends, best friends, a couple".

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
message of ours on the co-op side channel (type 40, `coop_sync::listen`; docs/COOP.md, "The co-op side channel"):
reliable and in order, unlike the first version's packets (type 0x27 through the game's own 16-byte send, which only
got through when the outgoing slot was free; `mq_coop_remote` now only drops such a packet from an older build). A
message names the student it is for, carries the pair's whole record (stage, both values, chats and hang-outs, the
days they last counted, the invitation and who made it) and what the other console should show. The receiving
console stores the record as it is and queues its side's event (90003, 90004, ...) for when no other event runs.
Verified with two copies: the guest's side opened while the host's greeting was up, and both PCs wrote the same
values after every action.

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
- Both students pressing A at each other: one talk at a time (fixed after the co-op week playtest,
  docs/COOP_PLAYTEST.md, where each A press started a talk and each put "<name> came over to talk with you" in the
  other's queue, so two players pressing A kept answering each other's talks and Select's Disconnect could not
  open). Now, while the other student's talk with this one is on (their message 40 op 1 until op 15 "talk ended",
  sent when their event 90002 ends; at most a minute for a build that never sends it), A in front of them starts no
  talk here; "came over to talk" is shown once (not queued again while it is still to show or showing); and no new
  talk starts within 45 frames of one of ours ending (the A that closed its last box). If both start in the same
  moment (each message on its way while the other pressed), the host's (the lower slot) goes on: the host's console
  leaves the guest's talk unannounced, and the guest's own event 90002 reads t155 = 9 (`kShowYield`) before and
  after its greeting and then says "<name> is talking to you." (line 149) and ends. Verified (same-PC link and the
  internet mode, the driver pressing A on both every 50-60 frames, `/home/claude/run/live/nd.py talkboth`): one talk
  at a time, each followed by the other's single box ("came over", "You had a nice chat"), "came over to talk again
  (already shown)" logged instead of a second box, Select's "Disconnect?" opening as soon as the presses stopped;
  the yield seen over the internet ("slot 0 talked to this student at the same moment: theirs goes on", the guest's
  "<name> is talking to you." after its greeting).
- A record exists only where both consoles were: a PC that missed a packet (the session ended first) keeps its older
  copy; the next chat sends the talking console's values again.
- The day is the talking console's clock: two PCs on different dates count "once a day" by whoever talks.
- Test runs: the two harness saves are one save copied, so both students are "AAAAAA" with the same 8 bytes; the
  test wrote other names into each console's session entry (gdb) to see the names.

Open: telling the listening console's student what was chosen in the menu as it happens (it only hears about the
results). (Giving an item: built, see "Giving an item" below.)

## Friends, best friends, a couple (plan steps 3-6, built)

Everything is in `game/src/coop_talk.cpp` (decisions, messages, the record) and `game/src/coop_content.cpp` (events
90002-90010 and lines 100-332 of bank 60, all written for this: short, warm, all-ages).

**The pair record** (`<save>.coop`, one line per pair): `<student> <student> stage friendship affection
last-chat-day chats last-hang-out-day hang-outs invitation time inviter last-invitation-day`; a line of the first
version (seven fields) still reads. The file is read again when it changes on disk. When two students first stand
in the host's town together in a session, each console sends its copy and the one further along (chats and
hang-outs, then stage, then the values) is kept by both, so a PC that missed something catches up.

**Stages and values** (0..100 each; the stages are the game's own kinds of command 0x26):

| what | rule |
|---|---|
| Chat | friendship +4, affection +1, once a day (as before) |
| Hang out (best friends, couples) | friendship +3, affection +1 (a couple +2), once a day |
| Friendship cast together | friendship + the game's amount (below) |
| Declare love cast together | affection + the game's amount |
| friends (2) | friendship 30, on any rise |
| invitation offered | friends with friendship 60 (best friends), or friends / best friends with friendship 60 and affection 40 (a couple too); not twice the same day |
| best friends (3) / couple (4) | chosen at Lovers Point |
| Go back to being friends | the game's breakup (kind 11) on both values: 100 or more drops to 70, else -32; stage 2 (or 0 under 30) |

**The incantations** [V]. Friendship (result id 30) and Declare love (32) in a session: query 0xB5 sends packet 0x45
and lets the cast through only while another player's action (+0xA2 of their object) is 27 or 28, that is while the
other student joins it (in wizard form when the caster's packet comes; both casting at the same moment fails on
both; the game's own two-player rule: "the two of you must go together..."); events 320 / 344 then only set the learned flag (s481 / s482) with command 0x3B. Their real
effect, the result apply (main 0x02045874), only works through a classmate's record (target t202, from the focused or
nearest NPC, main 0x02046554); a player object has none. So the success itself is the hook: `mq_coop_talk_store`
(from the store write's hook, main 0x0201E070, `mq_var_store`: no new hook) sees command 0x3B (its call returns to
ov028 0x02112EDE) write 1 to s481 / s482 in the local student's store (0x020ADCFA) in a session, and at the end of
the frame applies the game's own rule for a classmate to the pair: result record 29 (Friendship) or 31 (Declare
love), read from the game's table (main 0x02045E0C, 0x020B8E18), gives `rec[+0x11 + stage] * rec[+0x17]` (Friendship:
1..5 x 2) or `rec[+0x11 + stage] * rec[+0x16]` (Declare love: 1, 1, 2, 2, 3 x 2), the stage being the caster's store
field 0x89 / 0x8A (main 0x0201DFC0). The other student is the one whose action is 27 / 28 (else the other student in
this area). The caster sees "Your Friendship incantation reached <name>!" (event 90008), the other "<name> cast the
Friendship incantation with you!" (90004), and the other console adds the same amount to its copy (a message with
the amount, so two casts at once both count: each console adds its own and the other's).

**The invitation** (as a classmate's, event 700). When the pair qualifies, the talk (90002) starts with "There's
something you've been wanting to ask <name>..." and *Meet at Lovers Point?* / *Not today*; then the time (10 a.m.,
12 p.m., 3 p.m., 6 p.m., 8 p.m.: event 700's s213 values 1-5). The other console shows the invitation in the other
student's voice (90006: "Um... would you meet me at Lovers Point at 6 p.m.?") with *Sure, I'll be there!* / *Sorry, I
can't.*; the asker hears the answer (90007). An invitation never answered lapses the next day; a declined one can be
made again the next day. "How are we getting along?" reminds the pair of a planned meeting.

**Lovers Point** [V]. Area 49 (`mc_ss` block 49 starts event 701 there; the bell arch). Event 701's start records
test s212 / s213 and a time window hh:00-hh:59 (bytes +0x24 / +0x26 of the 80-byte record, hour * 100 + minute, ov001 0x020F44E0),
the time being the town block's +0x2CC / +0x2CD (the host's time on every console). Ours: the console of the student
who asked checks each frame; when its student is in area 49 with the other student (their object's +0x5A0), in the
chosen hour, and no event runs, it starts the scene on both consoles: there (90009) "<name> came to Lovers Point,
right on time.", a line about the view, the confession ("You take a deep breath and tell <name> how you feel." or,
for best friends only, "You tell <name> you'd love to be best friends."), then it waits (a loop on t151); on the
other console (90010) the asker speaks in a box titled with their name ("Thanks for coming... Would you go out with
me?" or "Will you be my best friend?") and this student chooses: *Let's be a couple!*, *Let's be best friends!* or
*Can we wait a little?* (only the last two for best friends). The choice sets the stage on that console, goes back
in the message, ends the other's wait, and both see the result ("You and <name> are a couple now!"). If the other
student leaves the session while one waits, the wait ends with nothing changed.

**Hang out** (best friends and couples; their talk menu: *Chat*, *Hang out*, *Give an item*, *Talk about us* (then
*How are we getting along?*, *Go back to being friends*, *Never mind*: the game's menus hold five options), *Never
mind*): four little scenes in turn, a couple's warmer ("<name> picks a little flower and tucks
it in your hat."); the other console hears "You spent a lovely time with <name>." **Go back to being friends** asks
"Do you really want to go back to being friends with <name>?" first; the other console is told gently ("That's okay.
You two will always be friends.").

**Flag bytes** added: t149 the invitation on offer (1 best friends, 2 a couple too; in the meeting, which one), t150
the time (1-5), t151 the other's answer, t153 "hung out today", t155 what the other console shows, t156 a meeting
is planned; t145 now carries the action a script asks for (1 chat, 2 hang out, 3 invite, 4 back to friends, 5/6 yes or
no to an invitation, 7 the choice at Lovers Point). All unused by retail scripts, start records and text (scanned).

**Verified** (two connected copies driven by test scripts outside the repo, with `MQ_WARP_FILE`, the pair record started
high instead of grinding): from friendship 58 / affection 37, a chat (62 / 38, both screens); Declare love cast by the
host (`MQ_COOP_CAST_FILE=32`: event 344's own session branch, query 0xB5 passed because the guest's action read 28)
raised affection to 40 ("Your Declare love incantation reached Bea!" / "AAAAAA cast the Declare love incantation with
you!"); the next talk offered the invitation, the host chose 10 a.m., the guest's screen showed the invitation in the
host's voice and its Yes, the host heard "Bea said yes!"; both warped to area 49 (10:35): the scene started on both
screens, the guest chose *Let's be a couple!*, both read "You and <name> are a couple now!" and both records held
stage 4; *Hang out* as a couple ("Your heart feels warm", +3 / +2, the guest told); *Go back to being friends* with
its confirm (65 / 42 became 33 / 10, stage 2, the guest told gently); the guest inviting the host and the host
declining ("AAAAAA can't make it this time."). After each step both `.coop` files were the same line.

**Risks**:
- Both students acting at once (both talk, or both invite) each decide on their own console; the record sent last
  wins on both. Casts are the exception (amounts, both added).
- The meeting needs the asker's console to see the other student in area 49 in the hour; a pair that never meets
  keeps the invitation (it can be seen in "How are we getting along?"); a new one cannot be made while it stands.
- Two saves copied from one share an identity when the names match too (the test saves: the guest's name was
  written into the host's session entry by the test).
- The amounts and thresholds are ours (the game's amounts for its talk menu were not traced); the incantations' are
  the game's.

## Giving an item (built)

The talk menu has **Give an item** (friends and before: *Chat*, *Give an item*, *How are we getting along?*, *Never
mind*). It is the game's own "Give item" to a classmate, aimed at the other student, and the item really moves from
one student's bag to the other's. Everything is in `game/src/coop_talk.cpp` and `game/src/coop_content.cpp` (event
90002's "give" part, event 90012, lines 400-411 of bank 60).

**How the game gives a classmate an item** [V]. The classmate menu (message 1674 in banks 40-51: *Talk / Give item /
Hang out / Go to school / Nothing*) goes on to 1676, whose tag `$073'705'1'1;` starts event 705 block 1. That event:
query 0x03 opens the bag on the touch screen and waits until it closes (the item dragged out goes to the student's
hand), 0x04 (1) and 0x05 (the player's state 7: the item held out); then `test_player` (0x5C, ov028 0x02113600:
compares the held item, the player's vtable +0x80 with 0, `*(player+0x4FC)+0x65E & 0xFFF`, or its kind with op >= 100)
refuses ten items with system message 1017 ("You cannot give this item away as a present": 392, 101, 122, 103, 332,
114, 2106, 398, 112, 123) and nothing held ("What?"); query 0x12 (ov028 0x02114D20) with 1 plays the hand-over and
leaves the hand empty, with 0 puts the held item back in the bag (vtable +0x10C, main 0x0202B224). [V, run]: the
student record (`player+0x4FC`, the local one 0x020AD824) holds the bag at +0x636 (15 s16 slots, -1 empty; main
0x02039948 finds a free slot, 0x0203A1C0 puts an item in one) and the hand at +0x65E (-1 empty); holding an item moves
it from its slot to the hand (a RAM dump before and after: only those 4 bytes changed). Pressing A with an item in
hand to talk puts it back in the bag first, which is why the item is picked inside the talk, as 705 does.

**Ours** (event 90002, "give"): the same queries 0x03, 0x04, 0x05 open the bag and hold the picked item out. Then the
script asks coop_talk (t145 = 8) and waits on t158 (a byte no retail script, start record or text uses):
- nothing picked (the bag closed): "Maybe another time, then."; an item 705 refuses: the game's message 1017;
- otherwise the other console is asked whether its student's bag has room (message 9). **Full**: "<name>'s bag is
  full, so you keep it for now." Nothing was taken. **No answer** in 10 s: "<name> doesn't seem to hear you."
- **Room**: the game's hand-over runs (query 0x12 with 1), coop_talk empties the hand (vtable +0x84, main 0x0202DF74)
  and sends the item with the pair's new values (message 11). The other console puts it in its own bag with the
  game's calls and answers (message 12): in the bag, or it didn't fit after all (its bag filled up meanwhile), and
  then the giver gets it back in their bag ("<name> couldn't take it after all. It's back in your bag.").
- **A recorded transfer.** Each gift has an id (the giver's identity and a counter, `#n`). The giver records it as
  pending (`@pending` in its `.coop`, with the item and how many of it the student had after it left) before sending;
  the receiver records the id as received (`@got`, the same way) when it puts the item in its bag, and a confirmed
  gift clears the giver's entry. If the session drops before the confirm, nothing is given back: the entry stays
  pending ("Your friend left in a hurry. You'll sort out the gift the next time you meet."), and the next time the two
  students are in a session together the giver asks about each pending id (messages 13/14): received, the entry
  goes; never received, the item comes back to the giver. Both sides look at what their student really has first:
  a session that ends with "Unable to receive data" gives the console an earlier copy of its student's things [V,
  run: the giver's bag had the item again right after the error], so an item already back is not given again, an
  item the game handed back although the receiver has it is taken again, and a receiver whose copy lost the item
  answers "never received". The gift records reach the disk with the game's own save (when the save file is
  written), so after a crash a console's records and its bag agree.
- **A gift that went through, then a failed session.** The game's rollback after "Unable to receive data" also
  undoes gifts that were confirmed (no longer pending): the giver's bag gets the item back (one item becomes two
  once the receiver saves), and the receiver's copy can lose it although the giver counts it as given. So each side
  keeps the confirmed gift a while longer: the giver `@gave <id> <item> <count after the gift>`, the receiver
  `@recv <id> <item> <count after it arrived>` (counts follow later gifts of the same item: one more received by the
  giver, one fewer given by the receiver). After every session (and once after loading), when the game's session
  object is gone and the student has been free to move for 60 frames, coop_talk compares: the giver having more than
  the count gets one taken again ("Your friend still has the <item> you gave.", event 90013; no name, the session is
  over), the receiver having fewer gets one put back ("The <item> your friend gave you is back in your bag."). A
  record ends at the first game save outside a session after that look with the item still gone (giver) or still
  there (receiver).

**Verified, a confirmed gift and a failed session** (one Beige Wardrobe each, the gift confirmed, then one copy
stopped): the receiver stopped: the giver's "Unable to receive data", the game put the wardrobe back in its bag (1,
0 after the gift); at the next free moment "taken again", "Your friend still has the Beige Wardrobe you gave.", the
bag empty, `@gave ...#1 1468 0` kept (the game's save after the error had come first). The giver stopped: the
receiver's error, the game took the wardrobe from its bag (1, 2 after the gift); "put back", "The Beige Wardrobe your
friend gave you is back in your bag.", two in the bag, `@recv ...#1 1468 2` kept.
- Given: "You gave the <item> to <name>!" (the item's name with the game's tag `$507'<item>;`, which takes a number:
  the text bank is built again for each item, kept, so a bank the game has open keeps its bytes) and, the first gift
  of the day for the pair, "<name> looks really happy!" (friendship +3, affection +1, once a day, like a chat; "You
  and <name> are friends now!" at 30). The other console: "<name> gave you the <item>!", "You put it in your bag."
- Whatever happened, the script ends with query 0x12 (0) (anything still in the hand back in the bag) and the
  student's motion 1, as 705 ends.

The pair record gains two fields (last-gift-day, gifts; older lines still read) and the messages two (older builds'
messages still read).

**Verified** (two day-one-done copies connected in the dorm hall, `--clock` a weekday 13:00, test scripts outside the
repo driving the touch screen): the host picked the Beige Wardrobe (item 1468) in the bag, "You gave the Beige
Wardrobe to <name>!" / "<name> looks really happy!" on the host, "<name> gave you the Beige Wardrobe!" / "You put it
in your bag." on the guest; the host's bag empty,
the guest's holding two; both `.coop` lines the same (friendship 3, affection 1, gifts 1). The guest gave one back (a
second gift that day on that pair: friendship +4 from the chat the test made, the gift itself not counted twice). A
full bag on the receiver (filled by a test poke): "...bag is full, so you keep it for now", nothing moved. The
receiver's bag filled between the question and the item (a test poke timed on the "has room" answer): "couldn't take
it after all. It's back in your bag", and the item was back in the giver's bag. The bag closed without a pick: "Maybe
another time". A refused item (101): the game's message 1017. Best friends (a record started at stage 3): the
five-option menu, *Give an item* opening the bag, *Talk about us* with its three options and "You and <name> are best
friends."

**Verified, the drop** (the receiving copy stopped by a test switch, `MQ_COOP_GIFT_STOP=1`, right after it put the
item in its bag and before its answer; then started again from its save and reconnected with Connect now): the
giver's screen said "Unable to receive data", its record kept `@pending ...#1 1468 0`, and the game itself had put
the item back in its bag; the restarted receiver had neither the item (never saved) nor the id; on meeting again
"never received", "item 1468 is already back": one Beige Wardrobe on each side, as before the gift.

**Risks**:
- A crash of the giver's copy after the gift left but before its game saved: its save still has the item and no
  pending entry, while the receiver may have saved the gift (one item becomes two). Two saves on two PCs cannot be
  made one; it needs a crash in that window.
- The counts are a heuristic: if the student gets or uses another of the same item between the drop and the next
  meeting, the check can read it wrong (at worst one item gained or lost).
- Received ids are kept per pair (the last 64).
- The `@gave` / `@recv` look is a count too: getting (or using) another of the item in the same session by other
  means than a gift can read as the game's rollback (at worst one item taken or put back). The records end only at a
  save outside a session after the look; a save during a session is not seen as the end.
- The receiver's bag is changed while its student may be in a menu or another event (as the game's own mail and
  deliveries do not); the bag screen, if open, shows it the next time it opens.
