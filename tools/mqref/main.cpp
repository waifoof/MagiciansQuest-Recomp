// mqref: run a ROM on melonDS's plain interpreter (no recompiled code) as a reference for the recompiled game.
// Writes screenshots (both screens stacked, PPM) and a register/palette dump at the frames asked for.
//
//   mqref <rom.nds> <out dir> <frames> [script]
//   script lines: <frame> shot | <frame> press <A|B|X|Y|L|R|Start|Select|Up|Down|Left|Right> [n] | <frame> touch x,y [n]
//   MQREF_SAVE=<file>: start from this save. MQREF_EXPLORE=<seed> MQREF_EXPLORE_TO=<frame>: after the script, explore
//   (see explore() below) and write the whole run as a frontend input script, <out dir>/explore.txt.
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "NDS.h"
#include "NDSCart.h"
#include "Platform.h"
#include "SPI_Firmware.h"
#include "GPU.h"
#include "Savestate.h"
#include <random>
#include <set>
#include <memory>

namespace melonDS::Platform {
struct FileHandle { std::FILE* f; };
struct Thread { std::thread t; };
struct Semaphore { std::mutex m; std::condition_variable cv; int n = 0; };
struct Mutex { std::mutex m; };
void SignalStop(StopReason, void*) {}
std::string GetLocalFilePath(const std::string& f) { return f; }
FileHandle* OpenFile(const std::string& path, FileMode mode) {
    const char* m = (mode & FileMode::Write) ? ((mode & FileMode::Read) ? "r+b" : "wb") : "rb";
    std::FILE* f = std::fopen(path.c_str(), m);
    return f ? new FileHandle{f} : nullptr;
}
FileHandle* OpenLocalFile(const std::string& p, FileMode m) { return OpenFile(p, m); }
bool FileExists(const std::string& n) { return std::filesystem::exists(n); }
bool LocalFileExists(const std::string& n) { return FileExists(n); }
bool CheckFileWritable(const std::string&) { return false; }
bool CheckLocalFileWritable(const std::string&) { return false; }
bool CloseFile(FileHandle* f) { if (f) { std::fclose(f->f); delete f; } return true; }
bool IsEndOfFile(FileHandle* f) { return std::feof(f->f); }
bool FileReadLine(char* s, int n, FileHandle* f) { return std::fgets(s, n, f->f); }
u64 FilePosition(FileHandle* f) { return std::ftell(f->f); }
bool FileSeek(FileHandle* f, s64 o, FileSeekOrigin w) { return !std::fseek(f->f, o, w == FileSeekOrigin::Start ? SEEK_SET : w == FileSeekOrigin::Current ? SEEK_CUR : SEEK_END); }
void FileRewind(FileHandle* f) { std::rewind(f->f); }
u64 FileRead(void* d, u64 s, u64 c, FileHandle* f) { return std::fread(d, s, c, f->f); }
bool FileFlush(FileHandle* f) { return !std::fflush(f->f); }
u64 FileWrite(const void* d, u64 s, u64 c, FileHandle* f) { return std::fwrite(d, s, c, f->f); }
u64 FileWriteFormatted(FileHandle* f, const char* fmt, ...) { va_list a; va_start(a, fmt); int n = std::vfprintf(f->f, fmt, a); va_end(a); return n; }
u64 FileLength(FileHandle* f) { long p = std::ftell(f->f); std::fseek(f->f, 0, SEEK_END); long l = std::ftell(f->f); std::fseek(f->f, p, SEEK_SET); return l; }
void Log(LogLevel, const char* fmt, ...) { va_list a; va_start(a, fmt); std::vfprintf(stderr, fmt, a); va_end(a); }
Thread* Thread_Create(std::function<void()> fn) { auto* t = new Thread; t->t = std::thread(std::move(fn)); return t; }
void Thread_Free(Thread* t) { if (t->t.joinable()) t->t.detach(); delete t; }
void Thread_Wait(Thread* t) { if (t->t.joinable()) t->t.join(); }
Semaphore* Semaphore_Create() { return new Semaphore; }
void Semaphore_Free(Semaphore* s) { delete s; }
void Semaphore_Reset(Semaphore* s) { std::lock_guard l(s->m); s->n = 0; }
void Semaphore_Wait(Semaphore* s) { std::unique_lock l(s->m); s->cv.wait(l, [&] { return s->n > 0; }); s->n--; }
bool Semaphore_TryWait(Semaphore* s, int ms) { std::unique_lock l(s->m); if (!s->cv.wait_for(l, std::chrono::milliseconds(ms), [&] { return s->n > 0; })) return false; s->n--; return true; }
void Semaphore_Post(Semaphore* s, int c) { { std::lock_guard l(s->m); s->n += c; } s->cv.notify_all(); }
Mutex* Mutex_Create() { return new Mutex; }
void Mutex_Free(Mutex* m) { delete m; }
void Mutex_Lock(Mutex* m) { m->m.lock(); }
void Mutex_Unlock(Mutex* m) { m->m.unlock(); }
bool Mutex_TryLock(Mutex* m) { return m->m.try_lock(); }
void Sleep(u64 us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
u64 GetMSCount() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
u64 GetUSCount() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void WriteNDSSave(const u8*, u32, u32, u32, void*) {}
void WriteGBASave(const u8*, u32, u32, u32, void*) {}
void WriteFirmware(const Firmware&, u32, u32, void*) {}
void WriteDateTime(int, int, int, int, int, int, void*) {}
void MP_Begin(void*) {}
void MP_End(void*) {}
int MP_SendPacket(u8*, int, u64, void*) { return 0; }
int MP_RecvPacket(u8*, u64*, void*) { return 0; }
int MP_SendCmd(u8*, int, u64, void*) { return 0; }
int MP_SendReply(u8*, int, u64, u16, void*) { return 0; }
int MP_SendAck(u8*, int, u64, void*) { return 0; }
int MP_RecvHostPacket(u8*, u64*, void*) { return 0; }
u16 MP_RecvReplies(u8*, u64, u16, void*) { return 0; }
int Net_SendPacket(u8*, int, void*) { return 0; }
int Net_RecvPacket(u8*, void*) { return 0; }
void Camera_Start(int, void*) {}
void Camera_Stop(int, void*) {}
void Camera_CaptureFrame(int, u32*, int, int, bool, void*) {}
void Mic_Start(void*) {}
void Mic_Stop(void*) {}
int Mic_ReadInput(s16*, int, void*) { return 0; }
AACDecoder* AAC_Init() { return nullptr; }
void AAC_DeInit(AACDecoder*) {}
bool AAC_Configure(AACDecoder*, int, int) { return false; }
bool AAC_DecodeFrame(AACDecoder*, const void*, int, void*, int) { return false; }
bool Addon_KeyDown(KeyType, void*) { return false; }
void Addon_RumbleStart(u32, void*) {}
void Addon_RumbleStop(void*) {}
float Addon_MotionQuery(MotionQueryType, void*) { return 0; }
DynamicLibrary* DynamicLibrary_Load(const char*) { return nullptr; }
void DynamicLibrary_Unload(DynamicLibrary*) {}
void* DynamicLibrary_LoadFunction(DynamicLibrary*, const char*) { return nullptr; }
}  // namespace melonDS::Platform

static void write_ppm(const std::string& path, const uint32_t* top, const uint32_t* bottom) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n256 384\n255\n";
    for (int s = 0; s < 2; s++)
        for (int i = 0; i < 256 * 192; i++) {
            const uint32_t p = (s ? bottom : top)[i];
            const unsigned char rgb[3] = {uint8_t(p >> 16), uint8_t(p >> 8), uint8_t(p)};
            f.write(reinterpret_cast<const char*>(rgb), 3);
        }
}


