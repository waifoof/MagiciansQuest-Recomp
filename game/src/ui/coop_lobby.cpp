// The co-op lobby on the start screen (coop_lobby.hpp, docs/COOP_INTERNET.md "The lobby").
//
// The lobby talks over the co-op link itself (ntr::lan_restart opens it as host or guest, with STUN for the connect
// codes; its lobby datagrams, ntr::lan_lobby_send), so the way the lobby found through both routers is the one the
// game then plays over. Messages (each "ML" + a type):
//   'S' state, four times a second: u8 protocol, u8 role (1 host, 2 guest), u8 phase (0 in the lobby, 1 starting,
//       2 leaving), u8 name length, the name, u8 version length, the program's version
//   'P' / 'p' a ping (u64 the sender's clock) / its echo: the round trip
//   'G' / 'g' the host's Play (again every 100 ms) / the guest's answer
// A partner not heard from for 5 s is lost.
#include "coop_lobby.hpp"

#include "app.hpp"
#include "clipboard.hpp"
#include "pane.hpp"
#include "string_button.hpp"
#include "ui.hpp"

#include "../report.hpp"
#include "../settings.hpp"

#include "ntr/lan.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace ui {
namespace {

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

constexpr uint8_t kProto = 1;
constexpr auto kLost = std::chrono::seconds(5);
constexpr char kKeys[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";  // (the codes' letters, connect_code.cpp)
constexpr int kKeyCount = 32, kDel = 32, kClear = 33, kColumns = 8;

uint64_t now_us() {
    return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());
}

void send(char type, const uint8_t* body = nullptr, size_t n = 0) {
    std::vector<uint8_t> m = {'M', 'L', uint8_t(type)};
    if (n) m.insert(m.end(), body, body + n);
    ntr::lan_lobby_send(m.data(), m.size());
}

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    return s;
}

std::string my_name() { return app().settings ? resolved_console_name(*app().settings) : std::string("Player"); }

// The program's version as the lobby compares it (MQ_LOBBY_VERSION replaces it: tests of a mismatch).
std::string my_version() {
    if (const char* e = std::getenv("MQ_LOBBY_VERSION"); e && *e) return e;
    return report::version();
}

int lan_port() { return app().settings ? app().settings->lan_port : 49251; }

// A code that is this PC's own (pasted by mistake: one's own code is in the clipboard after Copy).
bool own_code(const std::string& text) {
    const ntr::LanStatus lan = ntr::lan_status();
    std::string a, b;
    for (char ch : text)
        if (std::isalnum(static_cast<unsigned char>(ch))) a += char(std::toupper(static_cast<unsigned char>(ch)));
    for (char ch : lan.code)
        if (std::isalnum(static_cast<unsigned char>(ch))) b += ch;
    return !b.empty() && a == b;
}

ntr::LanConfig g_before;     // the link's configuration before the lobby opened it (given back when it closes)
bool g_before_kept = false;

const char* kHelpChoose =
    "Play together with one other player, over the internet or on the same network.<br/><br/>"
    "<b>Host a game</b>: your town is the one you both play in. You get a connect code to send to the other player "
    "(a chat message, Discord, Steam...).<br/><br/>"
    "<b>Join a game</b>: paste or type the host's code. On the same network the host is found by itself.<br/><br/>"
    "When the host presses Play, both games start and connect by themselves: you both arrive in the host's dorm, "
    "upstairs by the Gateway. Both need the same version of this program.";

const char* kHelpHost =
    "Send your connect code to the other player; they paste it in their lobby (Join a game).<br/><br/>"
    "If your router's port is open (most routers open it by themselves: UPnP), that is all. If not, the other player "
    "gets a code of their own after pasting yours: paste theirs here (Guest's code), and the two games reach each "
    "other through both routers.<br/><br/>"
    "When they show up below, press Play: both games start, and connect once you are both in your rooms.<br/><br/>"
    "If they never show up: check the codes; open UDP port 49251 on your router (UPnP, or forward it to this PC); or "
    "use a VPN such as Tailscale and give them your VPN address instead of the code.";

