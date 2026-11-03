// What the UI needs from the program (the program fills this in and the UI reads it): the settings it edits, and what
// to do for the buttons that act on the program.
#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "../input.hpp"
#include "../settings.hpp"
#include "../setup.hpp"

namespace ui {

struct App {
    Settings* settings = nullptr;  // edited in place; the program applies the changes when settings_changed is called
    bool compute_available = false;
    const ::input::Bindings* bindings = nullptr;  // the "Open menu" button and key (settings->controls)

    std::function<void()> settings_changed;
    std::function<void()> save_settings;  // write settings.ini now
    std::function<void()> report_issue;   // "Report an issue": close the UI, then zip the report
    std::function<void()> quit;

    // Setup (the start screen builds the game from the player's ROM).
    std::filesystem::path data_dir, toolchain_dir;
    setup::Status status;                           // refreshed by the start screen after a build
    std::function<bool()> game_running;             // a game is running behind the UI
    std::function<void()> start_game;               // a build finished: start the game (behind the start screen)
    // The co-op lobby's Play (ui/coop_lobby.cpp): over the internet from now on, connecting automatically in this role
    // (1 host, 2 guest) once the student is free to move.
    std::function<void(int role)> coop_lobby_start;
    bool coop_lobby_open = false;  // (a lobby is open: the game is not paused in the background, the lobby must answer)
};

App& app();

}  // namespace ui