// Exploration (MQREF_EXPLORE=<seed>, MQREF_EXPLORE_TO=<frame>): after the script, keep playing by trying several random
// stretches of input from a savestate and keeping the one that shows the most pictures not seen before (a coarse
// thumbnail of both screens), so a run goes much further into the game than a plain random walk. Writes the whole
// input as a frontend input script, <out>/explore.txt (with a `shot` every 1000 frames), and refNNNNN.ppm for those.
using melonDS::u32; using melonDS::u16; using melonDS::u8;
struct Step { int at; std::string cmd; };

static uint64_t thumb_hash(melonDS::NDS& nds) {
    void *t, *b;
    nds.GPU.GetFramebuffers(&t, &b);
    uint64_t h = 1469598103934665603ull;
    for (int s = 0; s < 2; s++) {
        const uint32_t* px = static_cast<const uint32_t*>(s ? b : t);
        for (int by = 0; by < 6; by++)
            for (int bx = 0; bx < 8; bx++) {
                uint32_t sum = 0;
                for (int y = by * 32 + 4; y < by * 32 + 28; y += 6)
                    for (int x = bx * 32 + 4; x < bx * 32 + 28; x += 6) {
                        const uint32_t p = px[y * 256 + x];
                        sum += ((p >> 16) & 0xFF) + ((p >> 8) & 0xFF) + (p & 0xFF);
                    }
                h = (h ^ (sum / 16 / 3 / 48)) * 1099511628211ull;  // 6 levels of brightness per block
            }
    }
    return h;
}

