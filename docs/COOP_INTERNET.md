# Co-op over the internet

How two players 30-150 ms apart (round trip), with some packet loss, play a co-op visit; why the PC-to-PC link of
before (`runtime/src/lan_wireless.cpp` carrying the game's radio frames) cannot do it; the designs; the one built
(D: a stand-in copy of the game on each PC); how it works, how it was tested, what is left.

## Summary

- **Built** (2026-10-25): Settings > Network > *Co-op between PCs* (Host / Join) and *Over the internet: On* on both
  PCs (or `--lan-host` / `--lan-join` with `--coop-internet`). Each PC then also runs a hidden **stand-in** copy of the
  game for the other player, started from the other player's save, linked to the player's copy over the same-PC link:
  the game's radio exchange never leaves the PC. Only game data crosses the internet: the session's packets (each
  frame's movement newest-wins, everything else reliably and in order), our side channel (town sync, players' talk,
  gifts, the pair records), each player's save once, and each player's state, which its stand-in follows (it connects
  in the far player's role, and leaves with the game's Disconnect when the far player does). Tested on one machine
  (four processes, `NTR_LAN_DELAY` / `NTR_LAN_LOSS` on the link): connecting by Connect now, walking (each sees the
  other, also after a warp into town), a chat, a gift and a Friendship cast (each once, the pair records the same on
  both), the town sync, a player leaving (or its game killed), and the new day's disconnect and reconnect, at 25 ms
  each way with 10 % loss and 75 ms each way (with 0-20 ms jitter and 5-10 % loss); the players' frame rate is the
  same as with no delay ("Results" below).
- **A player's game gone** (killed, crashed, closed, the network lost; 2026-10-27): every link now reports the lost
  console to the game within 5-10 s (a process that ended at once), so the survivor gets the game's own error or
  leaving messages, goes home and plays on; a crashed stand-in is started again. Before, a guest on the same-PC link
  (also a player's copy beside its stand-in) froze for ever. "A player's game gone" below.
- **The cause.** The link carries the DS's radio protocol itself (melonDS's Multi-Card Play frames). That protocol
  assumes an exchange done within microseconds, and melonDS emulates it by **stopping the emulation of both
  consoles until the other side's frame arrives**. Every MP turn costs one network round trip of wall time, and a
  reply later than the wait (25 ms) counts as missing. So a link fails above about a 20-25 ms round trip (default
  wait) or runs at about 1000 / RTT frames a second (longer wait). The game itself is not the limit.
- **Answering the MP frames locally from cached data (the "relay") does not work at the frame level.** Tried as an
  experiment (below): with replies answered at once from the newest one received and the guest not blocked, the
  pair **did not connect even with no delay at all**. The WM library's radio layer (the TSF clock the client keeps
  in step with the host, the reply window, the ack bitmap and the retries) depends on the blocking timing.
- **So the stand-in** ("shadow", design D): the cached-reply idea moved up one layer, to the game's packets instead
  of the radio's frames.

## Why the radio frames over UDP break

### The DS's MP exchange

Multi-Card Play is the WM library's MP mode (NitroSDK WM, running on the emulated ARM7 and Wi-Fi hardware: our
recomp does not touch it; `docs/ARCHITECTURE.md`, "Local wireless"). One **MP turn**:

1. The host's hardware sends a **command** frame (TX slot CMD) carrying the host's data, the reply window per client
   (`W_CmdReplyTime`, a few hundred microseconds) and the bitmap of clients polled.
2. Each polled client's hardware answers within its slot of the window with the **reply** the client's ARM7
   prepared *beforehand* in its reply TX slot (on real hardware the reply is not computed when the command arrives;
   it is whatever the client queued last; if nothing is queued the hardware sends an empty default reply).
3. The host sends an **ack** with the bitmap of clients that failed to reply; WM retries their data in later turns.

The client's clock (the 802.11 TSF, `USCOUNTER`) is set to the host's at association and kept in step by each frame;
WM schedules on it. Several turns run per video frame: the experiment below counted ~3 polled commands per emulated
frame (44,400 commands while ~14,000 frames ran).

### How melonDS emulates it (third_party/melonDS/src/Wifi.cpp)

- **Host**: when a command's transmit ends (`ProcessTX`, case 1, slot 1) it calls `Platform::MP_RecvReplies(...)`
  **synchronously, on the emulation thread**: the console stops until every polled client replied or the wait runs
  out. melonDS's LAN backend waits `MPRecvTimeout = 25` ms (`net/LAN.cpp`), LocalMP the same (`RecvTimeout`); ours
  `kRecvTimeoutMs = 25` (`lan_wireless.cpp`, `LanWireless::recv_replies`, `NTR_LAN_WAIT_MS` to change it). A reply
  older than the command (`timestamp < cmd timestamp - 32` µs) is thrown away as stale. Clients missing → their bit
  in `MPClientFail`, `ReportMPReplyErrors`, and the ack tells WM.
- **Client**: after each frame from the host it computes `NextSync` (the frame's timestamp + its air time + the reply
  window, or + the ack's `runahead`), and `USTimer` calls `CheckRX(2)` → `MP_RecvHostPacket` once its clock passes
  `NextSync`; that call **blocks** until the host's next frame (our `recv_host_packet` → `recv(..., block=true)`,
  25 ms). So the client cannot run ahead of the host's last frame.
- Together: host and client run in **lockstep per MP turn**. Each turn costs: command one way + reply one way, i.e.
  one round trip of wall time, plus the hop through the host PC for a third PC.

### What that does at a given round trip

| link | what happens | evidence |
|---|---|---|
| same PC, LAN (< 1 ms) | turn costs nothing; 123 fps unpaced | docs/COOP.md "What is built" |
| 2-5 ms each way | connects, plays | same |
| 15 ms each way (30 ms RTT), wait 25 ms | replies arrive after the wait: thrown away as stale in the next turn's wait; every turn fails; "Unable to connect" | same ("30 ms each way did not"; 25 ms is the wall) |
| 15 ms each way, `NTR_LAN_WAIT_MS=80` | connects; **27 fps** instead of 123 | same; ≈ 1000 / (30 ms + emulation), the per-turn round trip |
| 50-150 ms RTT, long wait | would connect if the WM handshake survives; **7-20 fps at best** (one RTT per turn, several turns per frame would make it worse) | extrapolated |
| 7-10 % loss on the game's frames | "Unable to connect" / "Unable to receive data": a lost command, reply or ack fails the turn; a radio retries within microseconds, UDP does not | same |
| reordered frames (jitter) | broke the exchange; fixed by dropping late frames (`handle`, `frame_ts`) | same |