const char* kHelpJoin =
    "Paste or type the host's connect code (or the host's address, e.g. a Tailscale address). With a controller: "
    "Enter the code with the controller.<br/><br/>"
    "If the host's port is not open, your own code appears once you have entered theirs: send it to the host, who "
    "pastes it in their lobby. Then wait for the host to press Play.<br/><br/>"
    "On the same network the host is found by itself, with no code.";

}  // namespace

CoopLobby::CoopLobby(std::function<void()> onStart) : mOnStart(std::move(onStart)) {
    app().coop_lobby_open = true;
    add_tab("Co-op lobby", [this](Rml::Element* content) { build(content); });
}

CoopLobby::~CoopLobby() {
    app().coop_lobby_open = false;
    if (!mStarted && mOpened && g_before_kept) ntr::lan_restart(g_before);  // (closed some other way)
}

void CoopLobby::build(Rml::Element* content) {
    auto& left = add_child<Pane>(content, Pane::Type::Controlled);
    auto& right = add_child<Pane>(content, Pane::Type::Uncontrolled);
    const char* help = mMode == Mode::Host ? kHelpHost : mMode == Mode::Join ? kHelpJoin : kHelpChoose;
    const auto show_help = [help](Pane& pane) {
        pane.clear();
        pane.add_rml(help);
    };
    mStatus = nullptr;
    if (mMode == Mode::Choose) {
        if (!mMessage.empty()) mStatus = left.add_text(mMessage);
        left.register_control(left.add_button("Host a game").on_pressed([this] {
            play_sound(kSoundClick);
            choose(Mode::Host);
        }), right, show_help);
        left.register_control(left.add_button("Join a game").on_pressed([this] {
            play_sound(kSoundClick);
            choose(Mode::Join);
        }), right, show_help);
        show_help(right);
        return;
    }
    mStatus = left.add_text("");
    const auto copy_code = [] {
        const ntr::LanStatus lan = ntr::lan_status();
        if (lan.code.empty()) return;
        play_sound(kSoundClick);
        copy_text(lan.code);
        push_toast({"info", "Connect code copied", lan.code, std::chrono::seconds(3)});
    };
    if (mMode == Mode::Host) {
        left.register_control(left.add_select_button({
                                  .key = "Your connect code",
                                  .getValue =
                                      [] {
                                          const ntr::LanStatus lan = ntr::lan_status();
                                          if (!lan.code.empty()) return Rml::String(lan.code);
                                          return Rml::String(lan.active ? "(finding it)" : "(none)");
                                      },
                              }).on_pressed(copy_code),
            right, show_help);
        left.register_control(left.add_select_button({
                                  .key = "Your router",
                                  .getValue =
                                      [] {
                                          const ntr::LanStatus lan = ntr::lan_status();
                                          if (lan.router == 1 || (lan.code.empty() && lan.active)) return Rml::String("checking");
                                          return Rml::String(lan.code_open ? "port open" : "port not open");
                                      },
                              }),
            right, show_help);
        left.register_control(left.add_child<StringButton>(StringButton::Props{
                                  .key = "Guest's code",
                                  .getValue = [this] { return Rml::String(mHostEntry); },
                                  .setValue =
                                      [this](Rml::String value) {
                                          const std::string text = trim(value);
                                          if (text.empty()) return;
                                          if (own_code(text)) {
                                              mMessage = "That is your own code: paste the guest's.";
                                              return;
                                          }
                                          std::string why;
                                          if (ntr::lan_set_peer(text, &why)) mHostEntry = text, mMessage.clear();
                                          else mMessage = "\"" + text + "\" is not a connect code or an address.";
                                      },
                                  .maxLength = 64,
                              }),
            right, show_help);
        left.register_control(left.add_button("Paste the guest's code").on_pressed([this] {
            const std::string text = trim(paste_text());
            if (text.empty()) {
                mMessage = "The clipboard is empty: copy the guest's code first.";
                return;
            }
            play_sound(kSoundClick);
            if (own_code(text)) {
                mMessage = "The clipboard holds your own code: copy the guest's first.";
                return;
            }
            std::string why;
            if (ntr::lan_set_peer(text, &why)) mHostEntry = text, mMessage.clear();
            else mMessage = "The clipboard holds no connect code.";
        }), right, show_help);
        left.register_control(left.add_button("Enter the guest's code with the controller").on_pressed([this] {
            play_sound(kSoundClick);
            push(std::make_unique<CodeKeypad>(mHostEntry, [this](const std::string& text) {
                std::string why;
                if (own_code(text)) mMessage = "That is your own code: enter the guest's.";
                else if (!text.empty() && ntr::lan_set_peer(text, &why)) mHostEntry = text, mMessage.clear();
                else if (!text.empty()) mMessage = "\"" + text + "\" is not a whole connect code.";
            }));
        }), right, show_help);
        left.register_control(left.add_select_button({
                                  .key = "Partner",
                                  .getValue = [this] { return Rml::String(partner_text()); },
                              }),
            right, show_help);
        left.register_control(left.add_button(ControlledButton::Props{
                                                   .text = "Play",
                                                   .isDisabled = [this] { return !partner_here() || !versions_match() || mGoing; },
                                               })
                                  .on_pressed([this] {
                                      if (!partner_here() || !versions_match() || mGoing) return;
                                      play_sound(kSoundPlay);
                                      mGoing = true;
                                      mAcked = false;
                                      mGoSince = Clock::now();
                                  }),
            right, show_help);
        left.register_control(left.add_button("Close the lobby").on_pressed([this] {
            play_sound(kSoundClick);
            leave("");
        }), right, show_help);
    } else {
        left.register_control(left.add_child<StringButton>(StringButton::Props{
                                  .key = "Host's code",
                                  .getValue = [this] { return Rml::String(mHostEntry); },
                                  .setValue = [this](Rml::String value) { join_with(value); },
                                  .maxLength = 64,
                              }),
            right, show_help);
        left.register_control(left.add_button("Paste the host's code").on_pressed([this] {
            const std::string text = trim(paste_text());
            if (text.empty()) {
                mMessage = "The clipboard is empty: copy the host's code first.";
                return;
            }
            play_sound(kSoundClick);
            join_with(text);
        }), right, show_help);
        left.register_control(left.add_button("Enter the host's code with the controller").on_pressed([this] {
            play_sound(kSoundClick);
            push(std::make_unique<CodeKeypad>(mHostEntry, [this](const std::string& text) {
                if (!text.empty()) join_with(text);
            }));
        }), right, show_help);
        left.register_control(left.add_select_button({
                                  .key = "Your connect code",
                                  .getValue =
                                      [this] {
                                          if (!mHostGiven) return Rml::String("(after the host's)");
                                          const ntr::LanStatus lan = ntr::lan_status();
                                          return Rml::String(!lan.code.empty() ? lan.code : "(finding it)");
                                      },
                              }).on_pressed([this, copy_code] {
                                  if (mHostGiven) copy_code();
                              }),
            right, show_help);
        left.register_control(left.add_select_button({
                                  .key = "Host",
                                  .getValue = [this] { return Rml::String(partner_text()); },
                              }),
            right, show_help);
        left.register_control(left.add_button("Leave the lobby").on_pressed([this] {
            play_sound(kSoundClick);
            leave("");
        }), right, show_help);
    }
    show_help(right);
}

