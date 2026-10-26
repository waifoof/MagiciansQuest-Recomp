// Magician's Quest co-op over the internet: the game's side of design D (docs/COOP_INTERNET.md; the runtime's side is
// runtime/src/coop_net.cpp, include/ntr/coop_net.hpp).
//
// Each PC runs the player's copy and, hidden, a stand-in copy for the far player (started here from the far player's
// save), linked by the same-PC link: the game's radio exchange stays on the PC. The session's game packets go through
// the session's receive (ov023 0x0211068C: session, aid, bytes, length; in the session (state +0x6915 == 9) a 16-byte
// packet of type < 28 goes into the sender's inbox (+0x4A18 + aid * 0x50) if it is empty (type 0x0E), 28-50 straight to
// the sender's player (ov001 0x020EFE0C), 51 and up (ground objects, telegrams, incantations) into the inbox for the
// host's arbitration):
// - a stand-in hands every packet its partner (the player's copy on its PC) sends to the far PC (mq_net_recv: taps);
// - a player's copy drops what its stand-in sends (its own packets are the stand-in's, not the far player's) and puts
//   the far player's packets in instead, through the same receive, as if the stand-in had sent them (mq_net_frame):
//   events in order, one into the inbox per frame once the game has taken the last, movement newest-wins.
// So the host's arbitration is the real host's: a guest's request reaches the real host through its stand-in there,
// and the answer comes back the same way; the stand-ins' own answers are dropped.
//
// The stand-in follows its player: it connects in the far player's role while the far player connects or plays (the
// game's own Connect now, coop_connect.cpp), and leaves with the game's Disconnect when the far player is no longer in
// a session (coop_session.cpp). It runs hidden, silent, on a scratch copy of the far player's save that is never
// written back, and ends with this copy of the game (or by itself when it no longer hears from it).
#include "coop_net.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "cfie_game.hpp"
#include "coop_connect.hpp"
#include "ntr/coop_net.hpp"
#include "ntr/lan.hpp"
#include "ntr/recomp.h"
#include "process.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <process.h>
#include <windows.h>
#else
#include <csignal>
#include <sys/prctl.h>
#include <unistd.h>
#endif

namespace coop_session {
void leave(NtrCpu* c);
bool leaving();
void host_gone(NtrCpu* c);
}  // namespace coop_session