static std::vector<Step> random_stretch(std::mt19937& r, int start, int length) {
    std::vector<Step> out;
    std::uniform_real_distribution<double> u(0, 1);
    const char* dirs[] = {"Left", "Right", "Up", "Down"};
    int f = start;
    while (f + 12 < start + length) {  // (every press and touch ends inside the stretch)
        const double k = u(r);
        if (k < 0.55) {
            const int n = std::min(40 + int(u(r) * 300), start + length - f - 12);  // (released inside the stretch)
            if (n < 10) break;
            out.push_back({f, std::string("press ") + dirs[r() % 4] + " " + std::to_string(n)});
            f += n + 10;
        } else if (k < 0.85) {
            out.push_back({f, "press A 4"});
            f += 30;
        } else if (k < 0.92) {
            out.push_back({f, "press B 4"});
            f += 30;
        } else {
            if (f + 10 > start + length) break;
            out.push_back({f, "touch " + std::to_string(20 + r() % 217) + "," + std::to_string(20 + r() % 153) + " 6"});
            f += 40;
        }
    }
    return out;
}

static const std::map<std::string, int> kKeys = {{"A", 0}, {"B", 1}, {"Select", 2}, {"Start", 3}, {"Right", 4}, {"Left", 5},
    {"Up", 6}, {"Down", 7}, {"R", 8}, {"L", 9}, {"X", 16}, {"Y", 17}};

// Runs frames [from, to) applying `steps` (sorted), calling `sample` every 60 frames.
static void run_steps(melonDS::NDS& nds, int from, int to, const std::vector<Step>& steps,
                      const std::function<void(int)>& sample) {
    int key_until = -1, touch_until = -1;
    size_t i = 0;
    while (i < steps.size() && steps[i].at < from) i++;
    for (int f = from; f < to; f++) {
        if (sample) sample(f);  // (the state after frame f - 1, as the script's `shot` sees it)
        for (; i < steps.size() && steps[i].at == f; i++) {
            std::istringstream ss(steps[i].cmd);
            std::string cmd; ss >> cmd;
            if (cmd == "press") {
                std::string k; int n = 6; ss >> k >> n;
                u32 mask = 0xFFF;
                if (auto ki = kKeys.find(k); ki != kKeys.end()) {
                    const int bit = ki->second;
                    if (bit < 16) mask &= ~(1u << bit); else mask &= ~(1u << (bit - 6));
                }
                nds.SetKeyMask(mask);
                key_until = f + n;
            } else if (cmd == "touch") {
                std::string xy; int n = 6; ss >> xy >> n;
                int x = 0, y = 0; std::sscanf(xy.c_str(), "%d,%d", &x, &y);
                nds.TouchScreen(u16(x), u16(y));
                touch_until = f + n;
            }
        }
        if (f == key_until) nds.SetKeyMask(0xFFF);
        if (f == touch_until) nds.ReleaseScreen();
        nds.RunFrame();
    }
}