void CoopLobby::choose(Mode mode) {
    if (!g_before_kept) {
        g_before = ntr::lan_config();
        g_before_kept = true;
    }
    ntr::LanConfig c;
    c.mode = mode == Mode::Host ? ntr::LanConfig::Mode::Host : ntr::LanConfig::Mode::Join;
    c.port = uint16_t(lan_port());
    c.port_mapping = mode == Mode::Host && (!app().settings || app().settings->coop_upnp);
    c.stun = true;  // (joining with no code: the host is looked for on this network until a code is given)
    std::string why;
    mMessage.clear();
    if (!ntr::lan_restart(c, &why)) {
        mMessage = mode == Mode::Host ? "The co-op port (UDP " + std::to_string(c.port) + ") could not be opened: " + why + "."
                                      : "The network could not be used: " + why + ".";
        mMode = Mode::Choose;
        mRebuild = true;
        return;
    }
    mOpened = true;
    mMode = mode;
    mPartner = 0;
    mPartnerName.clear(), mPartnerVersion.clear();
    mRttMs = -1;
    mHostEntry.clear();
    mHostGiven = false;
    mGoing = mAcked = false;
    mRebuild = true;
}

void CoopLobby::leave(const std::string& why) {
    for (int i = 0; i < 3; i++) send_state(2);
    if (mOpened && g_before_kept) ntr::lan_restart(g_before);
    mOpened = false;
    mMode = Mode::Choose;
    mPartner = 0;
    mMessage = why;
    mRebuild = true;
}