namespace {

// The game's addresses (USA, YNNE).
constexpr uint32_t kGame = 0x020ACA58;           // the game object (+0xA28: the wireless session)
constexpr uint32_t kField = 0x020AD7EC;          // the town/field object
constexpr uint32_t kScriptContext = 0x020B8AC0;  // the event script context
constexpr uint32_t kControlled = 0x020FE264;     // ov001: the actor the player controls
constexpr uint32_t kEventRunning = 0x0201DF58;   // main (2): an event runs or is queued
constexpr uint32_t kPad = 0x020ACAA4;            // the game's pad (+4 held, +8 pressed, +0xA repeated)
constexpr uint32_t kTown = 0x020AF020;           // the town save block (+0x2C8 date, +0x2CC hour, +0x2CD minute)
constexpr uint32_t kOverlaySlot = 0x0209C988, kSlotOverlay = 0x020A309C;  // (main 0x02009DD8's table)
constexpr int kSessionOverlay = 23;              // Multi-Card Play's session code
constexpr uint32_t kReceive = 0x0211068C;        // ov023: the session's receive (session, aid, bytes, length)
constexpr uint32_t kSessionState = 0x6915;       // session+: 9 in the session
constexpr uint32_t kInbox = 0x4A18, kSlotSize = 0x50;
constexpr uint8_t kEmpty = 0x0E;
constexpr uint16_t kA = 1u << 0;

bool ram(uint32_t a) { return a >= 0x02000000 && a < 0x023FF000; }

uint32_t call(NtrCpu* c, uint32_t fn, uint32_t a0 = 0) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = a0;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

uint32_t session(NtrCpu* c) {
    const uint32_t game = ntr_rd32(c, kGame);
    return ram(game) ? ntr_rd32(c, game + 0xA28) : 0;
}

bool overlay_loaded(NtrCpu* c, int id) {
    const int8_t slot = int8_t(ntr_rd8(c, kOverlaySlot + uint32_t(id)));
    return slot >= 0 && slot < 8 && int8_t(ntr_rd8(c, kSlotOverlay + uint32_t(slot))) == id;
}

// The session, in its "in the session" state with its code loaded (0 otherwise).
uint32_t live_session(NtrCpu* c) {
    const uint32_t s = session(c);
    if (!ram(s) || ntr_rd8(c, s + kSessionState) != 9 || !overlay_loaded(c, kSessionOverlay)) return 0;
    return s;
}

int own_slot(NtrCpu* c, uint32_t s) {
    const uint32_t vt = ntr_rd32(c, s);
    if (!ram(vt)) return -1;
    return int(call(c, ntr_rd32(c, vt + 0x30), s) & 0xFF);
}

// Free to move: the field up with the student in it, no event running or about to start (as coop_connect.cpp).
bool free_to_move(NtrCpu* c) {
    const uint32_t field = ntr_rd32(c, kField), ctx = ntr_rd32(c, kScriptContext);
    if (!ram(field) || !ram(ctx) || !ram(ntr_rd32(c, kPad)) || !ram(ntr_rd32(c, kControlled))) return false;
    if (ntr_rd8(c, field + 0x3EAE) != 0 || ntr_rd8(c, ctx + 8) != 0 || ntr_rd8(c, ctx + 0x23BC) != 0) return false;
    return call(c, kEventRunning, 2) == 0;
}

bool log_on() {
    static const bool on = std::getenv("MQ_NET_LOG") != nullptr;
    return on;
}
// (research, MQ_NET_PASS=1: a player's copy keeps its stand-in's packets and puts nothing of the far player's in)
bool pass() {
    static const bool on = std::getenv("MQ_NET_PASS") != nullptr;
    return on;
}

coop_net::Launch g_launch;
std::atomic<bool> g_boost{false};  // a stand-in that has not booted yet runs unpaced
std::mutex g_status_mutex;
std::string g_status;  // for the window's title

void set_status(const std::string& s) {
    std::lock_guard lock(g_status_mutex);
    auto words = [](const std::string& t) {  // (the round trip's number left out: logged when the rest changes)
        std::string w;
        for (char ch : t)
            if (ch < '0' || ch > '9') w += ch;
        return w;
    };
    if (words(s) != words(g_status)) std::fprintf(stderr, "coop net: the window's title says: co-op: ..., %s\n", s.c_str());
    g_status = s;
}

// ---- a player's copy ----

struct Player {
    bool offered = false;
    unsigned frames = 0;
    proc::Child child;
    bool launched = false;
    unsigned launches = 0;
    int far_aid = -1;              // the aid the stand-in's packets come with (learned; else the other slot)
    bool injecting = false;
    uint64_t swallowed = 0, injected_events = 0, injected_moves = 0, shared_events = 0;
    bool shared_have = false;      // the far player's shared bytes shown in its stand-in's place
    uint32_t session = 0;          // the session it was ready in
    unsigned settled = 0;          // frames this console has been in the session, its arrival done, free to move
    unsigned standin_standing = 0; // frames the stand-in's student has stood here after its arrival
    unsigned stuck = 0;            // frames the far player's student here was caught in an arrival's walk (or stood)
    bool unstick = false;          // ... the stand-in's own bytes meanwhile
    bool ready = false;            // ... long enough: its stand-in's packets are the far player's from now on
    bool subst = false;            // this console and the far player's both ready: the far player's packets go in
    uint8_t shared[16] = {};
    unsigned blocked = 0;          // frames an event waited for the inbox
    unsigned unlinked = 0;         // frames the far PC was not heard from
    bool together = false;         // the far player's packets went in in this session (subst was on)
    unsigned far_out = 0;          // frames since then the far host, heard from, was not in a session
    bool split = false;            // ... long enough: the far host left this session (its stand-in here stayed)
    std::string clock;             // ours, for the far stand-in
} pl;

std::string clock_now(NtrCpu* c) {
    // The town's own date and time once a game is loaded (the DS's clock as the game keeps it); before that, the
    // clock this copy booted with.
    const uint32_t field = ntr_rd32(c, kField);
    char b[40];
    if (ram(field)) {
        const int y = int8_t(ntr_rd8(c, kTown + 0x2C8)), mo = int8_t(ntr_rd8(c, kTown + 0x2C9)),
                  d = int8_t(ntr_rd8(c, kTown + 0x2CA)), h = ntr_rd8(c, kTown + 0x2CC), mi = ntr_rd8(c, kTown + 0x2CD);
        if (y >= 0 && y < 100 && mo >= 0 && mo < 12 && d >= 0 && d < 31 && h < 24 && mi < 60) {
            std::snprintf(b, sizeof b, "%04d-%02d-%02dT%02d:%02d:00", 2000 + y, mo + 1, d + 1, h, mi);
            return b;
        }
    }
    return g_launch.clock;
}

void launch_standin(const std::vector<uint8_t>& save, const std::string& clock) {
    namespace fs = std::filesystem;
    if (pl.launched) proc::stop(pl.child);
    pl.launched = false;
    std::error_code ec;
    const fs::path dir = g_launch.save.empty() ? fs::temp_directory_path(ec) / "mq-coop-standin"
                                               : fs::path(g_launch.save).parent_path() / "coop-standin";
    fs::create_directories(dir, ec);
    const fs::path save_file = dir / "standin.sav", script = dir / "standin.txt", log = dir / "standin.log";
    {
        std::ofstream f(save_file, std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(save.data()), std::streamsize(save.size()));
        if (!f) {
            std::fprintf(stderr, "coop net: cannot write %s\n", save_file.string().c_str());
            return;
        }
    }
    {
        // Through the title screen as a player taps it (the touch to start, then Continue), then the game.
        std::ofstream f(script, std::ios::trunc);
        f << "1950 touch 128,150 8\n2210 touch 128,55 8\n";
    }
    std::vector<std::string> args{g_launch.rom};
    if (!g_launch.module.empty()) args.insert(args.end(), {"--game-module", g_launch.module});
    args.insert(args.end(), {"--standin", std::to_string(ntr::net_standin_port()), "--save-file", save_file.string(),
                             "--read-only-save", "--no-settings", "--input-script", script.string(), "--ignore-require",
                             "--present-every", "600", "--scale", "1", "--renderer", "software", "--no-interpolate"});
    if (!clock.empty()) args.insert(args.end(), {"--clock", clock});
    if (g_launch.unpaced) args.push_back("--unpaced");
    std::vector<std::pair<std::string, std::string>> env{{"SDL_AUDIO_DRIVER", "dummy"}};
    // (this copy's test drivers are its own: the stand-in connects only when told, coop_net.cpp)
    for (const char* name : {"MQ_COOP_AUTO", "MQ_COOP_CONNECT_FILE", "MQ_WARP_FILE", "MQ_RAM_DUMP", "MQ_POS_FILE",
                             "MQ_COOP_POKE_FILE", "MQ_COOP_CAST_FILE", "MQ_EVENT_FILE"})
        env.push_back({name, ""});
    if (const char* d = std::getenv("NTR_DRAW_EVERY"); !d) env.push_back({"NTR_DRAW_EVERY", "600"});
    std::string error;
    if (!proc::spawn(g_launch.exe, args, env, log, pl.child, error)) {
        std::fprintf(stderr, "coop net: the stand-in did not start: %s\n", error.c_str());
        return;
    }
    pl.launched = true;
    pl.launches++;
    std::fprintf(stderr, "coop net: the stand-in for the far player started (process %lld, log %s, clock %s)\n",
                 pl.child.pid, log.string().c_str(), clock.c_str());
}

const char* standin_state_name(uint8_t s) {
    switch (s) {
    case ntr::NetStandin::Booting: return "starting";
    case ntr::NetStandin::Home: return "ready";
    case ntr::NetStandin::Connecting: return "connecting";
    case ntr::NetStandin::InSession: return "in the session";
    case ntr::NetStandin::Leaving: return "leaving";
    default: return "not running";
    }
}

// Puts one of the far player's packets into the session, as if the stand-in had sent it.
void inject(NtrCpu* c, uint32_t s, uint8_t aid, const ntr::NetPacket& p) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    const uint32_t sp = (c->r[13] - 160) & ~7u;
    const uint32_t buf = sp + 32;
    for (uint32_t i = 0; i < p.len; i++) ntr_wr8(c, buf + i, p.data[i]);
    c->r[13] = sp;
    c->r[0] = s, c->r[1] = aid, c->r[2] = buf, c->r[3] = p.len;
    pl.injecting = true;
    ntr_call(c, kReceive | 1);
    pl.injecting = false;
    std::memcpy(c->r, saved, sizeof saved);
    if (log_on())
        std::fprintf(stderr, "coop net: in from the far player: type %02x (%u bytes) as aid %u\n", p.data[0], p.len, aid);
}