static void explore(melonDS::NDS& nds, char** argv, int start, int seed) {
    const char* to_env = std::getenv("MQREF_EXPLORE_TO");
    const int end = to_env ? std::atoi(to_env) : start + 60000;
    const int stretch = 720, tries = 5;
    std::mt19937 r{uint32_t(seed)};
    std::set<uint64_t> seen;
    std::vector<Step> kept;
    const std::string out = argv[2];
    std::ofstream script(out + "/explore.txt");
    const char* prefix_env = std::getenv("MQREF_SHOT_PREFIX");
    const std::string prefix = prefix_env ? prefix_env : out + "/ex_";
    if (argv[4]) {
        std::ifstream in(argv[4]);
        for (std::string l; std::getline(in, l);)
            if (l.find("shot") == std::string::npos) script << l << "\n";
    }
    for (int f = start; f < end; f += stretch) {
        melonDS::Savestate st;
        nds.DoSavestate(&st);
        st.Finish();
        std::vector<uint8_t> state(static_cast<const uint8_t*>(st.Buffer()), static_cast<const uint8_t*>(st.Buffer()) + st.Length());
        std::vector<Step> best;
        int best_score = -1;
        for (int k = 0; k < tries; k++) {
            if (k) {
                melonDS::Savestate ld(state.data(), u32(state.size()), false);
                nds.DoSavestate(&ld);
            }
            std::vector<Step> cand = random_stretch(r, f, stretch);
            std::set<uint64_t> fresh;
            run_steps(nds, f, f + stretch, cand, [&](int sf) {
                if (sf % 60) return;
                const uint64_t h = thumb_hash(nds);
                if (!seen.count(h)) fresh.insert(h);
            });
            const int score = int(fresh.size());
            if (score > best_score) { best_score = score; best = cand; }
        }
        melonDS::Savestate ld(state.data(), u32(state.size()), false);
        nds.DoSavestate(&ld);
        run_steps(nds, f, f + stretch, best, [&](int sf) {
            if (sf % 60 == 0) seen.insert(thumb_hash(nds));
            if (sf % 1000 == 0) {
                void *t, *b;
                nds.GPU.GetFramebuffers(&t, &b);
                char name[64]; std::snprintf(name, sizeof name, "/ref%05d.ppm", sf);
                write_ppm(out + name, static_cast<uint32_t*>(t), static_cast<uint32_t*>(b));
                char shot[512]; std::snprintf(shot, sizeof shot, "%d shot %s%06d.png", sf, prefix.c_str(), sf);
                script << shot << "\n";
            }
        });
        for (const Step& s : best) script << s.at << " " << s.cmd << "\n";
        std::fprintf(stderr, "explore: frame %d, %zu pictures seen, best stretch %d new\n", f + stretch, seen.size(), best_score);
        script.flush();
    }
}

