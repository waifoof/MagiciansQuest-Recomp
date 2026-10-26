// State the game hooks (CFIE, CFIP) share with the frontend.
#pragma once

#include <cstdint>
#include <string>

namespace cfie {

struct GameEvents {
    // Background music started for the first time: the title screen (nothing before it plays BGM).
    bool title_bgm_started = false;
};

extern GameEvents events;
// Which game the loaded ROM is: FFCC's hooks and features apply only to it.
enum class Game { Ffcc, MagiciansQuest };
extern Game game;

// Online play: how much the host buffers a guest's inputs (see cfie_set_online_timing).
enum class OnlineBuffer { Low, Medium, Original };
extern OnlineBuffer online_buffer;

// Save reads and writes at once instead of at the DS's pace (see cfie_card_backup_request).
extern bool fast_saving;

// Magician's Quest: how fast dialogue text appears (see mq_text_box_draw).
enum class TextSpeed { Normal, Fast, Instant };
extern TextSpeed text_speed;
// Magician's Quest: how fast you walk, percent (100 as the DS; see mq_walk_speed).
extern int walk_speed;
// Magician's Quest: the 3D view magnified this many hundredths (100: as the game draws it). Widescreen zooms in so
// the DS's own view fills the wide picture's width (see mq_perspective_aspect): set from the frontend.
extern int view_zoom;
// Magician's Quest, PC view (docs/PC_VIEW.md): the field's two screens drawn from one camera, the top one continuing
// the bottom one upward (see mq_view_camera). Set from the frontend (the PC view layout, or MQ_PC_VIEW=1).
extern bool pc_view;
// ... and whether its camera acted in the game's last 3D pass (the field's own camera outdoors; not indoors, in menus
// without the field, cutscenes: see mq_pcv_field). The frontend shows the PC view's joined picture only while it does,
// else the bottom screen alone.
extern bool pc_view_acting;
// The town's map cells and objects built beside the 4:3 view when the view is wider (game/src/town_cells.cpp): set by
// the frontend (with "Build the world beyond the view" or the PC view, never while a co-op link is set up).
extern bool wide_cells;
// ... the PC view's presenter: how many times the game has drawn a text box (it draws an open one every frame, see
// mq_text_box_draw), and whether a menu over the field is open (see mq_pcv_menu). Safe to call from any thread.
uint64_t text_box_draws();
bool pc_view_menu();
// How many frames the game's keyboard (ov035) has run its touch handling: it is up while this counts.
uint32_t keyboard_frames();
// ... and the school bell's composer (ov013), its piano.
uint32_t bell_frames();
// Magician's Quest: the Gateway's communications room (multiplayer) from the first day, not after the first lesson.
extern bool coop_anytime;
// Magician's Quest co-op: 3D on both screens in a session (the town on the top screen, as alone) instead of the DS's
// session pattern there. Set by the frontend from its setting (online.coop_top_3d) only outside a session, so a field
// keeps one answer while it is up (MQ_COOP_DUAL_3D=0 / 1 overrides it).
extern bool coop_top_3d;
// Magician's Quest co-op: the file beside the save that keeps the pair records of students who played together
// (game/src/coop_talk.cpp); empty: kept for the session only. Set by the frontend with the save's path.
extern std::string coop_pairs_path;

// Analog movement (see cfie_pack_input): when this console sends its stick in its input records. Every console of
// this build moves a character by the stick its records carry, whatever its own setting, so all stay in step.
// The 3D projection's aspect ratio (width over height, 4.12 fixed point): 0x1555 is the DS's 4:3.
extern uint16_t aspect;
enum class AnalogMovement { Off, SinglePlayer, Multiplayer };
extern AnalogMovement analog_movement;

// The left stick as the frontend last read it, x to the right and y down, its length how far it is pushed past the
// deadzone (0 to 1); zero when it is not.
struct Stick {
    float x = 0, y = 0;
};
extern Stick stick;

// Menu sounds: asks the game to play its sound effect `id` (its own number for it, as the game's menus pass it). The
// effect starts at the game's next sound update, on the game's own thread; it is dropped if the game doesn't get there
// within a third of a second. Safe to call from any thread.
void queue_sound(uint16_t id);

// Logs the id of every sound effect the game starts (to stderr), to learn the numbers (--log-sounds).
extern bool log_sounds;

// The spell element (the six gems of the bottom screen, top to bottom: 0 to 5). request_spell asks for one; the game's
// own pad handler applies it at its next tick, the way R + D-pad does, so it only happens while the field's HUD is
// live and the game decides what is allowed. Safe to call from any thread.
void request_spell(int element);
// The element the game has now (-1 until its HUD has run once), and how many ticks the HUD's pad handler has run:
// a counter that moves means the player is on the field with the spell bar.
int spell_element();
uint64_t spell_ticks();

// The cutscene the game is playing (its movie object, movie/movie%03d.mods): which one (the number in the file name) and the
// number of the frame the game has decoded last (0 is the first). The picture on the DS's screen is that frame. Safe to
// call from any thread; `playing` is false between movies.
struct MovieState {
    bool playing = false;
    int id = 0;
    int frame = 0;
};
MovieState movie();

// Logs each frame of a cutscene the game decodes (to stderr), to learn the numbers (--log-movies).
extern bool log_movies;
// Debugging: every cutscene the game opens is this movie instead (0: as the game asks).
extern int force_movie;

// Picks the release the hooks' addresses are for: "CFIE" (USA, the default) or "CFIP" (Europe). False for any other
// code. Call it before the game starts.
bool select_release(const std::string& game_code);

// The hooks of game/config/<release>/hooks.txt by name, for the game module (null for a name that is not one).
void* find_hook(const char* name);

}  // namespace cfie
