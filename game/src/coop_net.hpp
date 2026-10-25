// Magician's Quest co-op over the internet: the stand-in copy for the far player and the session's packets
// (game/src/coop_net.cpp, docs/COOP_INTERNET.md).
#pragma once

#include <filesystem>
#include <string>

namespace coop_net {

// How this copy was started, so it can start a stand-in the same way (a player's copy over the internet), and the
// clock it booted with ("YYYY-MM-DDTHH:MM:SS").
struct Launch {
    std::filesystem::path exe;
    std::string rom, module;
    std::filesystem::path save;
    std::string clock;
    bool unpaced = false;
};
void configure(const Launch& launch);

// This program's path (argv0 if the system does not say).
std::filesystem::path exe_path(const std::string& argv0);

// A player's copy over the internet: a same-PC link of its own (NTR_LINK_NAME, which its stand-in inherits), named
// after this process. Before the runtime is created.
void use_own_link();

// For the window's title: the other PC, the round trip, the stand-in (empty: not over the internet).
std::string status();

// A stand-in still booting: run it as fast as the PC allows (the frontend skips its pacing).
bool boosting();

// The game is ending: so does the stand-in.
void shutdown();

}  // namespace coop_net
