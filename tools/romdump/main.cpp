// romdump: write each ARM9 code module of a ROM (main, ITCM, DTCM, overlays) to <out>/<name>.bin for analysis.
// The output is game data: keep it out of the repository.
//
//   romdump <rom.nds> <out dir>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "ntr/rom.hpp"
#include "recomp/recomp.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: romdump <rom.nds> <out dir>\n");
        return 2;
    }
    ntr::Rom rom = ntr::Rom::open(argv[1]);
    recomp::Program p = recomp::load_program(rom);
    std::filesystem::create_directories(argv[2]);
    std::ofstream idx(std::filesystem::path(argv[2]) / "modules.txt");
    for (const recomp::Module& m : p.modules) {
        std::ofstream(std::filesystem::path(argv[2]) / (m.name + ".bin"), std::ios::binary)
            .write(reinterpret_cast<const char*>(m.data.data()), std::streamsize(m.data.size()));
        char line[96];
        std::snprintf(line, sizeof line, "%s %08x %08zx\n", m.name.c_str(), m.base, m.data.size());
        idx << line;
    }
    return 0;
}