// At the end of a frame: the far player's messages, in order (up to a frame's shared bytes, which wait for the next
// DataSharing step: mq_net_shared).
void player_inject(NtrCpu* c) {
    const uint32_t s = live_session(c);
    if (!s || !pl.subst) {
        if (log_on()) {
            ntr::NetPacket q;
            if (ntr::net_peek_event(q))
                std::fprintf(stderr, "coop net: [%u] dropped the far player's waiting packets (%s)\n", pl.frames,
                             !s ? "no session" : "not both in the session");
        }
        ntr::net_drop_far_packets();
        pl.far_aid = -1;
        pl.blocked = 0;
        pl.shared_have = false;
        pl.stuck = 0, pl.unstick = false;
        return;
    }
    const int own = own_slot(c, s);
    const uint8_t aid = uint8_t(pl.far_aid >= 0 ? pl.far_aid : own == 0 ? 1 : 0);
    const uint32_t inbox = s + kInbox + uint32_t(aid) * kSlotSize;
    bool inbox_used = false;
    ntr::NetPacket p;
    if (log_on() && ntr::net_peek_event(p))
        std::fprintf(stderr, "coop net: [%u] waiting: %s type %02x\n", pl.frames,
                     p.aid & ntr::kNetShared ? "shared" : "message", p.data[0]);
    while (ntr::net_peek_event(p) && !(p.aid & ntr::kNetShared)) {
        const uint8_t type = p.data[0];
        const bool straight = p.len != 16 || (type >= 28 && type < 51);  // (not through the inbox)
        if (!straight) {
            if (inbox_used || ntr_rd8(c, inbox) != kEmpty) {  // (the game has not taken the last one yet)
                if (++pl.blocked == 600) std::fprintf(stderr, "coop net: a packet waits for the inbox (10 s)\n");
                break;
            }
            inbox_used = true;
        }
        inject(c, s, aid, p);
        ntr::net_pop_event();
        pl.injected_events++;
        pl.blocked = 0;
    }
}