void CoopLobby::join_with(const std::string& entered) {
    const std::string text = trim(entered);
    if (text.empty()) return;
    if (own_code(text)) {
        mMessage = "That is your own code: paste or type the host's.";
        return;
    }
    std::string why;
    if (!ntr::lan_set_peer(text, &why)) {
        mMessage = "\"" + text + "\" is not a connect code or an address (a code is 13 letters and digits, e.g. "
                   "11JR-0E45-W0RXK).";
        return;
    }
    mHostEntry = text;
    mHostGiven = true;
    mMessage.clear();
}

bool CoopLobby::consume_close_request() {
    if (mMode != Mode::Choose) {  // (B or the close button in a lobby: out of the lobby first)
        play_sound(kSoundWindowClose);
        leave("");
        return true;
    }
    if (mOpened && g_before_kept) ntr::lan_restart(g_before);
    mOpened = false;
    return false;
}

void CoopLobby::send_state(uint8_t phase) {
    if (mMode == Mode::Choose) return;
    std::vector<uint8_t> b = {kProto, uint8_t(mMode == Mode::Host ? 1 : 2), phase};
    const std::string name = my_name().substr(0, 40), version = my_version().substr(0, 120);
    b.push_back(uint8_t(name.size()));
    b.insert(b.end(), name.begin(), name.end());
    b.push_back(uint8_t(version.size()));
    b.insert(b.end(), version.begin(), version.end());
    send('S', b.data(), b.size());
}

bool CoopLobby::partner_here() const { return mPartner != 0 && Clock::now() - mHeard < kLost; }

bool CoopLobby::versions_match() const {
    return mPartnerProto == kProto && mPartnerVersion == my_version();
}

std::string CoopLobby::partner_text() const {
    if (!partner_here()) return mMode == Mode::Host ? "(waiting)" : mHostGiven ? "(not reached yet)" : "-";
    std::string t = mPartnerName.empty() ? "(no name)" : mPartnerName;
    if (mRttMs >= 0) t += ", " + std::to_string(mRttMs) + " ms";
    return t;
}

std::string CoopLobby::status_text() const {
    const ntr::LanStatus lan = ntr::lan_status();
    if (mGoing || mStarted) return "Starting...";
    if (partner_here() && !versions_match())
        return mPartnerName + " has version " + mPartnerVersion + "; you have " + my_version() +
               ". Both need the same version to play together: update the older one (Settings > About).";
    if (mMode == Mode::Host) {
        if (partner_here()) return mPartnerName + " is here. Press Play to start: both games load and connect.";
        if (!mMessage.empty()) return mMessage;
        if (!lan.active) return "The co-op port could not be opened.";
        if (lan.code.empty()) return "Finding your connect code...";
        std::string t = "Send your connect code to the other player.";
        t += lan.code_open ? " Your router's port is open: that is all they need."
                           : " Your router's port is not open: once they have pasted yours, they send you theirs: "
                             "paste it in Guest's code.";
        if (lan.punch == 1) t = "Reaching your partner's game through both routers...";
        if (lan.punch == 3) t = "No answer from your partner's game yet (" + lan.punch_text + ").";
        if (!lan.code_note.empty() && !lan.code_open) t += " Note: " + lan.code_note + ".";
        return t;
    }
    if (partner_here()) return "In " + mPartnerName + "'s lobby. Waiting for the host to press Play.";
    if (!mMessage.empty()) return mMessage;
    if (!mHostGiven) return "Paste or type the host's connect code. On the same network the host is found by itself.";
    std::string t = "Reaching the host...";
    if (lan.punch == 3 || (lan.punch == 1 && !lan.code.empty()))
        t = "Send your connect code (below) to the host, who pastes it in their lobby. Waiting for the host's game to "
            "answer.";
    if (lan.punch == 3) t += " (Still no answer: " + lan.punch_text + ".)";
    if (!lan.code_note.empty()) t += " Note: " + lan.code_note + ".";
    return t;
}

