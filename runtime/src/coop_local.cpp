// The co-op side channel between copies of the game on one PC (coop_channel.hpp, LocalCoop): a block of shared memory
// of its own beside the game's wireless (local_wireless.cpp), so the game's frames are not touched. One ring of
// datagrams that every copy reads at its own pace (each has its own read position; a copy that falls a whole ring
// behind skips to the present, and the channel above sends again what was lost), and a word of which copies are in
// the game's multiplayer, with their processes, so a copy that quit without saying so is left out.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "coop_channel.hpp"
#include "runtime_internal.hpp"

namespace ntr {

namespace {

constexpr uint32_t kRingSize = 0x80000;
constexpr uint32_t kMagic = 0x51434F50;  // "POCQ"

// The shared memory. Zero (as a new mapping is) is the empty state.
struct Shared {
    uint32_t member;     // bit i: the copy in slot i is in the game's multiplayer
    uint32_t pid[16];    // its process
    uint64_t written;    // bytes ever written to the ring (the write position is this % kRingSize)
    uint8_t ring[kRingSize];
};

struct Record {
    uint32_t magic;
    uint16_t from, to;  // nodes (slot + 1); to 0: everyone
    uint32_t length;
};

uint32_t padded(uint32_t n) { return (n + 3) & ~3u; }

#ifdef _WIN32
bool process_alive(uint32_t pid) {
    HANDLE p = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!p) return GetLastError() != ERROR_INVALID_PARAMETER;
    const bool alive = WaitForSingleObject(p, 0) == WAIT_TIMEOUT;
    CloseHandle(p);
    return alive;
}
uint32_t own_pid() { return GetCurrentProcessId(); }
#else
bool process_alive(uint32_t pid) { return pid && (::kill(pid_t(pid), 0) == 0 || errno == EPERM); }
uint32_t own_pid() { return uint32_t(::getpid()); }
#endif

class LocalCoopImpl final : public LocalCoop {
public:
    ~LocalCoopImpl() override {
        if (shared_) {
            end();
#ifdef _WIN32
            UnmapViewOfFile(shared_);
#else
            ::munmap(shared_, sizeof(Shared));
#endif
        }
#ifdef _WIN32
        if (mutex_) CloseHandle(mutex_);
        if (mapping_) CloseHandle(mapping_);
#else
        if (fd_ >= 0) ::close(fd_);
#endif
    }

    bool open(int slot, const char* name) {
        slot_ = slot;
#ifdef _WIN32
        std::wstring wname;
        for (const char* p = name; *p; ++p) wname += wchar_t(static_cast<unsigned char>(*p));
        mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(Shared), wname.c_str());
        if (!mapping_) return false;
        shared_ = static_cast<Shared*>(MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared)));
        if (!shared_) return false;
        mutex_ = CreateMutexW(nullptr, FALSE, (wname + L"-lock").c_str());
        if (!mutex_) return false;
#else
        fd_ = ::shm_open(name, O_RDWR | O_CREAT, 0600);
        if (fd_ < 0) return false;
        ::flock(fd_, LOCK_EX);
        struct stat st{};
        const bool sized = ::fstat(fd_, &st) == 0 &&
                           (size_t(st.st_size) >= sizeof(Shared) || ::ftruncate(fd_, sizeof(Shared)) == 0);
        ::flock(fd_, LOCK_UN);
        if (!sized) return false;
        void* p = ::mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (p == MAP_FAILED) return false;
        shared_ = static_cast<Shared*>(p);