void player_frame(NtrCpu* c) {
    ++pl.frames;
    // Our save, for the far PC's stand-in (once it exists on disk; the far PC asks again if it restarts).
    if (!pl.offered && pl.frames % 120 == 1 && !g_launch.save.empty()) {
        std::ifstream f(g_launch.save, std::ios::binary);
        std::vector<uint8_t> save((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (!save.empty()) {
            pl.clock = clock_now(c);
            ntr::net_offer_save(std::move(save), pl.clock);
            pl.offered = true;
        }
    }
    // What we are doing, for the far PC (its stand-in for us follows it).
    const uint32_t s = live_session(c);
    // Ready: in the session with the arrival done (the session's arrival steps over: +0x6917 0, +0x6919 -1; the field
    // loaded in the session) and free to move for two seconds. The arrival itself (the town's transfer, the guest's
    // "I am here" and the host's answers) runs between this console and its stand-in, as a visit's does; from then
    // on, once the far player is ready too, the stand-in's packets are replaced by the far player's.
    const uint32_t field = ntr_rd32(c, kField);
    const bool arrived = s && int8_t(ntr_rd8(c, s + 0x6917)) == 0 && int8_t(ntr_rd8(c, s + 0x6919)) == -1 &&
                         ram(field) && ntr_rd8(c, field + 0x3EAF) != 0;
    // (once ready, ready for the rest of the session: a field changing (a door, a warp) is no new arrival, and the far
    // player's packets must go on: its student is where it is, not where this console's stand-in stands)
    const uint32_t session_now = session(c);
    if (session_now != pl.session)
        pl.session = session_now, pl.settled = 0, pl.ready = false, pl.together = false, pl.far_out = 0, pl.split = false;
    if (!session_now) pl.settled = 0, pl.ready = false;
    else if (!arrived) pl.settled = 0;
    else if (!pl.ready && free_to_move(c) && ++pl.settled >= 120) pl.ready = true;
    // A guest whose host left this session (the new day, its Disconnect) while the stand-in host here stayed in it (it
    // could not leave: this console's session did not answer, a text box open): the far host plays on in a new
    // session of its own, and each console would show the other's packets as if together. Once the far host, heard
    // from, has been out of a session for 3 s after both were in this one, this session is over: this console says
    // it is not in a session (the far PC's stand-in for it leaves the host's new one), shows the stand-in's student
    // again, the stand-in host here is told to leave (the game's "The person you were playing with has
    // disconnected." here), and this console goes home and connects again (coop_session.cpp).
    const ntr::NetFar far_now = ntr::net_far();
    const bool guest_here = ram(session_now) && ntr_rd8(c, session_now + 0x6912) == 0;
    if (pl.subst) pl.together = true;
    if (guest_here && pl.together && far_now.linked && !far_now.in_session) ++pl.far_out;
    else if (far_now.linked && far_now.in_session && !pl.split) pl.far_out = 0;
    if (guest_here && !pl.split && pl.far_out >= 180) {
        pl.split = true;
        std::fprintf(stderr, "coop net: the far host is no longer in this session: it is over here too\n");
    }
    if (pl.split) coop_session::host_gone(c);
    ntr::NetLocal local;
    local.in_session = pl.ready && !pl.split;
    // (in a session, its role by the session's hosting byte +0x6912, which needs no code of the session's overlay: a
    // field loading in between must not read as "left", or the far player's stand-in would leave)
    local.role = ram(session_now) ? (ntr_rd8(c, session_now + 0x6912) ? 1 : 2) : uint8_t(coop_connect::active_role());
    const ntr::NetStandin standin = ntr::net_standin();
    local.standin = standin.state;
    ntr::net_set_local(local);
    // The far player's save: our stand-in for them starts from it.
    std::vector<uint8_t> far_save;
    std::string far_clock;
    if (ntr::net_take_far_save(far_save, far_clock)) launch_standin(far_save, far_clock);
    if (pl.launched && !proc::alive(pl.child)) {
        pl.launched = false;
        std::fprintf(stderr, "coop net: the stand-in ended\n");
    }
    // What our stand-in should do: what the far player does.
    const ntr::NetFar other = ntr::net_far();
    pl.unlinked = other.linked ? 0 : pl.unlinked + 1;
    ntr::NetCommand cmd = ntr::NetCommand::Leave;
    if (other.linked && other.role == 1) cmd = ntr::NetCommand::Host;
    else if (other.linked && other.role == 2) cmd = ntr::NetCommand::Join;
    else if (!other.linked && pl.unlinked < 300) cmd = ntr::NetCommand::None;  // (a short silence: wait)
    if (pl.split) cmd = ntr::NetCommand::Leave;  // (the far host's old session: the stand-in leaves it first)
    ntr::net_standin_command(cmd);
    if (!s) pl.standin_standing = 0;
    const bool subst = pl.ready && !pl.split && other.linked && other.in_session && !pass() &&
                       (pl.subst || pl.standin_standing >= 60);
    if (subst != pl.subst)
        std::fprintf(stderr, "coop net: %s\n", subst ? "both in the session: the far player's packets go in for the stand-in's"
                                                     : "the stand-in's packets are its own again");
    pl.subst = subst;
    player_inject(c);
    // The window's title.
    if (pl.frames % 30 == 0) {
        std::string t;
        if (!other.linked) t = "over the internet: waiting for the other PC";
        else {
            t = "over the internet: other PC " + (other.rtt_ms >= 0 ? std::to_string(other.rtt_ms) + " ms" : std::string("linked"));
            if (other.far_save_percent >= 0 && other.far_save_percent < 100)
                t += ", their save " + std::to_string(other.far_save_percent) + "%";
            t += std::string(", stand-in ") + standin_state_name(standin.state);
            if (pl.split) t += ", the other player left this visit: joining again";
            else if (pl.subst) t += ", playing together";
        }
        set_status(t);
    }
    if (log_on() && pl.frames % 600 == 0)
        std::fprintf(stderr, "coop net: swallowed %llu of the stand-in's messages, put in %llu messages, %llu shared "
                     "events and %llu moves\n",
                     (unsigned long long)pl.swallowed, (unsigned long long)pl.injected_events,
                     (unsigned long long)pl.shared_events, (unsigned long long)pl.injected_moves);
}

// ---- a stand-in ----

struct Standin {
    unsigned frames = 0;
    uint8_t shared[4][16] = {};    // each other console's shared bytes as last handed on
    unsigned shared_age[4] = {};   // frames since
    bool home_once = false;    // was free to move (booted)
    unsigned free = 0;         // frames in a row free to move
    unsigned asked = 0;        // frames since Connect now was last asked for
    uint64_t taps = 0;
    uint8_t state = ntr::NetStandin::Booting;
} si;

void standin_frame(NtrCpu* c) {
    ++si.frames;
    const uint32_t s = live_session(c);
    const bool fr = free_to_move(c);
    si.free = fr ? si.free + 1 : 0;
    if (fr && !si.home_once) {
        si.home_once = true;
        g_boost.store(false, std::memory_order_relaxed);
        std::fprintf(stderr, "coop net: stand-in ready (frame %u)\n", si.frames);
    }
    const coop_connect::Role active = coop_connect::active_role();
    if (!si.home_once) si.state = ntr::NetStandin::Booting;
    else if (coop_session::leaving()) si.state = ntr::NetStandin::Leaving;
    else if (s) si.state = ntr::NetStandin::InSession;
    else if (active != coop_connect::Role::None || ram(session(c))) si.state = ntr::NetStandin::Connecting;
    else si.state = ntr::NetStandin::Home;
    if (si.frames % 15 == 0) ntr::net_report(si.state, si.frames);
    // Its player's copy is gone: so is the stand-in (its save is a scratch copy: nothing to keep).
    if (ntr::net_parent_command() == ntr::NetCommand::Quit || ntr::net_parent_gone()) {
        std::fprintf(stderr, "coop net: the player's copy is gone; the stand-in ends\n");
        std::fflush(stderr);
        std::_Exit(0);
    }
    const ntr::NetCommand cmd = ntr::net_parent_command();
    ++si.asked;
    if ((cmd == ntr::NetCommand::Host || cmd == ntr::NetCommand::Join) && si.state == ntr::NetStandin::Home &&
        si.free > 60 && si.asked > 120) {
        si.asked = 0;
        coop_connect::request(cmd == ntr::NetCommand::Host ? coop_connect::Role::Host : coop_connect::Role::Join);
        std::fprintf(stderr, "coop net: the far player connects as %s: so does the stand-in\n",
                     cmd == ntr::NetCommand::Host ? "host" : "guest");
    }
    if (cmd == ntr::NetCommand::Leave && s && fr && !coop_session::leaving() && si.free > 60) {
        coop_session::leave(c);
        std::fprintf(stderr, "coop net: the far player left the session: the stand-in leaves too\n");
    }
}

}  // namespace

namespace coop_net {

void configure(const Launch& launch) {
    g_launch = launch;
    const ntr::NetConfig& cfg = ntr::net_config();
    if (cfg.standin) {
        g_boost.store(true, std::memory_order_relaxed);
#ifndef _WIN32
        prctl(PR_SET_PDEATHSIG, SIGKILL);  // (the player's copy ended: so does its stand-in, at once: it keeps nothing)
#endif
    }
}

std::filesystem::path exe_path(const std::string& argv0) {
#ifdef _WIN32
    std::wstring path(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, path.data(), DWORD(path.size()));
    if (n > 0 && n < path.size()) return std::filesystem::path(path.substr(0, n));
#else
    std::error_code ec;
    const std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) return p;
#endif
    return std::filesystem::path(argv0);
}

void use_own_link() {
    const char* base = std::getenv("NTR_LINK_NAME");
#ifdef _WIN32
    const std::string name = std::string(base && *base ? base : "net") + "-i" + std::to_string(_getpid());
    _putenv_s("NTR_LINK_NAME", name.c_str());
#else
    const std::string name = std::string(base && *base ? base : "net") + "-i" + std::to_string(getpid());
    setenv("NTR_LINK_NAME", name.c_str(), 1);
#endif
}

bool boosting() { return g_boost.load(std::memory_order_relaxed); }

std::string status() {
    std::lock_guard lock(g_status_mutex);
    return g_status;
}

void shutdown() {
    if (pl.launched) {
        ntr::net_standin_command(ntr::NetCommand::Quit);
        proc::stop(pl.child);
        pl.launched = false;
    }
}

}  // namespace coop_net

