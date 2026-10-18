#include "texture_pack.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include "ntr/hash.hpp"
#include "ntr/runtime.hpp"

namespace texture_pack {
namespace {

struct State {
    std::mutex mutex;
    bool dump = false, load = false;
    int scale = 1;
    std::filesystem::path dump_dir;
    std::map<std::string, std::filesystem::path> files;           // name -> replacement PNG
    std::unordered_map<std::uint64_t, std::vector<unsigned>> cache;  // name hash -> texels at the pack's scale
    std::set<std::string> dumped;
    Status status;
};

State& state() {
    static State s;
    return s;
}

std::string name_of(unsigned w, unsigned h, const unsigned* texels) {
    const std::uint64_t hash = ntr::hash64(texels, std::size_t(w) * h * 4, w * 1000003ull + h);
    char name[64];
    std::snprintf(name, sizeof name, "%ux%u_%016llx", w, h, (unsigned long long)hash);
    return name;
}

// A PNG's size, from its header (no decoding).
bool png_size(const std::filesystem::path& path, unsigned& w, unsigned& h) {
    std::ifstream f(path, std::ios::binary);
    unsigned char b[24];
    if (!f.read(reinterpret_cast<char*>(b), sizeof b) || b[1] != 'P' || b[2] != 'N' || b[3] != 'G') return false;
    w = unsigned(b[16]) << 24 | unsigned(b[17]) << 16 | unsigned(b[18]) << 8 | b[19];
    h = unsigned(b[20]) << 24 | unsigned(b[21]) << 16 | unsigned(b[22]) << 8 | b[23];
    return true;
}

void save(const std::filesystem::path& path, unsigned w, unsigned h, const unsigned* texels) {
    SDL_Surface* s = SDL_CreateSurface(int(w), int(h), SDL_PIXELFORMAT_RGBA32);
    if (!s) return;
    for (unsigned y = 0; y < h; y++) {
        auto* row = static_cast<unsigned char*>(s->pixels) + std::size_t(y) * s->pitch;
        for (unsigned x = 0; x < w; x++) {
            const unsigned t = texels[y * w + x];
            const unsigned r = t & 0x3F, g = (t >> 8) & 0x3F, b = (t >> 16) & 0x3F, a = (t >> 24) & 0x1F;
            row[x * 4 + 0] = (unsigned char)(r << 2 | r >> 4);
            row[x * 4 + 1] = (unsigned char)(g << 2 | g >> 4);
            row[x * 4 + 2] = (unsigned char)(b << 2 | b >> 4);
            row[x * 4 + 3] = (unsigned char)(a ? (a << 3 | a >> 2) : 0);
        }
    }
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    SDL_SavePNG(s, path.string().c_str());
    SDL_DestroySurface(s);
}

// A replacement PNG as texels of out_w x out_h (scaled to it, nearest, if the file is another size).
bool load_png(const std::filesystem::path& path, unsigned out_w, unsigned out_h, std::vector<unsigned>& out) {
    SDL_Surface* raw = SDL_LoadPNG(path.string().c_str());
    if (!raw) return false;
    SDL_Surface* s = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);
    if (!s) return false;
    out.resize(std::size_t(out_w) * out_h);
    for (unsigned y = 0; y < out_h; y++) {
        const auto* row = static_cast<const unsigned char*>(s->pixels) + std::size_t(y * unsigned(s->h) / out_h) * s->pitch;
        for (unsigned x = 0; x < out_w; x++) {
            const unsigned char* p = row + std::size_t(x * unsigned(s->w) / out_w) * 4;
            out[y * out_w + x] = unsigned(p[0] >> 2) | unsigned(p[1] >> 2) << 8 | unsigned(p[2] >> 2) << 16 |
                                 unsigned(p[3] >> 3) << 24;
        }
    }
    SDL_DestroySurface(s);
    return true;
}

int hook(void*, unsigned w, unsigned h, const unsigned* texels, unsigned* out, unsigned out_w, unsigned out_h) {
    State& st = state();
    std::lock_guard lock(st.mutex);
    const std::string name = name_of(w, h, texels);
    if (st.dump && st.dumped.insert(name).second) {
        const auto path = st.dump_dir / (name + ".png");
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            save(path, w, h, texels);
            st.status.dumped++;
        }
    }
    if (!st.load) return 0;
    const auto file = st.files.find(name);
    if (file == st.files.end()) return 0;
    const std::uint64_t key = ntr::hash64(name.data(), name.size());
    auto cached = st.cache.find(key);
    if (cached == st.cache.end() || cached->second.size() != std::size_t(out_w) * out_h) {
        std::vector<unsigned> texels_out;
        if (!load_png(file->second, out_w, out_h, texels_out)) {
            std::fprintf(stderr, "texture pack: cannot read %s\n", file->second.string().c_str());
            st.files.erase(file);
            return 0;
        }
        cached = st.cache.insert_or_assign(key, std::move(texels_out)).first;
    }
    std::copy(cached->second.begin(), cached->second.end(), out);
    st.status.replaced++;
    return 1;
}

}  // namespace

std::filesystem::path dump_dir(const std::filesystem::path& data_dir) { return data_dir / "textures" / "dump"; }
std::filesystem::path load_dir(const std::filesystem::path& data_dir) { return data_dir / "textures" / "load"; }

int configure(const std::filesystem::path& data_dir, bool dump, bool load) {
    State& st = state();
    std::lock_guard lock(st.mutex);
    st.dump = dump;
    st.load = load;
    st.dump_dir = dump_dir(data_dir);
    st.files.clear();
    st.cache.clear();
    st.scale = 1;
    if (load) {
        std::error_code ec;
        const auto dir = load_dir(data_dir);
        std::filesystem::create_directories(dir, ec);
        for (auto it = std::filesystem::recursive_directory_iterator(dir, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            const auto& path = it->path();
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            if (ext != ".png") continue;
            const std::string stem = path.stem().string();
            unsigned tw = 0, th = 0, pw = 0, ph = 0;
            unsigned long long hash = 0;
            if (std::sscanf(stem.c_str(), "%ux%u_%16llx", &tw, &th, &hash) != 3 || !tw || !th) continue;
            if (!png_size(path, pw, ph)) continue;
            // (a replacement is its texture's size times 1, 2 or 4, the same both ways)
            const unsigned k = pw / tw;
            if (pw % tw || ph != th * k || (k != 1 && k != 2 && k != 4)) {
                std::fprintf(stderr, "texture pack: %s is %ux%u, not 1, 2 or 4 times %ux%u: skipped\n",
                             path.string().c_str(), pw, ph, tw, th);
                continue;
            }
            st.files[stem] = path;
            st.scale = std::max(st.scale, int(k));
        }
    }
    if (dump) {
        std::error_code ec;
        std::filesystem::create_directories(st.dump_dir, ec);
    }
    st.status.replacements = int(st.files.size());
    st.status.scale = st.scale;
    ntr::set_texture_hook((dump || !st.files.empty()) ? &hook : nullptr, nullptr);
    if (load || dump)
        std::fprintf(stderr, "texture pack: %d replacements (drawn at %dx)%s\n", int(st.files.size()), st.scale,
                     dump ? ", dumping new textures" : "");
    return st.files.empty() ? 1 : st.scale;
}

Status status() {
    State& st = state();
    std::lock_guard lock(st.mutex);
    return st.status;
}

}  // namespace texture_pack