#endif
        return true;
    }

    uint32_t coop_node() const override { return uint32_t(slot_ + 1); }

    void begin() override {
        lock();
        shared_->pid[slot_] = own_pid();
        shared_->member |= 1u << slot_;
        read_ = shared_->written;  // (what was written before is not for this session)
        unlock();
        in_ = true;
    }

    void end() override {
        if (!in_) return;
        in_ = false;
        lock();
        shared_->member &= ~(1u << slot_);
        unlock();
    }

    void coop_peers(std::vector<uint32_t>& out) override {
        out.clear();
        if (!in_) return;
        lock();
        uint32_t m = shared_->member;
        for (int i = 0; i < 16; i++)
            if ((m & (1u << i)) && i != slot_ && !process_alive(shared_->pid[i])) m &= ~(1u << i);
        shared_->member = m | (1u << slot_);
        unlock();
        for (int i = 0; i < 16; i++)
            if ((m & (1u << i)) && i != slot_) out.push_back(uint32_t(i + 1));
    }

    void coop_send(uint32_t to, const uint8_t* data, size_t len) override {
        if (!in_ || len > kCoopDatagram) return;
        const Record r{kMagic, uint16_t(slot_ + 1), uint16_t(to), uint32_t(len)};
        lock();
        write(&r, sizeof r);
        write(data, uint32_t(len));
        shared_->written += padded(uint32_t(sizeof r + len)) - (sizeof r + len);  // (records start on 4 bytes)
        unlock();
    }

    bool coop_recv(uint32_t& from, std::vector<uint8_t>& data) override {
        if (!in_) return false;
        lock();
        for (;;) {
            const uint64_t written = shared_->written;
            if (read_ == written) break;
            if (written - read_ > kRingSize - 2 * (sizeof(Record) + kCoopDatagram)) {
                // (this copy fell a ring behind: what it missed is gone; the channel sends it again)
                read_ = written;
                break;
            }
            Record r;
            read(&r, sizeof r);
            if (r.magic != kMagic || r.length > kCoopDatagram) {
                read_ = written;
                break;
            }
            data.resize(r.length);
            read(data.data(), r.length);
            read_ += padded(uint32_t(sizeof r + r.length)) - (sizeof r + r.length);
            if (r.from == slot_ + 1 || (r.to != 0 && r.to != slot_ + 1)) continue;  // (ours, or for another copy)
            from = r.from;
            unlock();
            return true;
        }
        unlock();
        return false;
    }

private:
    void write(const void* buf, uint32_t len) {
        const uint32_t off = uint32_t(shared_->written % kRingSize);
        const uint32_t first = len < kRingSize - off ? len : kRingSize - off;
        std::memcpy(&shared_->ring[off], buf, first);
        std::memcpy(shared_->ring, static_cast<const uint8_t*>(buf) + first, len - first);
        shared_->written += len;
    }
    void read(void* buf, uint32_t len) {
        const uint32_t off = uint32_t(read_ % kRingSize);
        const uint32_t first = len < kRingSize - off ? len : kRingSize - off;
        std::memcpy(buf, &shared_->ring[off], first);
        std::memcpy(static_cast<uint8_t*>(buf) + first, shared_->ring, len - first);
        read_ += len;
    }
#ifdef _WIN32
    void lock() { WaitForSingleObject(mutex_, INFINITE); }  // (abandoned by a crashed copy: ours now)
    void unlock() { ReleaseMutex(mutex_); }
    HANDLE mapping_ = nullptr, mutex_ = nullptr;
#else
    void lock() { ::flock(fd_, LOCK_EX); }  // (released by the system if a copy dies)
    void unlock() { ::flock(fd_, LOCK_UN); }
    int fd_ = -1;
#endif
    Shared* shared_ = nullptr;
    int slot_ = 0;
    bool in_ = false;
    uint64_t read_ = 0;
};

}  // namespace

std::unique_ptr<LocalCoop> LocalCoop::open(int slot, const char* name) {
    if (slot < 0 || slot >= 16) return nullptr;
    auto l = std::make_unique<LocalCoopImpl>();
    if (!l->open(slot, name)) {
        log("coop: no shared memory for the side channel\n");
        return nullptr;
    }
    return l;
}

const char* local_coop_name() {
    static const std::string name = [] {
#ifdef _WIN32
        std::string n = "Local\\MagiciansQuest-Recomp-coop-1";
#else
        std::string n = "/MagiciansQuest-Recomp-coop-1";
#endif
        if (const char* own = std::getenv("NTR_LINK_NAME"); own && *own) n += std::string("-") + own;
        return n;
    }();
    return name.c_str();
}

}  // namespace ntr
