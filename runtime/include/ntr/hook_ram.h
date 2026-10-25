#pragma once
/* Guest memory the console does not have, for the game's hooks: a block mapped at an address the DS leaves unused
 * (nothing of the game's own is there), read and written by recompiled code like main RAM. Not in save states. Used
 * by game/src/town_cells.cpp (the town's render objects beside the 4:3 view). */
#define NTR_HOOK_RAM 0x0B000000u
#define NTR_HOOK_RAM_SIZE 0x10000u