So the limit is the emulated radio's per-turn lockstep, not the game's session code: the game's own data is one
16-byte packet per console per frame (main 0x0202C4CC: `[0]` type, `[1]` slot; outgoing slot session+0x4CB0;
received into the player's queue of 4 at +0xE4, the last two at session+0x4A08 + slot * 0x50) and it already
tolerates a far partner over the Wi-Fi Connection (its internet mode, below).

### What the game tolerates (what is known)

- **Movement (types 0x0E-0x12, 0x0F every frame)** is state, not events: the outgoing slot may be overwritten by a
  newer one (main 0x0202C4CC: only these types may replace the slot's packet), the receiver keeps the last two.
  Stale-but-present is fine: the other student is drawn a little behind.
- **Events (0x13 action, 0x1E talk, 0x33-0x47 ground objects, telegrams, incantations)** must arrive exactly once and
  in order (the sender's slot is not overwritten by movement while one waits). Ground objects are arbitrated by
  the host against its own grid (ov001 0x020F0E50 / 0x020F0C4C) and re-broadcast; the two-player incantations wait
  for the other console's answer (0x45). These tolerate delay (they are request/answer), not loss or duplication.
- **Silence**: a guest that stopped was reported "Unable to receive data" by the host after our link pruned it
  (`kGoneMs` = 5 s, `prune`); during those seconds every turn failed and the session survived. How many failed
  turns the WM layer or the session code accept before giving up was not measured here; the 7 % loss result says
  scattered failures are already too many for connecting.
- The connection itself (scan, beacons, auth/assoc, then the town transfer of 0x993C bytes through the game's own
  frames) goes through the same radio layer and the same timing.

## Designs

### A. Longer waits (what `NTR_LAN_WAIT_MS` tries)

Pros: no new code. Cons: frame rate ≈ 1000 / RTT per turn and worse with several turns a frame; any loss still fails
turns. 30 ms RTT gave 27 fps; useless at 50+ ms. **Rejected.**

### B. Frame-level relay / cached replies (tried)

Each PC answers its console's MP exchange at once: the host's commands get the guest's newest reply received so far
(stale), the guest is not held to the host's frames; the network carries the frames as before, only nobody waits.

Experiment (`NTR_LAN_RELAY`, env-gated in `LanWireless::recv_replies` / `recv_host_packet`, built in the verify
worktree and reverted; patch kept outside the repo): host's `recv_replies` returns at once with the newest
non-empty reply of each polled client that arrived since the last command (none if none), `send_cmd` no longer
clears the queue, and the guest's `recv_host_packet` does not block. Two copies from `day1done.sav` over UDP on
loopback, Connect now (`udppair.sh` + `fastgo.py`):

- with `NTR_LAN_DELAY=40` (80 ms RTT): the host was never stalled (54,600 commands answered at once) but **83 % of
  them had no new reply**, and the guest's search ended "not connected (no host found, or called off)";
- with **no delay at all**: the same: 89 % of commands without a new reply, the guest not connected. Without the
  relay such a pair connects (47-87 s from start, docs/COOP.md).

So the radio layer needs the blocking schedule to work at all: an unblocked client misses its reply windows (its
TSF runs ahead of or behind the commands it is given), and the host's WM counts every turn without a reply as a
failure. A relay that works would have to keep the client's clock and reply windows consistent and rewrite WM's
sequence and ack fields of replayed frames: re-implementing WM's parent and child state machines in the link.
**Rejected** (high effort, fragile, untestable against hardware).

### C. Mirror lockstep (melonDS's netplay design)

melonDS's own netplay (`third_party/melonDS/src/net/Netplay.cpp`, mostly `#if 0`, unfinished) syncs the cart SRAM
and a savestate to every PC, spawns **mirror instances** so each PC runs every player's console (their MP between
them is local), and sends only pad input, applied `lag = 4` frames later (`InputQueue`, "TODO: make
configurable").

For us: each PC runs both consoles (two processes on the same-PC link, one shown, one headless), both PCs feed both
consoles the same inputs with a delay of about RTT/2 + margin (150 ms RTT → ~6 frames, 100 ms on one's own student).
Pros: no game knowledge; anything the game does in a session works, incantations and arbitration included.
Cons: needs the **four processes bit-identical**. The game alone is deterministic (replays), but the two-process
wireless is not: replies are waited for with wall-clock timeouts (`local_wireless.cpp`), our side channel resends
on 150 ms timers, the town sync runs at frame ends; any difference is a desync, found only by frame hashes, with no
savestates to recover (`docs/TESTING.md`). Every PC needs every save and the same clock. Input delay on one's own
student; a late packet stalls both PCs. No rollback (no savestates).

### D. Stand-in console on each PC, game packets between PCs (built: "Built: D, as it works")

The cached-reply idea at the game's level. On each PC run the player's copy and, headless, a **stand-in** (shadow)
copy for the other player, connected by the same-PC link (shared memory, already built: `LocalWireless`,
`NTR_LINK_NAME`). The game's MP exchange, the scan, the association and the town transfer all happen between the two
local processes in microseconds. The stand-in's outgoing session packets are replaced by the far player's real
packets, which arrive over the internet through our own channel.

```
host PC:   H (player, shown) <== same-PC link ==> G' (stand-in for the guest, headless)
                 ^  side channel, H's packets                 ^ G's packets go out in G''s place
                 |              internet (UDP, our protocol)   |
guest PC:  G (player, shown) <== same-PC link ==> H' (stand-in for the host, headless)
                                                    H's packets go out in H''s place
```

- **What each console sees**: H sees G' whose packets are G's (G's student walking, talking, casting), delayed by
  the one-way latency. G sees H' whose packets are H's. Each console's own frame rate is not touched by the network.
- **What must be the far player's**: G' joins H's session with G's **student record** (0x1858 bytes, sent at
  connect) and H' must host **H's town** (the bulk 0x993C-byte block it sends G at connect). So each stand-in starts
  from a copy of the far player's save (sent over the side channel before the session; the stand-in runs on a scratch
  copy, never saving over anything that matters) and the same clock (host time, which the game syncs anyway).
- **Packets**: movement (0x0E-0x12) latest-wins (an unreliable datagram per frame; the stand-in emits the newest it
  has, repeated if none new: stale-but-present, as the game's own slot rule allows); every other type reliable and in
  order (our `coop_channel`, already tested with 25 % loss), each emitted once by the stand-in. This is the split the
  game makes itself.
- **One authority**: arbitration (0x33-0x47) must be the real host's. G's request reaches H through G' (H arbitrates
  for real); H's answer reaches G through H'. H''s own answers are not emitted (its packets are H's), but H' must not
  act on its own town either: its arbitration writes are harmless (H' is a stand-in) as long as the town G sees is
  H's (below). Two-player incantations (0x45 asks, the target answers) work the same way, with RTT added.
- **Our side channel** (`coop_sync`, `coop_talk`, `coop_npc`) runs between the two *players'* copies H and G, as now
  over `lan_wireless.cpp`'s socket; the stand-ins do not take part (they must not send town sync or pair messages).
  The town sync then keeps G's copy equal to H's real town (as today), whatever H' did.
- **Lifetime**: the stand-in follows its player: when the far player disconnects (or the link is lost for N
  seconds) the stand-in leaves with the game's own Disconnect (as `mq_coop_session_pad` does for the new day), so
  the player's copy sees the game's normal "went home" / "has disconnected" messages. When the far player's console
  is between fields (loading) the stand-in keeps emitting the last movement packet.
- Pros: no radio timing over the network; latency only delays what the other player sees; loss only costs
  movement freshness (events are resent by our channel); no input delay; reuses the same-PC link and the side
  channel that exist and are tested; the stand-in's own simulation does not need to match anything.
- Cons: a second game process per PC (headless, unpaced only as fast as its partner; the machine ran two copies
  fine); a save exchange at start; hooks in the game's packet path; the stand-in's game state is not the far
  player's (only its packets are), so anything the session code reads from the stand-in console's *own state*
  rather than from its packets is wrong: the student record at connect (handled by the save copy), and whatever
  else the session reads at connect (to check; e.g. the host's name in the guest's town list comes from the
  stand-in host's save, which is the host's).

### E. The game's own internet mode (Nintendo Wi-Fi Connection)

The communications room offers *Nintendo WFC* (message 41/42, option 1): the game's DWC peer-to-peer play, built
for internet latency, through a revival server (Settings > Network). Pros: the game's own protocol for exactly
this. Cons: a third-party server and friend codes; not verified for this game's visits here; our side channel
(town sync, pair messages, NPC sync) rides on the LAN link's socket and would need its own path (a direct UDP or a
relay) beside DWC's NAT traversal. Worth a test as the cheap option; it does not remove the need for our channel.

## Built: D, as it works

```
host PC:   H (player, shown) <== same-PC link ==> G' (stand-in for the guest, hidden)
                 ^  side channel; G's packets in G''s place      | taps: H's packets
                 |              internet (UDP, our datagrams)    v
guest PC:  G (player, shown) <== same-PC link ==> H' (stand-in for the host, hidden)
                    H's packets in H''s place                    taps: G's packets
```

### The pieces

- **The setting.** Settings > Network > *Over the internet* (Off / On; `online.coop_internet`; needs *Co-op between
  PCs* Host or Join; both players turn it on; takes effect at the next start), or `--coop-internet`. The window's
  title says what it does ("co-op: hosting, 1 other PC, over the internet: other PC 179 ms, stand-in in the session,
  playing together"; on the way: "waiting for the other PC", "their save 40%", "stand-in not running / starting /
  ready / connecting / leaving").
- **The links** (`runtime/src/lan_wireless.cpp`). In internet mode the game's radio (`wireless_link()`) is an
  `InternetLink`: the MP calls go to the same-PC link (`LocalWireless`, shared memory) under a link name of this
  process's own (`NTR_LINK_NAME` + `-i<pid>`, inherited by the stand-in), and its side channel (`coop()`) is the UDP
  link to the other PC, whose idea of "in the game's multiplayer" follows this console's (Begin/End and the
  keepalives as before). The UDP link carries no radio frames; besides the side channel's datagrams (kind Coop) it
  carries ours (kind Net, `runtime/src/coop_net.cpp`): `R` a reliable ordered stream (a `CoopChannel`, resent after
  250 ms), `M` movement newest-wins (numbered; a late one is dropped), `S` the player's state every 100 ms (role:
  connecting or connected as host / guest; in the session; its stand-in's state), `P`/`Q` pings (the round trip),
  `F`/`f` the save in 1 KB pieces with a bitmap of what arrived (64 in flight, resent after 2 x RTT).
- **The stand-in** (`game/src/coop_net.cpp`, `--standin <port>`). Started by the player's copy (`proc::spawn`,
  `game/src/process.cpp`) once the far player's save has arrived: the same program, the same ROM and game module,
  `--save-file <save folder>/coop-standin/standin.sav --read-only-save --no-settings`, the far player's clock
  (`--clock`, the town's date and time when its save was sent), an input script that taps through the title screen
  (frames 1950 and 2210, as the harness's `boot.sh`), a hidden window (`SDL_WINDOW_HIDDEN`), no audio, never paused
  for focus, drawn every 600 frames, unpaced until it has first been free to move (then paced). The test drivers of
  the player's copy (`MQ_COOP_AUTO`, `MQ_WARP_FILE`, ...) are left out of its environment. Its log is
  `coop-standin/standin.log`. It talks to its player's copy over UDP on 127.0.0.1 (`T` its taps, `r` its state four
  times a second; `c` the command, ten times a second). It ends with the player's copy (Quit; on Linux
  `PR_SET_PDEATHSIG` with SIGKILL, on Windows a job object that kills it; else 10 s without a command: seen, the
  player's copy killed with SIGKILL, its stand-in gone 10 s later, before the death signal). Inside it: no town sync, no pair messages, no new-day detection (all the
  player's); A is pressed every 30 frames only while a text box shows (the dorm's intro, a visit's end messages):
  an A elsewhere would be an action of its student, which the player's copy would show.
- **The stand-in follows the far player.** The far player connecting or connected as host (Connect now asked for
  or under way, `coop_connect::active_role`, or in the session as slot 0): the stand-in connects as host (Connect
  now); as guest: it joins (Connect now searches until the player's copy hosts). Neither (or the far PC not heard
  from for 5 s): the stand-in leaves with the game's Disconnect (`coop_session::leave`: Select, Yes, A through
  "Disconnecting." ... "Save complete."), so the player's copy sees the game's own "<name> is going home." or "The
  person you were playing with has disconnected." The new day (`coop_session.cpp`) works as before: the host's
  Disconnect ends its visit, its stand-in on the guest's PC follows, the guest goes home with the game's messages,
  each town turns its day, each player connects again with Connect now and the stand-ins follow. The guest first
  closes what it has open (its session would not answer its stand-in host's leaving), and if the visit still splits
  (the far host out of a session for 3 s while this guest's session goes on), the guest reports itself out of the
  session, its stand-in host is told to leave, and it goes home and connects again (docs/COOP.md, "When a player has
  something open at the new day"). It connects again only after the new day: the split also follows the host's own
  Disconnect (the far host leaves its session before the stand-in host here has gone through its Disconnect's
  messages), and then the guest goes home and stays there. `coop_session::host_gone` tells them apart by the host's
  side-channel message "the visit ends for the new day" (type 41, kDayAsk): heard in this session, the guest joins
  again; not heard, the host's player chose Disconnect ("coop day: the host is no longer in this visit: closing what
  is open, then home (the host's own Disconnect: not joining again)"). Before (to e73a460) the guest always joined
  again: the communications room, "Searching for a town to visit...", "Unable to find a town to visit.", and with
  "Connect automatically" or `MQ_COOP_AUTO=join` the room's 30 searches (docs/COOP_PLAYTEST.md, "Soak").

### The game's packets (found for this, ov023 = Multi-Card Play's session)

The plan assumed one per-frame 16-byte packet through the session's send (main 0x0202C4CC). The session in fact has
two paths (ov023 0x02110A68, the session's update, vtable +0x10; 0x021105B8 its first step):

- **DataSharing** (`WM_StartDataSharing`, port 13, 16 bytes per console, double mode; started at ov023 0x0210F3CC):
  every frame each console shares its slot's last packet of types < 28 (the session's entry +0x4A08 + slot * 0x50,
  written from the outgoing slot at 0x02110BC8): movement (0x0F standing or walking: +2 the area, +3 the action,
  +4 x, +8 z, +12 y (s16), +14 the direction, in the area's own units), 0x10-0x12, and actions (0x14 with a value).
  The same bytes come again every frame until the next packet. Each frame the session takes each console's bytes
  (ov023 0x0211003C, `WM_GetSharedDataAddress`) and hands them to that console's student (ov001 0x020EFE0C: into the
  student's queue of 4 at its +0xE4; the queue's address is at session +0x4A04 + slot * 0x50) and keeps them
  (+0x4A08). The arrival (the walk out of the Gateway, 0x11 with action 7) and the connection's own steps (types
  0x00-0x0D, +1 0) go this way too.
- **Messages** (`WM_SetMPDataToPort`, port 14, ov023 0x0210FFB8; received by the port callback 0x0210FAFC ->
  0x0210FFE8 -> the session's receive, vtable +8, ov023 0x0211068C: session, aid, bytes, length): types 28 and up,
  16 bytes, in the session (+0x6915 == 9) only: 28-50 straight to the console's student (0x020EFE0C), 51 and up into
  the sender's inbox (+0x4A18 + aid * 0x50) for the host's arbitration, types < 28 into the inbox only if it is empty.
  Seen: 0x20 / 0x21 (the host's answers), 0x37 / 0x38 (a guest walking into new cells), 0x31 (a guest's arrival),
  0x30 (the host leaving). The town's transfer at connect goes in 128-byte pieces through 0x02050174 and never
  reaches the receive.

Hooks (`game/config/ynne/hooks.txt`; one regeneration): `ov023 0211068c instead mq_net_recv` (the receive),
`ov023 0211003c instead mq_net_shared` (the shared bytes: the hook calls `WM_GetSharedDataAddress` itself, 0x020921E4,
then reads or writes the 16 bytes), `ov023 0210ffb8 before mq_net_sent` (research log only, `MQ_NET_LOG`).

### What crosses, and when it is swapped in

- **A stand-in taps** what its partner (the player's copy on its PC) sends: every message the receive gets in the
  session, and each frame's shared bytes of the partner when they change (and twice a second as they are, so a lost
  last movement is made good). Messages and shared bytes other than movement go on the reliable stream; shared
  bytes of types 0x0E-0x12 as movement, newest-wins.
- **A player's copy** drops its stand-in's messages and shows the far player's shared bytes in its stand-in's place
  (written over the stand-in's in the DataSharing step: the far player's next event, one a frame, in order; else its
  newest movement; else the last again), and puts the far player's messages in through the receive, as if the
  stand-in had sent them (at the end of a frame; one through the inbox per frame, once the game has taken the last).
  The packets' own bytes are not changed (the slots are the same on both PCs: host 0, guest 1).
- **Only once both have arrived.** The arrival runs between each player's copy and its stand-in, as a visit's does
  (the town's transfer, the guest's 0x31, the host's answers, the walk out of the Gateway): the far player's packets
  of then would make no sense here (they are its own arrival with its own stand-in). The swap starts when this console
  has been in the session with its arrival steps over (+0x6917 0, +0x6919 -1, the field loaded in the session) and
  free to move for 2 s, the far player says the same of itself, and the stand-in's student here stands after its
  own arrival (its bytes type 0x0F with no action, and its student's action byte +0xA2 back to 0) for a second. What
  came from the far player before is dropped. (Swapped too early, the stand-in's student stayed in the arrival's
  walk here: the far player's movement cannot end it; seen at 10 % loss, and again after the new day.) Once ready, a
  console stays ready until its session ends: a door or a warp is no new arrival, and the far player's student must
  go on being shown where it is (the stand-in stays where it arrived). (Dropping the swap at a field change once let
  the guest's town show the stand-in host in the dorm while the real host was in town, and an event of ours that
  looked for the host's student there ended the game.) The role a player reports comes from the session's hosting
  byte (+0x6912), which a field loading in between does not change. A safety net: if the far player's student here
  is caught in an arrival's walk (+0xA2 7) while the far player stands, for 5 s, the stand-in's own bytes are shown
  until it stands, then the far player's again.
- **The session's own messages stay local**: types 0x2B-0x32 (the transfer, a console arriving, leaving, the
  session's steps) are neither tapped nor dropped. Forwarded, the host's "leaving" reached the guest while the
  guest's stand-in host still hosted: the guest's session ended with "Unable to receive data". Each stand-in leaves
  its session itself, following its player. **Except the host's answers 0x2D / 0x2E** (request type - 23: the answers
  to 0x44 / 0x45, the two-player incantations' messages) **to another console's request** (byte 1, the requester, is
  not the stand-in's partner): the host arbitrates its own cast and tells the guests with these, so a stand-in taps
  them for its player (2026-10-27: kept local, a host's Friendship failed over the internet, "You failed at using
  your incantation.", the guest never joined; the same-PC link was fine). Answers to the partner's own requests stay
  local (the far player gets its own stand-in's).
- **A guest's telegram** (0x42, then the entry in the game's own transfer, ov001 0x020F1470 kinds 1 and 4): the
  transfer runs between the guest and its stand-in host, so the real host never got the entry, and its session waited
  for it with the second 0x42 in the guest's inbox for ever: every later request of the guest (its walking into new
  cells, 0x37 / 0x38) queued behind it, and the host's next field load hung (a black screen). Now the far guest's 0x42
  is not put in on the host; the guest sends its waiting-list entry (town block +0x7AA8, ten of 0xE4 bytes) by the town
  sync as a request, made as the host's game would make it from the transfer (+0xDE 0xFA "another player", +0xE0 1,
  +0xC4 the sender's name: the guest's own copy holds the addressee's there, and the cat read "From: AAAAAA").
- **The inbox**: a far player's request (types 51 and up) waits for the inbox of its slot to be empty, one per frame,
  but at most 300 frames; then it is put in over the last, as the DS's receive does (it never waits), so one request
  the host never takes cannot hold up all the later ones.
- **The session's own steps in the shared bytes stay local too**: a far player's DataSharing bytes of types
  0x00-0x0D (a console arriving or leaving) are not put in, and the stand-in's own bytes of those types are not
  written over. Found in a week's playtest (docs/COOP_PLAYTEST.md): put in, a guest's Disconnect reached the host's
  session as its stand-in's while the stand-in was still in it (the stand-in dropped at the radio, "deauth"; the host
  never said "<name> is going home."), and written over, the stand-in host's answers to the guest's own Disconnect
  never reached the guest ("Unable to disconnect.", "Unable to receive data."). With both, the guest's Disconnect
  ends as alone ("Disconnecting.", "Disconnected.", "Saving.", "Save complete."), and the new day's still works.
- **Arbitration** (ground objects, 0x33-0x47) is the real host's: a guest's request reaches the host's copy through
  its stand-in there, and the answer comes back the same way; the stand-ins' own answers are dropped.

### Differences from the plan

- Movement is not the per-frame packet of the session's send but the DataSharing bytes; the capture is not a hook
  on the sender but on the receiving stand-in (it gets exactly what the game delivers, once), and the substitution is
  on the player's copy, not in the stand-in's outgoing slot. The stand-in needs no packet hooks of its own beyond the
  taps.
- The save is 64 KB (`cfie.sav`), sent once when the far PC is first heard from; the stand-in starts from it right
  away (about a minute to be free to move on this machine) and lives as long as the game. The town and the student
  the stand-in brings to the session are the far player's as of that save; the town sync's whole block at its hello
  and each student's record bytes make them current (as for any visit).

### Open

- The time of day: a guest takes its time from its stand-in host (the game's own time sync at connect), which
  started with the host's clock when the host's save was sent. With the PCs' own clocks (as players run the game)
  both are the real time; in unpaced test runs the stand-in's clock runs ahead of its player's (seen: the guest 2
  minutes ahead after 4 minutes). The town sync leaves each console's time bytes its own.
- Classmates in town: in these runs (day two 15:30 and 17:30 from `day1done.sav`, both warped out of the dorm with
  `e 221`) no classmate came out in co-op, over the internet and over plain UDP alike (the same with the clean build
  of 692d8b2), while a single-player run at 15:30 had 8 out; the NPC entries matched on both consoles. Not from the
  internet link; to check against older builds (docs/COOP.md reported 9 out).

- Only two players (one guest): the far PC is the first one heard from; a third PC is ignored.
- The stand-in starts from the save as it was when the game started; a player who plays alone for a long time
  before connecting brings an older student and town to the stand-in (the town sync's whole block and the record
  bytes correct them at the hello, but what the session built from them at connect, e.g. the student's look, is the
  old one until it is made again).
- Each stand-in is a second game on each PC (about as much CPU as the player's copy while in a session).
- "Over the internet" is a setting, not chosen by itself from the round trip: both players turn it on.
- Not tested between two real PCs or on Windows (the Windows code builds in CI only: process spawn with a hidden
  window and an environment block, `GetModuleFileNameW`, `_putenv_s`).
- A remote student can still end up stuck in an action here if a far player's action event and the stand-in's own
  arrival interleave in ways not seen yet (the action byte +0xA2 not 0, 10, 13 or 14 makes the student ignore
  movement, main 0x02031404's 0x0F case).

## A player's game gone (killed, crashed, closed, the network lost) (2026-10-27)

If one player's game ends in a visit (a crash, the window closed, the process killed) or its network goes, the other
player's game must not wait for it for ever. The DS has its own handling of a lost link, and every link now leads to
it: the survivor sees the game's own error or leaving messages, goes home and plays on alone.

**What the game does with a lost link.** A guest whose host's frames stop gets the session's event 7 ("DS Wireless
Communications have been disconnected due to a communication error.") or 8 ("Unable to receive data. Terminating
connection..."): the field is loaded again with change 5, the session object is deleted, and after A the student
stands at home, free, without a save (the log's "coop visit: over, the student free at home"; its save stays the
visit's connect save, so nothing is lost or copied; closing the game right then counts as turning off without saving,
the game's own rule). A host whose guest stops answering gets event 8 the same way ("Unable to receive data."), and
is home, free. Over the internet mode the survivor's stand-in for the far player leaves with the game's Disconnect
instead, so the survivor sees the game's normal end of a visit: a guest "The person you were playing with has
disconnected.", "Disconnecting.", ..., "Saving.", "Save complete." (its own data saved, as at any host's leaving);
a host "<name> is going home." and plays on in its session (a new guest can join it).

**What was wrong: the same-PC link** (`runtime/src/local_wireless.cpp`, also the link between a player's copy and its
stand-in over the internet). melonDS asks for the host's frames on every timer tick of a guest in the MP exchange
(`Wifi::USTimer` -> `CheckRX(2)` -> `MP_RecvHostPacket`), and each ask waited 25 ms; a host was only "gone" when its
slot's "in the session" bit was cleared, which a killed or crashed process never does. So the guest stood still for
ever (6 % CPU, "no communication error in 10 minutes"), and a host whose guest died ran at ~7 frames a second (each
command waited 25 ms for the dead guest's reply) for ~30 s until the game's own error. Now:

- A copy whose process has ended is taken out of the session at once (checked every 250 ms; on Linux a zombie, ended
  but not yet reaped by its parent, counts as ended: a stand-in is the child of its player's copy, which reaps it
  from the very thread that waits). `recv_host_packet` then returns -1, melonDS leaves the MP exchange (as its own
  LocalMP does for a host that left) and the game's error follows.
- A copy not heard from for **8 s** (no frame, command, reply or ack at all: its game stopped, a debugger, a frozen
  machine) is left out of the waits the same way while the silence lasts (logged "instance N not heard from for 8 s:
  lost"; "heard from again" if it comes back; by then the game has ended the visit on the waiting side).
- Pause in background is off while the game's multiplayer runs (`game/src/main.cpp`): a paused copy stopped its
  partner, and would now be taken for lost.

**The timeout, and why 8 s.** Each copy logs the longest time it sent nothing while in the game's multiplayer
("wireless: instance N sent nothing for M ms (the longest so far)"). In every pair here (same-PC, internet mode at
200 ms with 1 % loss, connects, the connect's save on both sides, walking, the visit's end) the longest was 1.07 s,
always at the connect (the room's "Saving."): the DS's radio runs on its own (beacons, the MP exchange) through the
game's loads and saves, so a console that still runs is never silent for long. 8 s is over seven times that, leaves
room for a loaded PC, and is under the 10 s a player would wait before giving up; a process that ended is not
waited for at all. The other links keep their own times, also within 5-10 s: the UDP link between PCs drops a PC
not heard from for 5 s (`lan_wireless.cpp`, keepalives every 500 ms on their own thread, so a slow game or a long load
does not count), and over the internet mode the far PC counts as gone after 4 s without a datagram (state every
100 ms, pings every 500 ms: at 1 % loss 4 s of silence does not happen while the far game runs) plus 300 frames
before its stand-in is told to leave.

**Seen** (2026-10-27, one PC, `day1done.sav` host AAAAAA, `second_day1.sav` guest NORA, each pair connected with
Connect now, one copy killed with SIGKILL by its PID; harness `/home/claude/run/dc/dcpair.sh`, `killtest.py`,
`dcone.sh` + `reconnect.py` for the restart; screenshots `/home/claude/run/live/shots/dc/`):

| link, copy killed | before | after |
|---|---|---|
| same-PC, host | guest frozen at that frame (6 frames in 60 s) | guest's error within 2 s ("...disconnected due to a communication error.", `samepc_hostkilled_guest_1.png`; another run "Unable to receive data.", `samepc_hostkilled_1_error.png`), A: home, free (`samepc_hostkilled_2_after.png`) |
| same-PC, guest | host at ~7 frames/s for 30 s, then "Unable to receive data." | host at full speed, "Unable to receive data. Terminating connection..." within 5 s (`samepc_guestkilled_host_1.png`), home, free (`..._2.png`) |
| same-PC, host stopped (SIGSTOP 18 s) | guest frozen while stopped | guest "lost" after 8 s, "Unable to receive data.", home; the host on waking "Unable to receive data." too |
| UDP between PCs (no delay), host | (unchanged) guest stalls 5 s, "Unable to receive data.", home | same (`lan_hostkilled_*.png`) |
| UDP between PCs, guest | (unchanged) host stalls 5 s, "Unable to receive data.", home | same (`lan_guestkilled_*.png`) |
| internet mode, 200 ms (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`, 207-250 ms measured), host's copy | (unchanged) | guest: far PC gone after ~5 s, its stand-in host's Disconnect: "The person you were playing with has disconnected." ... "Save complete." (`inet200_hostkilled_*.png`), home, free; the host's stand-in for the guest ended with its player |
| internet mode, 200 ms, guest's copy | (unchanged) | host: "NORA is going home." ~10 s later, still in its session, free (`inet200_guestkilled_3_host.png`) |
| internet mode, 200 ms, the guest PC's stand-in | guest frozen (as same-PC, host killed) | guest's error at once, home; the host "NORA is going home."; a new stand-in started 2 s later |

**The dropped player comes back.** In each case the killed copy was started again from its folder's save and
connected again: same-PC both ways (`samepc_reconnect_after_guestkill.png`, `samepc_reconnect_after_hostkill.png`),
UDP (`lan_reconnect_after_hostkill.png`), internet mode: the host restarted (its new save went to the guest's PC,
which started a new stand-in host; `inet200_reconnect_after_hostkill_b.png`, each console showing the other student)
and the guest restarted into the host's still running session (`inet200_reconnect_after_guestkill.png`). A stand-in
that crashes is started again by its player's copy from the same far save (2 s later, or 30 s if it lasted less than
a minute: no crash loop), so the players connect again without restarting (`inet200_standinkilled_2_reconnected.png`).
At 200 ms with 1 % loss no copy was ever taken for lost while its game ran (6 pairs, ~25 minutes together).

**Left as the game has it**: a host killed mid-visit loses the visit's changes to its town (its save is the connect
save), while a guest that picked something up there keeps it once it saves at home; a DS host switched off does the
same. A debugger pause over 8 s on one copy ends the other's visit.

## How to test

On one machine (`docs/TESTING.md`, "Co-op over the internet on one machine"): `netpair.sh` / `netgo.sh` in
/home/claude/run/live start the two players' copies over UDP on loopback with `--coop-internet` (`NETENV` for both:
`NTR_LAN_DELAY=<ms>[:<jitter>]`, `NTR_LAN_LOSS=<percent>`); each starts its own stand-in (four processes). `fastgo.py`
presses through the intro until Connect now takes over; "coop net: both in the session" in both logs once the swap
runs. Walking: `cross.py`-style checks read the far student's position on the other console from RAM dumps (the
student behind the session's queue at +0x4A04 + slot * 0x50, its +0x2C: x, y, z in the area's units, and its action
+0xA2); a chat and a gift: `udptools/give.py`; the town sync: `MQ_COOP_POKE_FILE` and `udptools/cmpdump.py`; the new
day: both from `day1done.sav` with `--clock 2026-10-28T03:27:00`.

### Results (2026-10-25)

The machine: 2 cores, shared with other test runs (load average 6-10 during these), every copy unpaced with
`NTR_DRAW_EVERY=10`; four processes of ours. Frame rates are the players' copies' (each player's stand-in runs in
step with it).

| network (each way) | connected after start | frame rate (host, guest) | walking | chat, gift | other |
|---|---|---|---|---|---|
| none | 206-300 s | 31, 28 | the other's student follows | - | town sync: hello, whole block |
| 75 ms (RTT ~170 ms measured) | 206 s | 30, 31 | follows | each once, records equal | var pokes both ways, town blocks equal; a guest that quits: its stand-in leaves, the host sees "<name> is going home." |
| 75 ms, 0-20 ms jitter, 5 % loss | 244 s | 34, 36 | follows | each once, records equal | |
| 25 ms, 10 % loss (~10 % of datagrams dropped) | 237 s | 34, 33 | follows | each once, records equal | |
| 75 ms, new day at 3:30 | 214 s | | follows, after the reconnect too | | host's Disconnect, guest "The person you were playing with has disconnected", "Disconnecting." ... "Save complete.", both towns' last day the 28th, both connected again |
| 75 ms, 0-20 ms jitter, 5 % loss (final build, 0234003) | 223 s | 33, 28 | follows | each once, records equal | the guest's game killed: its stand-in gone at once (the death signal), the host's stand-in left, "<name> is going home." |
| 75 ms, 0-20 ms jitter, 10 % loss | 182 s | 33, 36 | follows; the guest warped into town: the host (still in the dorm) has its packets (area 255) and hides it; both in town: each console's copy of the other's student at the other's own position | Friendship cast (75 ms run): once, pair records equal | the guest's game killed (SIGKILL): its stand-in gone 10 s later; the host's stand-in for it left with the game's Disconnect, the host "<name> is going home." 13 s after |

For comparison (docs/COOP.md "What is built"), the radio frames over UDP connected only up to ~2-5 ms each way and
ran at 27 instead of 123 frames a second at 15 ms each way with a longer wait.

### Re-check by real input at 0 and 200 ms (2026-10-27)

Almost every co-op fix of 2026-10-26/27 had been verified over the same-PC link only. Re-checked over the internet
mode on one machine: host `day1done.sav` (AAAAAA), guest `second_day1.sav` (NORA: two different students), visit
markers, Connect now, every item reached by the pad and the touch screen as a player does (the set-up of a date or a
story state by pokes, as in docs/COOP_PLAYTEST.md). The bad network is the link's own test switch on both copies,
applied on send in our UDP path (`NTR_LAN_DELAY=80:40 NTR_LAN_LOSS=1`: 80-120 ms each way, 1 % of the datagrams
dropped; the window's title and the log said **205-228 ms round trip**), and the same items again with no delay (the
log's round trip 11 ms on the loaded machine). Harness outside the repo: `/home/claude/run/live/inetpair.sh [clock]`
(`NETENV`, own binary copy), `inet_all.sh`, `inet_rest.sh`, `inet*.py` wrappers of the earlier drivers; screenshots in
`/home/claude/run/live/shots/inet/` and `shots/cast/inet*fr_*`.

| item | 200 ms RTT | 0 ms | same-PC link |
|---|---|---|---|
| Connect now | works (connected 60-86 s from start, "both in the session" 10-30 s later) | works (42-61 s) | works |
| Chat, a gift (both ways) | works: "You chat with...", "You gave the Beige Wardrobe to NORA!", bags and `.coop` lines equal, once each (`gift_c200_3.png`, `gift_g200_3.png`, `gift_gback_*`) | works (`gift_I8g_3.png`) | works |
| Homeroom (the farewell, Eric) and a lesson | works: both "That's all for homeroom!" with Eric on both, the same classmates at the desks on both consoles (`i2_les_mid.png`) | works (`I8b_hr_mid.png`, `i8_les_mid.png`) | works |
| Walk-in mystery (area 10, 121.4) | works: "asked by slot 0: allowed", the guest asked, Yes, a step beside, both "ended here" (`i2_myst_3_both_in_scene.png`) | works (`I7_myst_*`) | works |
| Friendship at the ruins (the guest transforms, the host says the words and holds the emerald) | **bug, fixed**: "You failed at using your incantation." (`inet2fr_3.png`); after the fix "You succeeded...", "Your Friendship incantation reached NORA!", "You were only slightly excited." on the guest, +2 in both `.coop` files, once (`inet4fr_*`, `i4_fr_after.png`) | works after the fix (`I7fr_*`) | works |
| A telegram (NORA to AAAAAA, Thom, the keyboard by touch) and the mail cat | **bug, fixed**: the entry never reached the host's town, and the host's later field load hung (`hang_e221.png`, `hang2_e221.png`); after: the cat (with a model) by the host's door, "HI", **"From: NORA"** (`I7_cat_from.png`; before the name fix "From: AAAAAA") | works after the fix | works |
| The Gateway to the other world, the tower, back to the dorm | **bug, fixed**: the guest's tower scene waited for ever at "Oh, there you are," (the principal without a model on the guest, `i3_g25_back.png`); after: both through 102.3 into the dorm (`i5_g26_back.png`) | works after the fix (`I8_gate_back.png`) | works |
| The new day (3:50 to 4:01) | works: the host's Disconnect, "Save complete." on both, both connected again, each console's copy of the other student where the other stands (`i6_nd_walk.png`) | (not re-run; worked at 75 ms on 10-25) | works |

| Fishing: the guest's lure asks the host for a fish (`fishdrv.py`, `--clock 2000-07-18T14:00`, the river by the bookshop) | **bug, fixed** (a time-out): the ask answered ("fish 2 is ours (the host says yes)"), "You caught a Colored Carp!" on the guest, but the host took fish 2 back during the catch, "fish 2 is ours again (no word of the guest)"; after: no take-back, "fish 2 given back (point 9)" / "fish 2 is ours again (point 8 -> 9)" on the host (`i11_fish_after.png`) | (not re-run) | works |

The first three bugs were the internet mode's own (its message rules), above in "What crosses"; none was a time-out.
The fourth was a time-out: the host takes back a fish the guest runs when it has heard nothing of it for kStale
frames, but a record that does not change is sent again only every 60 frames, so 90 frames left no room for the
round trip and a resend (more so with the copies unpaced). Raised to 180 frames (3 s at 60 a second) for the fish
(kStale, kAskTimeout) and the insects (kHostStale, kAskTimeout; the same pattern). Insects at 200 ms by pad (the
ground butterfly by the bookshop, `--clock 2000-07-18T14:20`, `inet_ins.sh`): the guest's copy of it followed the
host's as it moved and it flew off as the guest walked up ("insect 0 gone at the host", the game's own flight), so
no catch was reached; the ask itself was not exercised. Again 2026-10-27, by pad, the host near (docs/COOP_PLAYTEST.md, "Insects over the
internet at 200 ms"): the guest's catch of a cicada with the host 3.6 units away went through ("asked the host" ->
"yes, it is theirs (removed here)" -> "You caught a Large Brown Cicada!", in the guest's bag only, gone from the host);
a butterfly's ask was answered no and the insect was then gone on both consoles with nobody holding it (open: which
test said no is not logged); the host's own catch with the guest near was not reached. The classmates' places in town matched on both consoles
(RAM, at 0 and 200 ms). The NPCs' owner messages come every 30 frames against 90 (left). Otherwise nothing timed out at 200 ms: the offers to go along (a minute), the tower's join ("near the other student: joining
at once"), the talk and gift messages, Thom and the cat, the new day's handshake. No action happened twice
(the `.coop` counters and the bags).

Measured (200 ms, 1 % loss, the machine shared; the game's frames per wall second, unpaced):
- **Frame rate**: 130 / 136 (host / guest) at 200 ms against 147 / 140 at 0 ms under the same load (load average 4.4);
  65 / 62 at 200 ms under load 7.4. No wait in our path: the game's radio exchange stays on the PC, a far packet
  that is late only shows the far student a little behind.
- **Movement**: the far player's student is shown about the one-way delay late (100 ms). While the guest walked,
  the host got 1172 movement packets in about 35 s (about 33 a second); 181 of them (15 %) came after a newer one
  (the 0-40 ms jitter reorders) and were dropped as stale, as designed (newest wins), so about 28 a second were
  shown. Standing still, each side sends its last movement twice a second. No rubber-banding was seen: the two
  consoles' copies of each student stood at the same place after every walk (RAM).
- **Bandwidth** (datagrams each player's copy sent on the UDP link, from `lan: testing a bad network: <dropped> of
  <sent>`): about 65-70 a second idle in the session and 50-90 while walking; each is a 28-byte header and 1-24
  bytes (a movement: 51 bytes before UDP/IP), so a few KB/s each way (under 10 KB/s with UDP/IP headers). The save
  (64 KB) crosses once at the start.

Seen beside it, **not from the internet link** (the same on the same-PC link, `samepc_dorm.png`): on the host's
screen the guest's student has the **host's hair colour** in the dorm after connecting (NORA's green hair drawn
orange like AAAAAA's) until the first map change (in the classroom and in town she is green). Only visible with two
different students (every earlier test save held AAAAAA). It comes with 3D on both screens: with
`MQ_COOP_DUAL_3D=0` the same pair shows her green there (`samepc_dorm_nodual.png`); the guest's own screen is right.
The two consoles' records of her are equal (RAM), so it is the drawing, not the data. **Fixed 2026-10-27**: her hair
palette's upload was lost from the game's full VRAM queue while the session waited for her arrival (docs/COOP.md, "3D
on both screens in a session", "The guest's hair colour on the host's screen").
Also not from the link: a student who joins a walk-in mystery with the step beside the other (event 90018) keeps a
top screen that looks at the sky, not at the school (`I7_myst_5_after.png`; the same over the same-PC link,
`/home/claude/run/live/shots/v5/hw_5_after.png`). **Fixed 2026-10-27**: the step left the town's loaded district (its
buildings) as it was; it is now set as walking there would have (docs/COOP.md, "A joiner's town after the step
beside").

## Sources

- melonDS: `third_party/melonDS/src/Wifi.cpp` (`ProcessTX`, `USTimer`, `CheckRX`, `SendMPReply`), `net/LAN.cpp`
  (`MPRecvTimeout = 25`, `RecvReplies`, `RecvHostPacket`), `net/LocalMP.cpp`, `net/Netplay.cpp` (mirror instances,
  input lag 4, unfinished).
- [melonDS multiplayer support (DeepWiki)](https://deepwiki.com/melonDS-emu/melonDS/5.2-multiplayer-support): LAN
  over ENet; netplay queues inputs against jitter and syncs SRAM and savestates to every instance.