void CoopLobby::tick() {
    if (mMode == Mode::Choose) return;
    const auto now = Clock::now();
    uint32_t from = 0;
    std::vector<uint8_t> m;
    while (ntr::lan_lobby_recv(from, m)) {
        if (m.size() < 3 || m[0] != 'M' || m[1] != 'L') continue;
        if (mPartner != 0 && from != mPartner && partner_here()) continue;  // (one partner)
        const uint8_t type = m[2];
        const uint8_t* b = m.data() + 3;
        const size_t n = m.size() - 3;
        if (type == 'S' && n >= 4) {
            const uint8_t role = b[1], phase = b[2];
            if ((mMode == Mode::Host) == (role == 1)) continue;  // (two hosts, or two guests)
            const size_t nl = b[3];
            if (4 + nl + 1 > n) continue;
            const size_t vl = b[4 + nl];
            if (4 + nl + 1 + vl > n) continue;
            const bool fresh = mPartner != from || !partner_here();
            mPartner = from;
            mHeard = now;
            mPartnerProto = b[0];
            mPartnerPhase = phase;
            mPartnerName.assign(reinterpret_cast<const char*>(b + 4), nl);
            mPartnerVersion.assign(reinterpret_cast<const char*>(b + 5 + nl), vl);
            if (fresh) {
                mMessage.clear();
                play_sound(kSoundItemEnable);
                std::fprintf(stderr, "lobby: %s is here (%s, version %s)\n", mPartnerName.c_str(),
                             role == 1 ? "hosting" : "joining", mPartnerVersion.c_str());
            }
            if (phase == 2) {
                std::fprintf(stderr, "lobby: %s %s\n", mPartnerName.c_str(), mMode == Mode::Host ? "left the lobby" : "closed the lobby");
                mMessage = mMode == Mode::Host ? mPartnerName + " left the lobby." : "The host (" + mPartnerName + ") closed the lobby.";
                mPartner = 0;
                if (mMode == Mode::Join) {
                    mHostGiven = false;
                    mHostEntry.clear();
                }
            } else if (phase == 1 && mMode == Mode::Join && versions_match()) {
                start();
                return;
            }
        } else if (type == 'P' && n >= 8) {
            send('p', b, 8);
        } else if (type == 'p' && n >= 8 && from == mPartner) {
            uint64_t t;
            std::memcpy(&t, b, 8);
            mRttMs = int((now_us() - t) / 1000);
        } else if (type == 'G' && mMode == Mode::Join && from == mPartner && versions_match()) {
            for (int i = 0; i < 3; i++) send('g');
            start();
            return;
        } else if (type == 'g' && mMode == Mode::Host && from == mPartner) {
            mAcked = true;
        }
    }
    if (mPartner != 0 && now - mHeard >= kLost) {
        std::fprintf(stderr, "lobby: lost %s (nothing heard for 5 s)\n", mPartnerName.c_str());
        mMessage = "Lost the connection to " + mPartnerName + " (nothing heard for 5 seconds).";
        mPartner = 0;
        mGoing = false;
    }
    if (now - mLastState >= milliseconds(250)) {
        mLastState = now;
        send_state(mGoing ? 1 : 0);
    }
    if (partner_here() && now - mLastPing >= milliseconds(1000)) {
        mLastPing = now;
        const uint64_t t = now_us();
        send('P', reinterpret_cast<const uint8_t*>(&t), 8);
    }
    if (mGoing) {
        send('G');
        if (mAcked || now - mGoSince > milliseconds(2000)) start();
    }
}

void CoopLobby::start() {
    if (mStarted) return;
    mStarted = true;
    for (int i = 0; i < 3; i++) send_state(1);
    const int role = mMode == Mode::Host ? 1 : 2;
    std::fprintf(stderr, "lobby: starting as %s with %s\n", role == 1 ? "host" : "guest", mPartnerName.c_str());
    if (app().coop_lobby_start) app().coop_lobby_start(role);
    pop();
    if (mOnStart) mOnStart();
}