int main(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "usage: mqref <rom.nds> <out dir> <frames> [script]\n"); return 2; }
    std::ifstream rf(argv[1], std::ios::binary);
    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(rf)), {});
    std::filesystem::create_directories(argv[2]);
    const int frames = std::atoi(argv[3]);
    std::multimap<int, std::string> script;
    if (argc > 4) {
        std::ifstream sf(argv[4]);
        std::string line;
        while (std::getline(sf, line)) {
            std::istringstream ss(line);
            int fr; std::string rest;
            if (ss >> fr) { std::getline(ss, rest); script.emplace(fr, rest); }
        }
    }
    melonDS::NDSArgs args;
    args.JIT = std::nullopt;
    auto nds_ptr = std::make_unique<melonDS::NDS>(std::move(args), nullptr);  // too big for the stack
    melonDS::NDS& nds = *nds_ptr;
    melonDS::NDSCart::NDSCartArgs cart_args;
    if (const char* save = std::getenv("MQREF_SAVE")) {
        // A save file to start from (read only: nothing is written back).
        std::ifstream sf(save, std::ios::binary);
        std::vector<uint8_t> sav((std::istreambuf_iterator<char>(sf)), {});
        if (!sav.empty()) {
            cart_args.SRAMLength = u32(sav.size());
            cart_args.SRAM = std::make_unique<u8[]>(sav.size());
            std::memcpy(cart_args.SRAM.get(), sav.data(), sav.size());
        }
    }
    auto cart = melonDS::NDSCart::ParseROM(rom.data(), uint32_t(rom.size()), nullptr, std::move(cart_args));
    nds.SetNDSCart(std::move(cart));
    nds.Reset();
    nds.SetupDirectBoot("game.nds");
    nds.Start();
    static const std::map<std::string, int> keys = {{"A", 0}, {"B", 1}, {"Select", 2}, {"Start", 3}, {"Right", 4}, {"Left", 5},
        {"Up", 6}, {"Down", 7}, {"R", 8}, {"L", 9}, {"X", 16}, {"Y", 17}};
    int key_until = -1, touch_until = -1;
    for (int f = 1; f <= frames; f++) {
        for (auto [it, end] = script.equal_range(f); it != end; ++it) {
            std::istringstream ss(it->second);
            std::string cmd; ss >> cmd;
            if (cmd == "shot") {
                void *t, *b;
                nds.GPU.GetFramebuffers(&t, &b);
                char name[64]; std::snprintf(name, sizeof name, "/ref%05d.ppm", f);
                write_ppm(std::string(argv[2]) + name, static_cast<uint32_t*>(t), static_cast<uint32_t*>(b));
                std::printf("frame %d: ARM9 pc %08x lr %08x DISPCNT %08x/%08x pal0 %04x/%04x\n", f, nds.ARM9.R[15], nds.ARM9.R[14],
                            nds.ARM9Read32(0x04000000), nds.ARM9Read32(0x04001000), nds.ARM9Read16(0x05000000), nds.ARM9Read16(0x05000400));
                std::printf("  VRAMCNT %08x %08x %02x  BGCNT A %08x %08x  B %08x %08x  BLD %04x/%04x BRIGHT %04x/%04x\n",
                            nds.ARM9Read32(0x04000240), nds.ARM9Read32(0x04000244), nds.ARM9Read8(0x04000249),
                            nds.ARM9Read32(0x04000008), nds.ARM9Read32(0x0400000C), nds.ARM9Read32(0x04001008),
                            nds.ARM9Read32(0x0400100C), nds.ARM9Read16(0x04000050), nds.ARM9Read16(0x04001050),
                            nds.ARM9Read16(0x0400006C), nds.ARM9Read16(0x0400106C));
            } else if (cmd == "ram") {
                char name[64]; std::snprintf(name, sizeof name, "/ram%05d.bin", f);
                std::ofstream(std::string(argv[2]) + name, std::ios::binary).write(reinterpret_cast<const char*>(nds.MainRAM), 0x400000);
            } else if (cmd == "press") {
                std::string k; int n = 6; ss >> k >> n;
                u32 mask = 0xFFF;  // released
                if (auto ki = keys.find(k); ki != keys.end()) {
                    int bit = ki->second;
                    if (bit < 16) mask &= ~(1u << bit); else mask &= ~(1u << (bit - 6));
                }
                nds.SetKeyMask(mask);
                key_until = f + n;
            } else if (cmd == "touch") {
                std::string xy; int n = 6; ss >> xy >> n;
                int x = 0, y = 0; std::sscanf(xy.c_str(), "%d,%d", &x, &y);
                nds.TouchScreen(u16(x), u16(y));
                touch_until = f + n;
            }
        }
        {
            static u32 la = 0, lb = 0;
            const u32 a = nds.ARM9Read32(0x04000000), b = nds.ARM9Read32(0x04001000);
            if (a != la || b != lb) std::printf("disp %d %08x %08x\n", f - 1, a, b);
            if (f % 50 == 0) {
                u32 active = 0;
                for (u32 ch = 0; ch < 16; ch++) active |= ((nds.ARM7Read32(0x04000400 + ch * 16) >> 31) & 1) << ch;
                std::printf("arm7 %d pc %08x halted %d ipcfifo7 %04x spu-active %04x SOUNDCNT %04x\n", f, nds.ARM7.R[15],
                            (int)nds.ARM7.Halted, nds.ARM7Read16(0x04000184), active, nds.ARM7Read16(0x04000500));
            }
            la = a; lb = b;
        }
        if (f == key_until) nds.SetKeyMask(0xFFF);
        if (f == touch_until) nds.ReleaseScreen();
        nds.RunFrame();
    }
    if (const char* ex = std::getenv("MQREF_EXPLORE")) explore(nds, argv, frames + 1, std::atoi(ex));
    return 0;
}
