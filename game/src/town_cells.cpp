// The town's map cells beyond the 4:3 view (docs/PC_VIEW.md, "The wide sides' geometry").
//
// The town (ov002) is a grid of 22 x 26 cells, 4.0 units each (x across, z away from the camera). It builds its
// render objects only for a window of cells around the camera, again whenever the camera enters another cell (the
// town's rebuild, ov002 0x02100278, vtable +144 of its field class, called from its update 0x02101538):
//   1. map cells (buildings, ground pieces; 0x020FFE90) and the items on them (0x02101C54): 4 columns from
//      (camera x - 6.0) / 4.0, the camera's row and the one before it;
//   2. the same, 6 columns from one column further left, the next 5 rows away, each row only the pieces big enough
//      for its distance (a level per row; items in the first 3 of them);
//   3. objects (trees, lamps, furniture; 0x02100028): 6 columns from (camera x - 10.0) / 4.0, 3 rows, the outermost
//      columns only the big ones (an edge flag);
//   4. the same, 8 columns from one column further left, the next 4 rows.
// Those windows fit the DS's 4:3 view, so a widened view found nothing beside it but the renderer's fill (the town
// smeared sideways). With the view wider than 4:3, the rebuild here runs the game's own and then adds the columns
// beside each window through the same functions. Objects and items go into the game's own lists (it updates, draws
// and releases them; their capacity checks are its own), and the objects' edge flag moves to the new outermost columns
// (mq_town_objects). The map cells' list (38 render objects and 38 animation links) is exactly as big as the game's
// windows and always full, so the extra map cells are render objects of ours, in guest memory the console does not
// have (ntr/hook_ram.h): released at the next rebuild, updated after the town's update (mq_town_update) and drawn after
// the town's draw (mq_town_draw) under the same conditions as the game's own (its hardware box test included), and
// forgotten (never touched again) once the town they were built for is gone.
//
// Off with the 4:3 aspect (the game as it is), without "Build the world beyond the view" (the PC view has it on), and
// while a co-op link is set up or the wireless is in a session (the frontend clears cfie::wide_cells): every console
// of a session runs the same game.
//
// Research switch: MQ_CELLS_LOG (each rebuild's windows and the lists' use), MQ_CELLS_EXTRA=<columns> (the same
// number of extra columns per side for every window; 0: none).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "cfie_game.hpp"
#include "ntr/hook_ram.h"
#include "ntr/recomp.h"

namespace cfie {
bool wide_cells = false;
}