// ov023 0x0211068C, the session's receive (r0 session, r1 aid, r2 the bytes, r3 the length). A stand-in hands what
// its partner sends on to the far PC; a player's copy drops what its stand-in sends (the far player's packets are put
// in instead, mq_net_frame). Outside co-op over the internet nothing.
extern "C" int mq_net_recv(NtrCpu* c) {
    const ntr::NetConfig& cfg = ntr::net_config();
    if (!cfg.internet && !cfg.standin) return 0;
    const uint32_t s = c->r[0], data = c->r[2], len = c->r[3];
    if (!ram(s) || !ram(data) || len == 0 || len > sizeof(ntr::NetPacket::data) || ntr_rd8(c, s + kSessionState) != 9)
        return 0;
    // The session's own messages (types 0x2B-0x32: the town's transfer, a console arriving, leaving, the session's
    // steps) stay between this console and its stand-in, which follows its player in and out of the session by itself:
    // the far player's would end or change a session the stand-in is still in (seen: the guest told the host was
    // leaving while its stand-in host still hosted, then "Unable to receive data").
    if (len == 16 && ntr_rd8(c, data) >= 0x2B && ntr_rd8(c, data) <= 0x32) {
        if (log_on())
            std::fprintf(stderr, "coop net: the session's own message, type %02x from aid %u: kept here\n", ntr_rd8(c, data),
                         c->r[1]);
        return 0;
    }
    if (cfg.standin) {
        ntr::NetPacket p;
        p.aid = uint8_t(c->r[1]);
        p.len = uint8_t(len);
        for (uint32_t i = 0; i < len; i++) p.data[i] = uint8_t(ntr_rd8(c, data + i));
        ntr::net_tap(p);
        si.taps++;
        if (log_on())
            std::fprintf(stderr, "coop net: [%u] tap: type %02x %02x %02x %02x (%u bytes) from aid %u\n", si.frames, p.data[0],
                         p.data[1], p.data[2], p.data[3], len, c->r[1]);
        return 0;
    }
    if (pl.injecting) return 0;
    if (log_on())
        std::fprintf(stderr, "coop net: [%u] the stand-in's message: type %02x (%u bytes) from aid %u\n", pl.frames,
                     ntr_rd8(c, data), len, c->r[1]);
    if (!pl.subst) return 0;  // (the arrival: between this console and its stand-in)
    pl.far_aid = int(c->r[1]);
    pl.swallowed++;
    return 1;
}

