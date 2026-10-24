# Player-to-player friendship and romance: research and design

Status: research done (two passes), building the groundwork. [V] = verified in the ROM's data, scripts or code; [I] = inferred.

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
5. **Lockstep**: every change runs in the shared simulation from both players' inputs and writes a record named by
   both players' indexes, never the local student's record (player -1 is a different student on each console); each
   player's copy is committed to their own save when the session ends.

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
   text bank no retail data uses), so new scenes are written in the game's own script language.
2. Talk to the other player: when the talk-target finder finds no NPC and the other player is in front, start our
   player-talk event (t154 = 110 + slot); the other console runs a mirror with a yes/no box.
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