namespace {

constexpr uint32_t kRebuild = 0x02100278;  // ov002: the town's cell windows built again
constexpr uint32_t kCell = 0x020FFE90;     // (town, &render objects, &links, cell, [level, row]): a map cell
constexpr uint32_t kItems = 0x02101C54;    // (town, 0, cell, row): the items on a cell
constexpr uint32_t kObjects = 0x02100028;  // (town, &objects, cell, level, [row, edge, limit]): a cell's objects
constexpr uint32_t kCamera = 0x020C6220;   // (town): its camera position (+4 x, +12 z; 20.12)

// The town object's lists: start and the word that holds the end (the next free entry).
constexpr uint32_t kCellObjs = 0x172E4, kCellObjsEnd = 0x19554, kCellObjSize = 232;  // 38 entries
constexpr uint32_t kLinks = 0x19558, kLinksEnd = 0x19980, kLinkSize = 28;           // 38 entries
constexpr uint32_t kObjsEnd = 0x29584;  // (the objects' end word sits right after their 288 entries: also the limit)
constexpr int kGridW = 22, kGridH = 26;
constexpr uint32_t kUpdate = 0x02101538, kDraw = 0x02101724;  // ov002: the town's update and draw (vtable +112, +116)
constexpr uint32_t kObjCtor = 0x0210316C, kLinkCtor = 0x0200D5A4;  // (as the town's constructor builds its lists)
constexpr uint32_t kObjRelease = 0x0200CDE8, kLinkRelease = 0x0200D770, kObjUpdate = 0x0200CD04, kBoxTest = 0x0200CC0C;

// Our map cells: render objects and animation links in the hooks' guest memory.
constexpr int kMine = 48;
constexpr uint32_t kMyCells = NTR_HOOK_RAM, kMyLinks = NTR_HOOK_RAM + kMine * kCellObjSize;
static_assert(kMine * (kCellObjSize + kLinkSize) + 64 <= NTR_HOOK_RAM_SIZE, "hook RAM");

struct Mine {
    uint32_t town = 0;            // the town they were built for (0: none)
    uint32_t cells_end = 0, links_end = 0;
    uint32_t game_end = 0, game_sig = 0;  // the game's list then: its end, its first object's model (still that town?)
} g_mine;

bool g_rebuilding = false;  // inside our rebuild (the game's own and our additions)
bool g_in_game = false;     // the game's own rebuild, called from ours
int g_extra[4] = {};        // extra columns per side for each window
int g_obj_x = 0, g_obj_x4 = 0;  // windows 3 and 4: first column as the game computes it (before clamping)

bool log_on() {
    static const bool on = std::getenv("MQ_CELLS_LOG") != nullptr;
    return on;
}

// A call into the game with arguments on the stack, below `area` (its own words above it stay untouched).
uint32_t call(NtrCpu* c, uint32_t area, uint32_t fn, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3,
              std::initializer_list<uint32_t> stack) {
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    const uint32_t sp = (area - 16) & ~7u;
    uint32_t a = sp;
    for (uint32_t v : stack) ntr_wr32(c, a, v), a += 4;
    c->r[0] = a0, c->r[1] = a1, c->r[2] = a2, c->r[3] = a3, c->r[13] = sp;
    ntr_call(c, fn | 1);
    const uint32_t r0 = c->r[0];
    std::memcpy(c->r, saved, sizeof saved);
    return r0;
}

// A window of columns as the game clamps it to the grid: first column, count.
struct Span {
    int x = 0, n = 0;
};
Span clamp_cols(int x, int n) {
    if (x < 0) n += x, x = 0;
    if (x + n - 1 >= kGridW) n = kGridW - x;
    return {x, n};
}
// ... and rows (counting down from z): first row, count.
Span clamp_rows(int z, int n) {
    if (z >= kGridH) n -= z - (kGridH - 1), z = kGridH - 1;
    if (z - n + 1 < 0) n = z + 1;
    return {z, n};
}
uint32_t cell(int x, int z) { return uint32_t(uint16_t(x)) | uint32_t(uint16_t(z)) << 16; }

}  // namespace

// ov002 0x02100028 (a cell's objects), instead (always runs the body): while the windows are widened, the edge flag
// (only big objects) is on the new outermost columns instead of the game's.
extern "C" int mq_town_objects(NtrCpu* c) {
    if (!g_rebuilding) return 0;
    const int x = int16_t(c->r[2] & 0xFFFF);
    const uint32_t sp = c->r[13];
    const bool far = ntr_rd32(c, sp) >= 3;  // (window 4 passes its rows from 3)
    const int lo = far ? g_obj_x4 - g_extra[3] : g_obj_x - g_extra[2];
    const int hi = far ? g_obj_x4 + 7 + g_extra[3] : g_obj_x + 5 + g_extra[2];
    ntr_wr32(c, sp + 4, x == lo || x == hi ? 1 : 0);
    return 0;
}

namespace {
bool mine_valid(NtrCpu* c, uint32_t town) {
    return g_mine.town && town == g_mine.town && ntr_rd32(c, town + kCellObjsEnd) == g_mine.game_end &&
           ntr_rd32(c, town + kCellObjs + 92) == g_mine.game_sig;
}
uint32_t call1(NtrCpu* c, uint32_t fn, uint32_t a0) { return call(c, c->r[13] - 64, fn, a0, 0, 0, 0, {}); }

// Our map cells released (as the game releases its own at a rebuild: render objects, then links).
void release_mine(NtrCpu* c, uint32_t town) {
    if (mine_valid(c, town)) {
        for (uint32_t o = kMyCells; o < g_mine.cells_end; o += kCellObjSize) call1(c, kObjRelease, o);
        for (uint32_t l = kMyLinks; l < g_mine.links_end; l += kLinkSize) call1(c, kLinkRelease, l);
    }
    g_mine = {};
}
// ... and built afresh (constructed, as the town's constructor does its own: their state from a town that is gone,
// never released, is left behind).
void construct_mine(NtrCpu* c) {
    for (int i = 0; i < kMine; i++) call1(c, kObjCtor, kMyCells + i * kCellObjSize);
    for (int i = 0; i < kMine; i++) call1(c, kLinkCtor, kMyLinks + i * kLinkSize);
}
}  // namespace