void CoopLobby::update() {
    if (mRebuild) {
        mRebuild = false;
        refresh_active_tab();
        focus();
    }
    tick();
    if (mStatus != nullptr && !mStarted) set_text_content(mStatus, status_text());
    Window::update();
}

// ---- the keypad ----

CodeKeypad::CodeKeypad(std::string start, std::function<void(const std::string&)> onDone)
    : Modal(Props{
          .title = "Enter the code",
          .bodyText = "",
          .actions =
              {
                  ModalAction{.label = "Done",
                              .onPressed =
                                  [this](Modal& modal) {
                                      if (mOnDone) mOnDone(mText);
                                      modal.pop();
                                  }},
                  ModalAction{.label = "Cancel", .onPressed = [](Modal& modal) { modal.pop(); }},
              },
      }),
      mOnDone(std::move(onDone)) {
    for (char ch : start) {
        const char up = char(std::toupper(static_cast<unsigned char>(ch)));
        if (std::strchr(kKeys, up) && up != 0 && mText.size() < 13) mText += up;
    }
    auto* content = mDialog->QuerySelector("modal-content");
    content->SetClass("active", true);
    auto* grid = append(content, "div");
    // (eight keys a row, as the D-pad moves: 8 x (56 + 2 x 3) dp)
    grid->SetAttribute("style", "display: flex; flex-wrap: wrap; justify-content: center; width: 500dp; margin: 8dp auto;");
    for (int i = 0; i < kKeyCount + 2; i++) {
        const Rml::String label = i < kKeyCount ? Rml::String(1, kKeys[i]) : i == kDel ? "Del" : "Clear";
        auto key = std::make_unique<Button>(grid, label);
        key->root()->SetAttribute("style", i < kKeyCount ? "width: 56dp; margin: 3dp; padding: 6dp 0dp;"
                                                         : "width: 130dp; margin: 3dp; padding: 6dp 0dp;");
        key->on_nav_command([this, i](Rml::Event&, NavCommand cmd) {
            if (cmd == NavCommand::Confirm) {
                play_sound(kSoundItemChange);
                type(i);
                return true;
            }
            return move(i, cmd);
        });
        mKeys.push_back(std::move(key));
    }
    show_text();
}

bool CodeKeypad::focus() { return !mKeys.empty() && mKeys.front()->focus(); }

void CodeKeypad::type(int key) {
    if (key < kKeyCount) {
        if (mText.size() < 13) mText += kKeys[key];
    } else if (key == kDel) {
        if (!mText.empty()) mText.pop_back();
    } else {
        mText.clear();
    }
    show_text();
}

void CodeKeypad::show_text() {
    std::string shown;
    for (size_t i = 0; i < 13; i++) {
        if (i == 4 || i == 8) shown += '-';
        shown += i < mText.size() ? mText[i] : '_';
    }
    set_body_text(shown + "   (" + std::to_string(mText.size()) + " of 13)");
}

bool CodeKeypad::move(int from, NavCommand cmd) {
    const int count = int(mKeys.size());
    int to = from;
    if (cmd == NavCommand::Left) to = from - 1;
    else if (cmd == NavCommand::Right) to = from + 1;
    else if (cmd == NavCommand::Up) to = from - kColumns;
    else if (cmd == NavCommand::Down) to = from + kColumns;
    else return false;
    if (to >= count && cmd == NavCommand::Down) {
        // Below the keys: the Done button.
        if (auto* actions = mDialog->QuerySelector("modal-actions"); actions && actions->GetFirstChild()) {
            actions->GetFirstChild()->Focus(true);
            play_sound(kSoundItemFocus);
            return true;
        }
        return true;
    }
    to = std::clamp(to, 0, count - 1);
    if (to != from && mKeys[size_t(to)]->focus()) play_sound(kSoundItemFocus);
    return true;
}

bool CodeKeypad::handle_nav_command(Rml::Event& event, NavCommand cmd) {
    auto* actions = mDialog->QuerySelector("modal-actions");
    if (cmd == NavCommand::Up && actions && actions->Contains(event.GetTargetElement())) {
        if (mKeys.back()->focus()) play_sound(kSoundItemFocus);
        return true;
    }
    return Modal::handle_nav_command(event, cmd);
}

}  // namespace ui