// ov023 0x0211003C (r0 the session's +0x6940, r1 an aid): the address of that console's 16 bytes in this frame's
// DataSharing step (WM_GetSharedDataAddress), which the session then hands to the console's player (ov001 0x020EFE0C)
// and keeps (ov023 0x021105B8). A stand-in hands its partner's on to the far PC when they change (and twice a second
// as they are); a player's copy shows the far player's in its stand-in's place: its events (actions) in order, one a
// frame, else its newest movement, else the last ones again (the bytes are the same every frame until the next packet,
// as on a DS).
extern "C" int mq_net_shared(NtrCpu* c) {
    const ntr::NetConfig& cfg = ntr::net_config();
    if (!cfg.internet && !cfg.standin) return 0;
    const uint32_t obj = c->r[0], aid = c->r[1];
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    c->r[0] = obj + 0x14A0, c->r[1] = obj + 0x1CC0, c->r[2] = aid;  // (the body: WM_GetSharedDataAddress)
    ntr_call(c, 0x020921E4 | 1);
    const uint32_t ptr = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    c->r[0] = ptr;
    const uint32_t s = obj - 0x6940;
    if (!ram(ptr) || aid > 3 || !ram(s) || ntr_rd8(c, s + kSessionState) != 9) return 1;
    if (int(aid) == own_slot(c, s)) return 1;
    if (cfg.standin) {
        uint8_t now[16];
        for (uint32_t i = 0; i < 16; i++) now[i] = uint8_t(ntr_rd8(c, ptr + i));
        const bool changed = std::memcmp(now, si.shared[aid], 16) != 0;
        if (changed || ++si.shared_age[aid] >= 30) {
            std::memcpy(si.shared[aid], now, 16);
            si.shared_age[aid] = 0;
            ntr::NetPacket p;
            p.aid = uint8_t(aid | ntr::kNetShared);
            p.len = 16;
            std::memcpy(p.data, now, 16);
            ntr::net_tap(p);
            si.taps++;
            if (log_on() && changed)
                std::fprintf(stderr, "coop net: [%u] tap: shared type %02x %02x %02x %02x %02x from aid %u\n", si.frames,
                             now[0], now[1], now[2], now[3], now[4], aid);
        }
        return 1;
    }
    if (int(aid) != (pl.far_aid >= 0 ? pl.far_aid : own_slot(c, s) == 0 ? 1 : 0)) return 1;
    if (log_on()) {
        static uint8_t last[16];
        uint8_t now[16];
        for (uint32_t i = 0; i < 16; i++) now[i] = uint8_t(ntr_rd8(c, ptr + i));
        if (std::memcmp(now, last, 16) != 0)
            std::fprintf(stderr, "coop net: [%u] the stand-in's shared: type %02x %02x %02x %02x %02x\n", pl.frames, now[0],
                         now[1], now[2], now[3], now[4]);
        std::memcpy(last, now, 16);
    }
    if (!pl.subst) {
        // (the stand-in's own student, as this console sees it, has arrived and stands: its bytes are a standing
        // student's, type 0x0F with no action, +3 0. Until then it is still coming out of the Gateway (0x11 with
        // action 7, the arrival's walk), and the far player's packets could not end that walk: the student would
        // stay in it here.)
        // (and its student here is done with the arrival: the action byte +0xA2 of the student (the queue the session
        // hands its packets to, session +0x4A04 + aid * 0x50, is the student's +0xE4) back to 0; the bytes stand a
        // while before the walk out of the Gateway ends)
        const uint32_t queue = ntr_rd32(c, s + 0x4A04 + aid * kSlotSize);
        const bool standing = ntr_rd8(c, ptr) == 0x0F && ntr_rd8(c, ptr + 3) == 0 && ram(queue) &&
                              ntr_rd8(c, queue - 0xE4 + 0xA2) == 0;
        pl.standin_standing = standing ? pl.standin_standing + 1 : 0;
        return 1;
    }
    ntr::NetPacket p;
    if (ntr::net_peek_event(p) && (p.aid & ntr::kNetShared)) {
        ntr::net_pop_event();
        // (the session's own steps, types 0x00-0x0D (a console arriving, leaving): between the far player and its
        // stand-in there, as the session's messages 0x2B-0x32 are. Put in here, the far player's Disconnect reached
        // this console's session as its stand-in's while the stand-in was still in it: the stand-in was dropped at the
        // radio (deauth), never left with the game's Disconnect, and this console never said "<name> is going home."
        // The stand-in follows the far player out by itself.)
        if (p.data[0] < 0x0E) {
            static int said = 0;
            if (++said <= 8)
                std::fprintf(stderr, "coop net: the far player's session step %02x stays between it and its stand-in\n",
                             p.data[0]);
        } else {
            std::memcpy(pl.shared, p.data, 16);
            pl.shared_have = true;
            pl.shared_events++;
            if (log_on()) std::fprintf(stderr, "coop net: shared from the far player: type %02x\n", p.data[0]);
        }
    } else if (ntr::net_take_move(p)) {
        if (log_on() && (p.data[0] != pl.shared[0] || p.data[3] != pl.shared[3]))
            std::fprintf(stderr, "coop net: [%u] move from the far player: type %02x state %02x\n", pl.frames, p.data[0],
                         p.data[3]);
        std::memcpy(pl.shared, p.data, 16);
        pl.shared_have = true;
        pl.injected_moves++;
    }
    if (!pl.shared_have) return 1;  // (nothing from the far player yet: the stand-in's own)
    // (a safety net: the far player's student here caught in the arrival's walk (action 7) while the far player
    // stands: the stand-in's own bytes for a while, which end the walk, then the far player's again)
    const uint32_t queue = ntr_rd32(c, s + 0x4A04 + aid * kSlotSize);
    const uint8_t action = ram(queue) ? ntr_rd8(c, queue - 0xE4 + 0xA2) : 0;
    if (pl.unstick) {
        pl.stuck = action == 0 ? pl.stuck + 1 : 0;
        if (pl.stuck < 30) return 1;
        pl.unstick = false, pl.stuck = 0;
        std::fprintf(stderr, "coop net: the far player's student stands again here\n");
    } else if (action == 7 && pl.shared[0] == 0x0F && pl.shared[3] == 0) {
        if (++pl.stuck >= 300) {
            pl.unstick = true, pl.stuck = 0;
            std::fprintf(stderr, "coop net: the far player's student is caught in an arrival's walk here: the stand-in's "
                                 "own bytes until it stands\n");
            return 1;
        }
    } else {
        pl.stuck = 0;
    }
    // (and the stand-in's own session steps stay as they are: its answers to this console's Disconnect, type 0x04
    // and on, are what this console's session waits for. Written over with the far player's movement, a guest's
    // Disconnect ended in "Unable to disconnect." and "Unable to receive data.")
    if (ntr_rd8(c, ptr) < 0x0E) {
        static int said = 0;
        if (++said <= 8) std::fprintf(stderr, "coop net: the stand-in's session step %02x kept\n", ntr_rd8(c, ptr));
        return 1;
    }
    for (uint32_t i = 0; i < 16; i++) ntr_wr8(c, ptr + i, pl.shared[i]);
    return 1;
}