// ov002 0x02100278 (the town's cell windows), instead: the game's own, then the columns beside them.
extern "C" int mq_town_cells(NtrCpu* c) {
    if (g_in_game) return 0;
    const uint32_t town = c->r[0];
    if (town + 0x164 != ntr_rd32(c, town + 32)) return 0;  // (its other branch: not the town's grid)
    uint32_t saved[16];
    std::memcpy(saved, c->r, sizeof saved);
    release_mine(c, town);
    std::memcpy(c->r, saved, sizeof saved);
    if (!cfie::wide_cells || cfie::aspect <= 0x1555) return 0;

    // How many columns: the view is wider by aspect / (4:3); each window's half width (in columns) grows by that.
    const double w = cfie::aspect / double(0x1555);
    static const int forced = std::getenv("MQ_CELLS_EXTRA") ? std::atoi(std::getenv("MQ_CELLS_EXTRA")) : -1;
    const double half[4] = {2, 3, 3, 4};
    for (int i = 0; i < 4; i++) g_extra[i] = forced >= 0 ? forced : int(std::ceil(half[i] * (w - 1) - 0.05));

    construct_mine(c);
    std::memcpy(c->r, saved, sizeof saved);
    const uint32_t area = c->r[13] - 32;  // (our words: the lists' ends while we add to them)
    const uint32_t cam = call(c, area, kCamera, town, 0, 0, 0, {});
    const int32_t cx = int32_t(ntr_rd32(c, cam + 4)), cz = int32_t(ntr_rd32(c, cam + 12));
    g_obj_x = (cx - 0xA000) >> 14;
    g_obj_x4 = clamp_cols(g_obj_x, 6).x - 1;

    g_rebuilding = true;
    g_in_game = true;
    ntr_call(c, kRebuild | 1);
    g_in_game = false;
    std::memcpy(c->r, saved, sizeof saved);

    // The lists' ends, in words of ours on the guest stack (the functions take their addresses).
    const uint32_t v_links = area + 4, v_cells = area + 8, v_things = area + 12;
    ntr_wr32(c, v_cells, kMyCells);
    ntr_wr32(c, v_links, kMyLinks);
    ntr_wr32(c, v_things, ntr_rd32(c, town + kObjsEnd));
    const uint32_t start_things = ntr_rd32(c, v_things);
    const uint32_t stack_below = area - 64;  // (our calls' stack, below our words)

    // Windows 1 and 2: map cells, nearest rows first, the columns next to the game's first.
    const Span c1 = clamp_cols((cx - 0x6000) >> 14, 4), r1 = clamp_rows(cz >> 14, 2);
    const Span c2 = clamp_cols(c1.x - 1, 6), r2 = clamp_rows(r1.x - r1.n, 5);
    bool full = false;
    auto add_cell = [&](int x, int z, int level, int row, bool items) {
        if (x < 0 || x >= kGridW || z < 0 || z >= kGridH) return;
        const uint32_t e = ntr_rd32(c, v_cells), l = ntr_rd32(c, v_links);
        if (e + kCellObjSize > kMyCells + kMine * kCellObjSize || l + kLinkSize > kMyLinks + kMine * kLinkSize)
            full = true;
        else
            call(c, stack_below, kCell, town, v_cells, v_links, cell(x, z), {uint32_t(level), uint32_t(row)});
        if (items) call(c, stack_below, kItems, town, 0, cell(x, z), uint32_t(row), {});
    };
    for (int k = 1; k <= std::max(g_extra[0], g_extra[1]); k++) {
        for (int i = 0; i < r1.n && k <= g_extra[0]; i++) {
            add_cell(c1.x - k, r1.x - i, 0, i, true);
            add_cell(c1.x + c1.n - 1 + k, r1.x - i, 0, i, true);
        }
        for (int i = 0; i < r2.n && k <= g_extra[1]; i++) {
            add_cell(c2.x - k, r2.x - i, i, i + 2, i < 3);
            add_cell(c2.x + c2.n - 1 + k, r2.x - i, i, i + 2, i < 3);
        }
    }
    g_mine.town = town;
    g_mine.cells_end = ntr_rd32(c, v_cells);
    g_mine.links_end = ntr_rd32(c, v_links);
    g_mine.game_end = ntr_rd32(c, town + kCellObjsEnd);
    g_mine.game_sig = ntr_rd32(c, town + kCellObjs + 92);

    // Windows 3 and 4: objects, into the game's list (its own capacity check against the limit).
    const Span c3 = clamp_cols(g_obj_x, 6), r3 = clamp_rows(cz >> 14, 3);
    const Span c4 = clamp_cols(c3.x - 1, 8), r4 = clamp_rows(r3.x - r3.n, 4);
    auto add_objects = [&](int x, int z, int level, int row) {
        if (x < 0 || x >= kGridW || z < 0 || z >= kGridH) return;
        call(c, stack_below, kObjects, town, v_things, cell(x, z), uint32_t(level), {uint32_t(row), 0u, town + kObjsEnd});
    };
    for (int k = 1; k <= std::max(g_extra[2], g_extra[3]); k++) {
        for (int i = 0; i < r3.n && k <= g_extra[2]; i++) {
            add_objects(c3.x - k, r3.x - i, 0, i);
            add_objects(c3.x + c3.n - 1 + k, r3.x - i, 0, i);
        }
        for (int i = 0; i < r4.n && k <= g_extra[3]; i++) {
            add_objects(c4.x - k, r4.x - i, i, i + 3);
            add_objects(c4.x + c4.n - 1 + k, r4.x - i, i, i + 3);
        }
    }
    ntr_wr32(c, town + kObjsEnd, ntr_rd32(c, v_things));
    g_rebuilding = false;

    if (log_on()) {
        const uint32_t o = ntr_rd32(c, town + kObjsEnd);
        std::fprintf(stderr,
                     "cells: cam %.2f %.2f extra %d %d %d %d | windows x %d+%d z %d-%d, x %d+%d z %d-%d | cells: game %u, ours %u%s, "
                     "links ours %u | objects %u (game %u)\n",
                     cx / 4096.0, cz / 4096.0, g_extra[0], g_extra[1], g_extra[2], g_extra[3], c1.x, c1.n, r1.x, r1.n, c2.x, c2.n,
                     r2.x, r2.n, (g_mine.game_end - (town + kCellObjs)) / kCellObjSize,
                     (g_mine.cells_end - kMyCells) / kCellObjSize, full ? " (full)" : "", (g_mine.links_end - kMyLinks) / kLinkSize,
                     (o - (town + 0x19984)) / 224, (start_things - (town + 0x19984)) / 224);
    }
    std::memcpy(c->r, saved, sizeof saved);
    return 1;
}

