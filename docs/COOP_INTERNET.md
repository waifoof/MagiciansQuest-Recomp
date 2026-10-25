# Co-op over the internet

How two players 30-150 ms apart (round trip), with some packet loss, could play a co-op visit; why the PC-to-PC link
of today (`runtime/src/lan_wireless.cpp`) cannot do it; the designs; the one recommended; how to test it.
Research note (2026-10-25); nothing here is built.

## Summary

- **The cause.** The link carries the DS's radio protocol itself (melonDS's Multi-Card Play frames). That protocol
  assumes an exchange done within microseconds, and melonDS emulates it by **stopping the emulation of both
  consoles until the other side's frame arrives**. Every MP turn costs one network round trip of wall time, and a
  reply later than the wait (25 ms) counts as missing. So a link fails above about a 20-25 ms round trip (default
  wait) or runs at about 1000 / RTT frames a second (longer wait). The game itself is not the limit.
- **Answering the MP frames locally from cached data (the "relay") does not work at the frame level.** Tried as an
  experiment (below): with replies answered at once from the newest one received and the guest not blocked, the
  pair **did not connect even with no delay at all**. The WM library's radio layer (the TSF clock the client keeps
  in step with the host, the reply window, the ack bitmap and the retries) depends on the blocking timing.
- **Recommended: a stand-in console on each PC** ("shadow"). Each PC also runs, headless, a copy of the game standing
  in for the far player, connected to the player's own copy over the same-PC link, where the radio exchange takes
  microseconds and never fails. Between the PCs only **game-level data** travels: the far console's 16-byte session
  packets (stood in for by the shadow), our side channel (town sync, pair messages), both of which tolerate latency
  and loss. This is the cached-reply idea moved up one layer, to the game's packets instead of the radio's frames.
- Second choice: **mirror lockstep** (melonDS's own unfinished netplay design: every PC runs every console, only pad
  input travels, with an input delay). No game knowledge needed, but it needs four processes to stay bit-identical
  across two PCs, including the wireless timing between them, which is wall-clock-bound today.

## Why the link of today breaks

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

### D. Stand-in console on each PC, game packets between PCs (recommended)

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

## Recommended: D, step by step

1. **Stand-in mode in the runtime** (`runtime/src/`, a new `standin.cpp` + `include/ntr/standin.hpp`): a flag
   `--standin <peer>` (or env `NTR_STANDIN=1`) that runs headless (no window, no audio, `NTR_DRAW_EVERY=0`-like: no
   drawing), never writes the save file (as `--read-only-save`), and takes its pad from nothing (the student idles).
   The player's copy starts it (process spawn in `game/src/coop_connect.cpp` when an internet session is chosen),
   with `NTR_LINK_NAME=<unique>` so the two share their own same-PC link, and stops it at the end.
2. **Internet link of ours**: reuse `lan_wireless.cpp`'s socket and star (Hello, keepalives, `Coop` datagrams), but
   in internet mode it does **not** carry the game's MP frames (kinds `Frame`, `Begin`, `End`): the game's
   `WirelessLink` is the same-PC link to the stand-in (`wireless_link()` returns the `LocalLink`), and the UDP link
   is only a `CoopTransport` plus a new datagram kind `Packet` (unreliable, latest-wins). Settings: *Co-op between
   PCs* gets *Over the internet*; the host's address and port as now.
3. **Save exchange before the session**: the player's copy sends its save (and `.coop`) to the far PC over the side
   channel (64 KB messages already supported: `coop_channel.cpp`), where it is written to the stand-in's scratch
   folder before the stand-in boots. The host's clock is sent too (`--clock`), so both stand-ins boot on host time.
4. **Packet capture** on each player's copy: a `before` hook at main 0x0202C4CC (the session send; `[0]` type, `[1]`
   slot) copies each packet the game queues (16 bytes, with a frame number) to the UDP link: types 0x0E-0x12 as
   `Packet` (latest-wins), others with `ntr::coop_send` on a new message type (reliable, ordered). Check that 0x0202C4CC
   is the only producer (ov001 0x020F11E4 fills the slot; look for other writers of session+0x4CB0).