// ov023 0x0210FFB8 (r0 the sender, r1 the bytes, r2 the length, r3 to whom): what this console sends in the session
// (research: MQ_NET_LOG).
extern "C" void mq_net_sent(NtrCpu* c) {
    if (!log_on() || !ram(c->r[1])) return;
    std::fprintf(stderr, "coop net: sent type %02x (%u bytes) to %04x\n", ntr_rd8(c, c->r[1]), c->r[2], c->r[3]);
}

// Once a frame (from mq_frame_end).
void mq_net_frame(NtrCpu* c) {
    const ntr::NetConfig& cfg = ntr::net_config();
    if (cfg.standin) standin_frame(c);
    else if (cfg.internet) player_frame(c);
}

// From mq_coop_pad: a stand-in presses A through what stops it while it is not free to move (the title's last
// screens, a scene, the messages of a visit's end), unless Connect now is answering the room's questions or a
// Disconnect is under way.
void mq_net_pad(NtrCpu* c, uint32_t pad) {
    if (!ntr::net_config().standin || pad != ntr_rd32(c, kPad) || !ram(pad)) return;
    if (si.frames % 30 != 0 || free_to_move(c) || coop_session::leaving()) return;
    const uint32_t field = ntr_rd32(c, kField), ctx = ntr_rd32(c, kScriptContext);
    if (!ram(field) || !ram(ctx)) return;
    if (coop_connect::active_role() != coop_connect::Role::None && ntr_rd8(c, ctx + 8) != 0) return;  // (the room)
    // Only while a text box shows (its message, main 0x02017C38: -1 none): A anywhere else would be an action of the
    // stand-in's student (examining, talking), which the player's copy would then show as the far player's.
    if (int32_t(ntr_rd32(c, ctx + 0x30 + 0x1470)) < 0) return;
    ntr_wr16(c, pad + 4, uint16_t(ntr_rd16(c, pad + 4) | kA));
    ntr_wr16(c, pad + 8, uint16_t(ntr_rd16(c, pad + 8) | kA));
}