// The game's own function (its hook returns 0 while inside), with the registers it returns.
struct Ran {
    uint32_t saved[16];
    uint32_t result[4];
};
void run_game(NtrCpu* c, uint32_t fn, bool& inside, Ran& ran) {
    std::memcpy(ran.saved, c->r, sizeof ran.saved);
    inside = true;
    ntr_call(c, fn | 1);
    inside = false;
    std::memcpy(ran.result, c->r, sizeof ran.result);
    std::memcpy(c->r, ran.saved, sizeof ran.saved);
}
int done(NtrCpu* c, const Ran& ran) {
    std::memcpy(c->r, ran.saved, sizeof ran.saved);
    std::memcpy(c->r, ran.result, sizeof ran.result);
    return 1;
}

// ov002 0x02101538 (the town's update), instead: the game's own, then our map cells' (as it does its own: each
// render object's step, when the town's flag or the pass says so).
extern "C" int mq_town_update(NtrCpu* c) {
    static bool inside = false;
    if (inside) return 0;
    const uint32_t town = c->r[0];
    Ran ran;
    run_game(c, kUpdate, inside, ran);
    if (cfie::wide_cells && mine_valid(c, town)) {
        const bool pass = int8_t(ntr_rd8(c, 0x020ACA54)) != 0 ? (ntr_rd8(c, 0x020ACA55) & 1) == 0
                                                               : (ntr_rd32(c, town + 0x3E7C) & 1) == 0;
        if (int8_t(ntr_rd8(c, town + 0x31BC2)) != 0 || pass)
            for (uint32_t o = kMyCells; o < g_mine.cells_end; o += kCellObjSize) call1(c, kObjUpdate, o);
    }
    return done(c, ran);
}

