# Player-to-player friendship and romance: research and design

Status: research done, nothing built yet. [V] = verified in the ROM's data, scripts or code; [I] = inferred.

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

Open: what fills the 4 player slots; whether a guest's values toward the host's classmates are kept after a visit;
the amounts; whether the "exchange notes" feature mentioned in interviews exists in the US release.
