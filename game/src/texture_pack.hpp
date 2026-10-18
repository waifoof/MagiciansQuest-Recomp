// Texture packs: dumping the game's 3D textures, and drawing replacements in their place (OpenGL renderer).
//
// Each texture the renderer decodes is named by its size and a hash of its pixels: <w>x<h>_<16 hex digits>.png, the
// same whichever palette slot or VRAM bank it came from. With dumping on, each new one is saved once to
// <data folder>/textures/dump/. A pack is a folder of such PNGs (any subfolders) in <data folder>/textures/load/: a
// replacement must be its texture's size times 1, 2 or 4 (the pack's scale is the largest found), and its
// transparent pixels are the ones with alpha below one half. Textures without a replacement look as before.
#pragma once

#include <filesystem>
#include <string>

namespace texture_pack {

struct Status {
    int replacements = 0;  // PNGs found in the load folder
    int scale = 1;         // the size they are drawn at (1, 2 or 4 times the DS's)
    int dumped = 0;        // textures saved to the dump folder this run
    int replaced = 0;      // textures drawn from the pack this run
};

// Scans the load folder (when `load`) and installs the renderer's hook. Call before the renderer starts, and again
// when the settings change (the caller then has the renderer decode every texture again). Returns the scale to
// give the renderer (VideoSettings::texture_scale).
int configure(const std::filesystem::path& data_dir, bool dump, bool load);
Status status();
std::filesystem::path dump_dir(const std::filesystem::path& data_dir);
std::filesystem::path load_dir(const std::filesystem::path& data_dir);

}  // namespace texture_pack