// ov002 0x02101724 (the town's draw), instead: the game's own, then our map cells, under its conditions for its own.
extern "C" int mq_town_draw(NtrCpu* c) {
    static bool inside = false;
    if (inside) return 0;
    const uint32_t town = c->r[0];
    Ran ran;
    run_game(c, kDraw, inside, ran);
    const bool valid = mine_valid(c, town);
    int drawn = 0;
    static uint64_t frames = 0;
    frames++;
    if (cfie::wide_cells && valid && g_mine.cells_end != kMyCells && int8_t(ntr_rd8(c, town + 0x3EB3)) != 1 &&
        int8_t(ntr_rd8(c, town + 0x3EB7)) != 2) {
        const bool bottom = (ntr_rd8(c, 0x020ACA55) & 1) != 0;
        const bool dual = int8_t(ntr_rd8(c, 0x020ACA54)) != 0 && bottom;
        auto method = [&](uint32_t o, uint32_t slot) {
            std::memcpy(c->r, ran.saved, sizeof ran.saved);
            c->r[0] = o;
            c->r[13] -= 64;
            ntr_call(c, ntr_rd32(c, ntr_rd32(c, o) + slot));
            const uint32_t r0 = c->r[0];
            std::memcpy(c->r, ran.saved, sizeof ran.saved);
            return r0;
        };
        if (ntr_rd8(c, town + 0x15242) != 0 || bottom) {
            for (uint32_t o = kMyCells; o < g_mine.cells_end; o += kCellObjSize) {
                if (dual) {
                    const int lvl = int8_t(ntr_rd8(c, o + 224));
                    // (as the game: a piece flagged for it is drawn only when the hardware box test passes)
                    const uint32_t box = lvl != 0 ? call1(c, kBoxTest, o) : 1;
                    if (box) method(o, 32), drawn++;
                    continue;
                }
                // (one 3D screen: the game's test of a piece's flags, and of the player's nearness for the ones it hides)
                const uint32_t rec = ntr_rd32(c, town + 0x96C) + ntr_rd16(c, o + 222) * 12;
                const uint8_t flags = ntr_rd8(c, rec + 7);
                bool near = false;
                if (int32_t(ntr_rd32(c, town + 0x3868 + 4)) < 0x2000) {
                    const int32_t z = int32_t(ntr_rd32(c, method(o, 12) + 8));
                    near = std::abs(int32_t(ntr_rd32(c, town + 0x3868 + 8)) - z) < 0x5000 && (flags & 0x80);
                }
                if ((flags & 0x08) || near) method(o, 32), drawn++;
            }
        }
    }
    if (log_on()) {
        if ((frames - 1) % 240 == 0)
            std::fprintf(stderr, "cells draw: town %08x valid %d mine %u drawn %d pass %d dual %d flags %d %d %d\n", town, valid,
                         (g_mine.cells_end - kMyCells) / kCellObjSize, drawn, ntr_rd8(c, 0x020ACA55) & 1, ntr_rd8(c, 0x020ACA54),
                         int8_t(ntr_rd8(c, town + 0x3EB3)), int8_t(ntr_rd8(c, town + 0x3EB7)), ntr_rd8(c, town + 0x15242));
    }
    return done(c, ran);
}