5. **Packet substitution** on the stand-in: `instead` hook on main 0x0202C4CC (skip the stand-in's own packet, return
   as if sent) and, once a frame, put the far player's packet into the outgoing slot session+0x4CB0 the way 0x020F11E4
   does (the next reliable one if any is waiting and the slot is free, else the newest movement packet), slot byte
   `[1]` rewritten to the stand-in's own session slot. Hooks in `game/src/cfie_hooks.cpp` (`kHooks`) and
   `game/config/ynne/hooks.txt`; 0x0202C4CC must be in `functions.txt` (it is called, so it should be; a hooks.txt
   change regenerates the game: batch these).
6. **Side channel between the players only**: `coop_sync`, `coop_talk`, `coop_npc` keep using `ntr::coop_send`, whose
   transport becomes the UDP link between H and G (the stand-ins open none: `coop_sync::listen` handlers and the
   frame-end work are off in a stand-in). The session slot each side uses for the other is the stand-in's slot (H
   sees G as G''s slot; G sees H as H''s slot, which is 0, the host's, as now).
7. **Lifetime and failures**: the stand-in leaves when its player leaves (a `Bye`, or nothing heard for 10 s): Select,
   Up, A (`mq_coop_session_pad`'s steps). The player's copy shows "connection lost" through the game's own message.
   The new day (`game/src/coop_session.cpp`) works as now: the host's Disconnect ends the stand-ins' sessions too.
8. **Docs**: COOP.md ("What is built", "Playing together" step 1), TESTING.md (the internet recipe below).

Effort: steps 1-3 are runtime work (a few days); 4-5 one hook batch (one regeneration); 6-7 small. The first thing
to verify (cheap, before step 1): run H and a plain copy G' on the same-PC link with a hook that replaces G''s
packets by a recorded stream of real packets, and check that H shows G' moving as recorded and stays connected.

## How to test

On one machine (`docs/TESTING.md`, "Co-op between PCs on one machine"): four processes (two players over UDP on
loopback with `NTR_LAN_DELAY` / `NTR_LAN_LOSS` on the UDP link, each with its stand-in on its own `NTR_LINK_NAME`);
the machine has two cores, so run the stand-ins with drawing off and stop other runs first.

1. Baseline at no delay: Connect now; both stand in the host's dorm; walk both students: each sees the other move
   (screenshots read at full size); a chat, a gift, Friendship cast together, a ground object picked up by the guest
   (the host's grid and the guest's view agree; RAM dumps of the town block as in TESTING.md).
2. `NTR_LAN_DELAY=25` (50 ms RTT), then `75` (150 ms), then `75:20` (jitter): the same steps; each player's frame rate
   (unpaced fps from `ctl.frame` over time) must equal the no-network rate (123 fps on this machine); the other
   student lags by about the one-way delay.
3. `NTR_LAN_LOSS=5`, then `10`: connected throughout; events (chat, gift, incantation, ground objects) arrive exactly
   once (the gift records, the host's grid); movement may skip.
4. A player stops its copy: the other's stand-in leaves within 10 s, the other copy shows the game's "has
   disconnected" / "is going home" and saves normally; start again and connect again.
5. The new day at 3:30 (the `coop_session` flow) with 150 ms RTT.
6. QUICK regression (nothing changes outside an internet session).

For the record, the measurements this note rests on: docs/COOP.md "What is built" (5 % / 7-10 % loss, 2, 5, 15, 30
ms each way, `NTR_LAN_WAIT_MS=80` 27 fps vs 123) and the relay experiment above (2026-10-25, pairs `rlh`/`rlg`,
port 49611, `day1done.sav`, `--clock 2000-01-18T15:30:00`).

## Sources

- melonDS: `third_party/melonDS/src/Wifi.cpp` (`ProcessTX`, `USTimer`, `CheckRX`, `SendMPReply`), `net/LAN.cpp`
  (`MPRecvTimeout = 25`, `RecvReplies`, `RecvHostPacket`), `net/LocalMP.cpp`, `net/Netplay.cpp` (mirror instances,
  input lag 4, unfinished).
- [melonDS multiplayer support (DeepWiki)](https://deepwiki.com/melonDS-emu/melonDS/5.2-multiplayer-support): LAN
  over ENet; netplay queues inputs against jitter and syncs SRAM and savestates to every instance.
