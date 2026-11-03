// The co-op lobby on the start screen (docs/COOP_INTERNET.md, "The lobby"): before the game starts, one player hosts
// (the lobby shows their connect code), the other joins with it (pasted, typed, or picked letter by letter with a
// controller); both see each other (name, round trip); the host's Play starts both games, connected over the internet
// with "Connect automatically", so both arrive in the host's dorm.
#pragma once

#include "modal.hpp"
#include "window.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui {

class CoopLobby : public Window {
public:
    // onStart: the lobby's Play went through (the game should start: the start screen's Play).
    explicit CoopLobby(std::function<void()> onStart);
    ~CoopLobby() override;

    void update() override;

protected:
    bool consume_close_request() override;

private:
    enum class Mode { Choose, Host, Join };
    using Clock = std::chrono::steady_clock;

    void build(Rml::Element* content);
    void choose(Mode mode);
    void leave(const std::string& why);
    void join_with(const std::string& text);
    void send_state(uint8_t phase);
    void tick();
    void start();
    std::string status_text() const;
    std::string partner_text() const;
    bool partner_here() const;
    bool versions_match() const;

    std::function<void()> mOnStart;
    Mode mMode = Mode::Choose;
    bool mRebuild = false;
    bool mStarted = false;
    bool mOpened = false;           // the link was opened for the lobby (else as the game started)
    std::string mHostEntry;         // joining: the host's code or address as entered
    bool mHostGiven = false;
    std::string mMessage;           // the last thing that happened (left, closed, lost, an error)
    Rml::Element* mStatus = nullptr;
    // The partner (the first other PC heard from in the lobby).
    uint32_t mPartner = 0;
    std::string mPartnerName, mPartnerVersion;
    int mPartnerProto = 0;
    uint8_t mPartnerPhase = 0;
    int mRttMs = -1;
    Clock::time_point mHeard{}, mLastState{}, mLastPing{}, mGoSince{};
    bool mGoing = false;            // hosting: Play pressed, telling the guest
    bool mAcked = false;
};

// Entering a connect code with a controller: the code's 32 letters and digits as a grid (D-pad, A types, B closes).
class CodeKeypad : public Modal {
public:
    CodeKeypad(std::string start, std::function<void(const std::string&)> onDone);
    bool focus() override;

protected:
    bool handle_nav_command(Rml::Event& event, NavCommand cmd) override;

private:
    void type(int key);
    void show_text();
    bool move(int from, NavCommand cmd);

    std::string mText;
    std::function<void(const std::string&)> mOnDone;
    std::vector<std::unique_ptr<Button>> mKeys;
};

}  // namespace ui
