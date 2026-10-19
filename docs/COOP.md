# Co-op

Where co-op stands, what the game itself offers, and the road from there to playing the whole game together.

## What the game has

Magician's Quest's multiplayer is visiting: up to four students meet in one town, over Multi-Card Play (DSes nearby)
or the Nintendo Wi-Fi Connection.

- It starts at the **Gateway of the Beginning and End**, upstairs in the dorm: touching it asks "Where would you like
  to go?" and offers the **communications room**. There, a menu offers *Can we play together?* (visit another town),
  *Come and play with me!* (invite others to yours), *friend code* and *Exit*, then *Multi-Card Play* (nearby) or
  *Nintendo WFC* (far away).
- A classmate with you in the communications room can be sent along as a *visiting student* (study abroad), for a few
  days.
- Early in the game the gateway answers "You cannot use it at this time" (seen on day one of a new save, at 10:00 on a
  Saturday and at 19:00 on a Monday); when it opens up is not known yet.
- While visiting, some things are refused: buying berry seedlings, some shop services ("you can't use that service
  while visiting another town"), some incantations ("cannot be used during wireless communications"), some items.

The game is lockstep (`docs/ARCHITECTURE.md`): every console runs the same simulation from the players' inputs, so
whatever the host's town does, every guest sees.

## What is built

- **Two copies on one PC** find each other (shared memory; Windows, and now Linux).
- **Two or more PCs** (`include/ntr/lan.hpp`, `runtime/src/lan_wireless.cpp`): the game's Multi-Card Play frames as UDP
  datagrams. One PC hosts (it hands each frame on to the others), the others join it by address: Settings > Network >
  *Co-op between PCs* (Off, Host, Join), *Host's address*, *Co-op port* (49251 by default), or `--lan-host [port]` /
  `--lan-join <address[:port]>`. Over the internet the host forwards that UDP port. The window's title shows who is
  connected. Each PC's console gets a MAC address of its own.
- **Far away** also works the game's own way, through the Wi-Fi Connection revival servers (Settings > Network).
- `tests/test_lan`: a host and two joining PCs over loopback through the calls melonDS makes (commands, replies,
  frames handed on, a PC leaving).

Not yet verified: a whole visit between two copies of the game, because the test save is still on day one (the
gateway refuses). That needs a save further on.

## The road to the whole game together

What was asked: start together, load into the world together, play the whole game side by side, and romance each
other. The game was not made for this: its story, classes and events belong to the one student of a save, and a
visit is a short trip to someone else's town. Each step below changes more of the game, so each is its own milestone.

1. **Visits that work everywhere** (here now, pending a check with a later save): host/join between PCs, the game's
   own visit.
2. **Stay together**: a visit that lasts — the guest follows the host through doors, into buildings, the school and
   events instead of the session ending, and the "not while visiting" refusals lifted where the game copes (hooks on
   those checks). Needs: where the game ends or limits a session, found in code.
3. **Start together**: both players' students in one town from the first day. The save holds one student; a second
   needs a save layout of our own beside the game's (the guest's student, items, money, friendships), written back to
   the guest's own save after each session so neither loses progress.
4. **The story together**: classes, events and cutscenes are scripted for one student. With both present, the second
   watches and joins where the scripts allow (they are data, `talkfigs`/`script`), and gets the same rewards.
5. **Romance between players**: the game's dating is between a student and classmates, with its own dialogue and
   events. Between two players it would be new content: new lines, new events, and their state in our save layout.
   Possible once 3 and 4 exist, as an addition rather than something unlocked.

Each milestone keeps the lockstep: everything added must run the same on every PC from the same inputs.
