// Magician's Quest co-op: "Connect now" (docs/COOP.md, "Connecting at once"). Starts the game's own Multi-Card Play
// visit, as host or as guest, from wherever the student stands, with no walk to the Gateway and no menus to answer.
#pragma once

#include <cstdint>

namespace coop_connect {

enum class Role : uint8_t { None = 0, Host = 1, Join = 2 };

// The settings: the role "Connect automatically" connects as once the student is free to move after loading (None:
// off), and the role of Co-op between PCs (lan_mode: 0 off, 1 host, 2 join), which the hotkey falls back on.
void configure(int auto_role, int lan_mode);

// The hotkey (Connect now): connects as the configured role (Connect automatically's, else Co-op between PCs', else
// as host), as soon as the student is free to move. Pressed again while it waits or connects, it is called off.
void request_now();

// Connect now in a given role, as soon as the student is free to move (nothing if a request is already waiting).
// Used after a visit ended for the new day (game/src/coop_session.cpp).
void request(Role role);

}  // namespace coop_connect
