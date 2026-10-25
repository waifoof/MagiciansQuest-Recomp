// Co-op over the internet (include/ntr/coop_net.hpp, docs/COOP_INTERNET.md): the far player's game packets, its
// save and its state, between the two players' copies over the PC-to-PC link (lan_wireless.cpp, datagrams of kind
// Net), and the link between a player's copy and its stand-in on the same PC (UDP on 127.0.0.1).
//
// Between the PCs (the first byte of each Net datagram):
//   'R' the reliable stream (coop_channel.cpp's CoopChannel: in order, resent until acknowledged); message type 1 is
//       one of the game's packets: u32 tap number, u8 aid, u8 length, the bytes
//   'M' a movement packet, newest wins: u32 tap number, u8 aid, u8 length, the bytes
//   'S' the sender's state (NetLocal): u8 role, u8 in session, u8 its stand-in
//   'P' / 'Q' ping (u64 the sender's clock) / its echo: the round trip
//   'F' a piece of the sender's save: u32 version, u32 size, u32 offset, u8 clock length, the clock, up to 1024 bytes
//   'f' which pieces of a save arrived: u32 version, u32 size, a bitmap of the pieces
// With the stand-in (its first byte): 'T' a tap (as 'M' after the byte), 'r' its report (u8 state, u32 frame),
// 'c' a command (u8 NetCommand), repeated twice a second (it is how the stand-in knows its player is there).
#include "ntr/coop_net.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using net_socket_t = SOCKET;
static constexpr net_socket_t kNoNetSocket = INVALID_SOCKET;
static void close_net_socket(net_socket_t s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using net_socket_t = int;
static constexpr net_socket_t kNoNetSocket = -1;
static void close_net_socket(net_socket_t s) { ::close(s); }
#endif

#include "coop_channel.hpp"
#include "net_port.hpp"
#include "runtime_internal.hpp"

namespace ntr {

namespace {

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

NetConfig g_net;

constexpr size_t kPiece = 1024;              // a save's piece
constexpr size_t kPiecesInFlight = 64;       // a save's pieces sent and not yet acknowledged
constexpr auto kLinkedFor = milliseconds(4000);  // a far PC not heard from this long is not linked
constexpr auto kStandinGone = milliseconds(4000);
constexpr auto kParentGone = milliseconds(15000);
constexpr uint8_t kEventMessage = 1;

void put32(uint8_t* p, uint32_t v) { std::memcpy(p, &v, 4); }
uint32_t get32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
uint64_t now_us() {
    return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());
}
bool is_move(const NetPacket& p) {
    return (p.aid & kNetShared) && p.len == 16 && p.data[0] >= 0x0E && p.data[0] <= 0x12;
}

uint32_t fnv(const std::vector<uint8_t>& v) {
    uint32_t h = 2166136261u;
    for (uint8_t b : v) h = (h ^ b) * 16777619u;
    return h ^ uint32_t(v.size());
}

// A UDP socket on 127.0.0.1 (port 0: any).
net_socket_t loopback_socket(uint16_t port, uint16_t* bound) {
#ifdef _WIN32
    static bool started = [] {
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }();
    if (!started) return kNoNetSocket;
#endif
    net_socket_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == kNoNetSocket) return s;
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(port);
    if (::bind(s, reinterpret_cast<sockaddr*>(&a), sizeof a) != 0) {
        close_net_socket(s);
        return kNoNetSocket;
    }
    int size = 1 << 20;
    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size), sizeof size);
#ifdef _WIN32
    DWORD timeout = 5;
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof timeout);
#else
    timeval timeout{0, 5000};
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif
    if (bound) {
        sockaddr_in got{};
#ifdef _WIN32
        int len = sizeof got;
#else
        socklen_t len = sizeof got;
#endif
        ::getsockname(s, reinterpret_cast<sockaddr*>(&got), &len);
        *bound = ntohs(got.sin_port);
    }
    return s;
}

// The reliable stream's transport: Net datagrams starting with 'R', to the far PCs heard from lately.
class StreamTransport final : public CoopTransport {
public:
    explicit StreamTransport(NetPort* port) : port_(port) {}
    uint32_t coop_node() const override { return port_->net_node(); }
    void coop_peers(std::vector<uint32_t>& out) override { out = peers; }
    void coop_send(uint32_t to, const uint8_t* data, size_t len) override {
        uint8_t buf[kNetDatagram];
        if (len + 1 > sizeof buf) return;
        buf[0] = 'R';
        std::memcpy(buf + 1, data, len);
        port_->net_send(to, buf, len + 1);
    }
    bool coop_recv(uint32_t& from, std::vector<uint8_t>& data) override {
        if (in.empty()) return false;
        from = in.front().first;
        data = std::move(in.front().second);
        in.pop_front();
        return true;
    }
    std::vector<uint32_t> peers;
    std::deque<std::pair<uint32_t, std::vector<uint8_t>>> in;

private:
    NetPort* port_;
};

struct SaveOut {  // our save on its way to one far PC
    uint32_t version = 0, size = 0;
    std::vector<uint8_t> acked;  // per piece
    std::vector<Clock::time_point> sent;
    int percent = -1;
};
struct SaveIn {   // a far PC's save on its way here
    uint32_t version = 0, size = 0;
    std::vector<uint8_t> bytes, have;
    size_t pieces_had = 0;
    std::string clock;
    bool done = false;
    Clock::time_point last_ack{};
};

struct Far {
    Clock::time_point heard{};
    NetLocal state;
    uint32_t move_seq = 0;
    int rtt_ms = -1;
    SaveOut save_out;
    SaveIn save_in;
};

class Net {
public:
    // ---- shared with the game's thread (under mutex_) ----
    std::mutex mutex_;
    NetLocal local_;
    std::vector<uint8_t> save_;
    std::string save_clock_;
    uint32_t save_version_ = 0;
    std::map<uint32_t, Far> far_;
    uint32_t far_id_ = 0;             // the far PC (the first one heard from)
    std::deque<NetPacket> events_;
    NetPacket move_;
    bool move_fresh_ = false;
    std::vector<uint8_t> far_save_;
    std::string far_save_clock_;
    bool far_save_new_ = false;
    NetCounts counts_;
    NetStandin standin_;
    Clock::time_point standin_heard_{};
    NetCommand command_ = NetCommand::None;
    // (stand-in)
    NetCommand parent_command_ = NetCommand::None;
    Clock::time_point parent_heard_{};

    void start_player() {
        port_ = lan_net_port();
        if (!port_) {
            log("net: no PC-to-PC link: co-op over the internet is off\n");
            return;
        }
        lo_ = loopback_socket(0, &lo_port_);
        if (lo_ == kNoNetSocket) log("net: no socket on 127.0.0.1 for the stand-in\n");
        else log("net: over the internet; the stand-in reports to 127.0.0.1:%u\n", unsigned(lo_port_));
        transport_ = std::make_unique<StreamTransport>(port_);
        stream_ = std::make_unique<CoopChannel>(transport_.get(), 250);
        thread_ = std::thread([this] { player_loop(); });
    }

    void start_standin() {
        lo_ = loopback_socket(0, nullptr);
        if (lo_ == kNoNetSocket) {
            log("net: stand-in: no socket on 127.0.0.1\n");
            return;
        }
        parent_.sin_family = AF_INET;
        parent_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        parent_.sin_port = htons(g_net.parent_port);
        parent_heard_ = Clock::now();
        log("net: stand-in for the far player, reporting to 127.0.0.1:%u\n", unsigned(g_net.parent_port));
        thread_ = std::thread([this] { standin_loop(); });
    }

    void send_lo(const uint8_t* d, size_t n, const sockaddr_in& to) {
        if (lo_ == kNoNetSocket) return;
        ::sendto(lo_, reinterpret_cast<const char*>(d), int(n), 0, reinterpret_cast<const sockaddr*>(&to), sizeof to);
    }
    void tap(const NetPacket& p) {
        uint8_t b[8 + sizeof p.data];
        b[0] = 'T';
        put32(b + 1, ++tap_seq_);
        b[5] = p.aid, b[6] = p.len;
        std::memcpy(b + 7, p.data, p.len);
        send_lo(b, 7 + p.len, parent_);
    }
    void report(uint8_t state, uint32_t frame) {
        uint8_t b[6] = {'r', state};
        put32(b + 2, frame);
        send_lo(b, sizeof b, parent_);
    }

    NetPort* port_ = nullptr;
    net_socket_t lo_ = kNoNetSocket;
    uint16_t lo_port_ = 0;
    sockaddr_in standin_addr_{};
    bool standin_known_ = false;
    sockaddr_in parent_{};
    uint32_t tap_seq_ = 0, move_out_seq_ = 0;
    std::unique_ptr<StreamTransport> transport_;
    std::unique_ptr<CoopChannel> stream_;
    std::thread thread_;

    void send_far(uint32_t to, const uint8_t* d, size_t n) { port_->net_send(to, d, n); }

    // ---- the player's copy's thread ----
    void player_loop() {
        std::vector<uint8_t> d;
        auto last_state = Clock::now(), last_ping = Clock::now(), last_cmd = Clock::now(), last_log = Clock::now();
        for (;;) {
            const auto now = Clock::now();
            // The far PCs.
            uint32_t from = 0;
            while (port_->net_recv(from, d)) far_datagram(from, d, now);
            std::vector<uint32_t> peers;
            port_->net_peers(peers);
            {
                std::lock_guard lock(mutex_);
                transport_->peers.clear();
                for (uint32_t p : peers)
                    if (far_.count(p) && now - far_[p].heard < kLinkedFor) transport_->peers.push_back(p);
                if (far_id_ && std::find(peers.begin(), peers.end(), far_id_) == peers.end()) {
                    log("net: the far PC %08x is gone\n", far_id_);
                    far_.erase(far_id_);
                    far_id_ = 0;
                }
            }
            stream_->poll([this](uint32_t from, uint8_t type, const uint8_t* m, size_t n) {
                if (type != kEventMessage || n < 6 || size_t(m[5]) + 6 > n || m[5] > sizeof(NetPacket::data)) return;
                NetPacket p;
                p.aid = m[4], p.len = m[5];
                std::memcpy(p.data, m + 6, p.len);
                std::lock_guard lock(mutex_);
                if (from != far_id_) return;
                events_.push_back(p);
                counts_.events_in++;
                if (events_.size() > 4096) events_.pop_front();
            });
            // The stand-in on this PC.
            if (lo_ != kNoNetSocket) {
                uint8_t b[512];
                for (int i = 0; i < 256; i++) {
                    sockaddr_in a{};
#ifdef _WIN32
                    int alen = sizeof a;
#else
                    socklen_t alen = sizeof a;
#endif
                    const int n = int(::recvfrom(lo_, reinterpret_cast<char*>(b), int(sizeof b), 0,
                                                 reinterpret_cast<sockaddr*>(&a), &alen));
                    if (n <= 0) break;
                    standin_datagram(b, size_t(n), a, Clock::now());
                }
            }
            // Ours: state, pings, the save, the stand-in's command.
            if (now - last_state >= milliseconds(100)) {
                last_state = now;
                uint8_t b[4] = {'S'};
                {
                    std::lock_guard lock(mutex_);
                    b[1] = local_.role, b[2] = local_.in_session, b[3] = local_.standin;
                }
                send_far(0, b, sizeof b);
            }
            if (now - last_ping >= milliseconds(500)) {
                last_ping = now;
                uint8_t b[9] = {'P'};
                const uint64_t t = now_us();
                std::memcpy(b + 1, &t, 8);
                send_far(0, b, sizeof b);
            }
            send_save(now);
            if (standin_known_ && now - last_cmd >= milliseconds(100)) {
                last_cmd = now;
                uint8_t b[2] = {'c'};
                {
                    std::lock_guard lock(mutex_);
                    b[1] = uint8_t(command_);
                }
                send_lo(b, sizeof b, standin_addr_);
            }
            if (now - last_log >= milliseconds(10000)) {
                last_log = now;
                std::lock_guard lock(mutex_);
                if (far_id_) {
                    const Far& f = far_[far_id_];
                    log("net: far PC %08x: %d ms; taps %llu, out %llu events %llu moves, in %llu events %llu moves "
                        "(%llu late)\n",
                        far_id_, f.rtt_ms, (unsigned long long)counts_.taps, (unsigned long long)counts_.events_out,
                        (unsigned long long)counts_.moves_out, (unsigned long long)counts_.events_in,
                        (unsigned long long)counts_.moves_in, (unsigned long long)counts_.moves_stale);
                }
            }
            std::this_thread::sleep_for(milliseconds(2));
        }
    }

    void far_datagram(uint32_t from, const std::vector<uint8_t>& d, Clock::time_point now) {
        if (d.empty()) return;
        std::unique_lock lock(mutex_);
        if (far_id_ == 0) {
            far_id_ = from;
            log("net: the far PC %08x is here\n", from);
        }
        if (from != far_id_) return;  // (one far PC: two players)
        Far& f = far_[from];
        f.heard = now;
        switch (d[0]) {
        case 'R':
            transport_->in.emplace_back(from, std::vector<uint8_t>(d.begin() + 1, d.end()));
            if (transport_->in.size() > 8192) transport_->in.pop_front();
            return;
        case 'M': {
            if (d.size() < 7 || size_t(d[6]) + 7 > d.size() || d[6] > sizeof(NetPacket::data)) return;
            const uint32_t seq = get32(&d[1]);
            if (seq <= f.move_seq && f.move_seq - seq < 1000000) {  // (an older one came late)
                counts_.moves_stale++;
                return;
            }
            f.move_seq = seq;
            move_.aid = d[5], move_.len = d[6];
            std::memcpy(move_.data, &d[7], move_.len);
            move_fresh_ = true;
            counts_.moves_in++;
            return;
        }
        case 'S':
            if (d.size() < 4) return;
            f.state.role = d[1], f.state.in_session = d[2] != 0, f.state.standin = d[3];
            return;
        case 'P': {
            if (d.size() < 9) return;
            uint8_t b[9];
            std::memcpy(b, d.data(), 9);
            b[0] = 'Q';
            lock.unlock();
            send_far(from, b, sizeof b);
            return;
        }
        case 'Q': {
            if (d.size() < 9) return;
            uint64_t t;
            std::memcpy(&t, &d[1], 8);
            const int rtt = int((now_us() - t) / 1000);
            f.rtt_ms = f.rtt_ms < 0 ? rtt : (f.rtt_ms * 3 + rtt) / 4;
            return;
        }
        case 'F': {
            if (d.size() < 14) return;
            const uint32_t version = get32(&d[1]), size = get32(&d[5]), off = get32(&d[9]);
            const size_t clen = d[13];
            if (d.size() < 14 + clen || size == 0 || size > (64u << 20) || off % kPiece || off >= size) return;
            SaveIn& in = f.save_in;
            if (in.version != version || in.size != size) {
                in = SaveIn{};
                in.version = version, in.size = size;
                in.bytes.assign(size, 0);
                in.have.assign((size + kPiece - 1) / kPiece, 0);
                in.clock.assign(reinterpret_cast<const char*>(&d[14]), clen);
                log("net: the far player's save is coming (%u bytes)\n", size);
            }
            const size_t n = std::min(d.size() - 14 - clen, size_t(size - off));
            const size_t piece = off / kPiece;
            if (!in.have[piece]) {
                std::memcpy(&in.bytes[off], &d[14 + clen], n);
                in.have[piece] = 1;
                in.pieces_had++;
            }
            if (!in.done && in.pieces_had == in.have.size()) {
                in.done = true;
                far_save_ = in.bytes;
                far_save_clock_ = in.clock;
                far_save_new_ = true;
                log("net: the far player's save arrived (%u bytes, clock %s)\n", size, in.clock.c_str());
            }
            // (acknowledged at most every 20 ms, and at once when complete)
            if (now - in.last_ack < milliseconds(20) && !in.done) return;
            in.last_ack = now;
            std::vector<uint8_t> b(9 + (in.have.size() + 7) / 8, 0);
            b[0] = 'f';
            put32(&b[1], version), put32(&b[5], size);
            for (size_t i = 0; i < in.have.size(); i++)
                if (in.have[i]) b[9 + i / 8] |= uint8_t(1u << (i % 8));
            lock.unlock();
            if (b.size() <= kNetDatagram) send_far(from, b.data(), b.size());
            return;
        }
        case 'f': {
            if (d.size() < 9) return;
            SaveOut& o = f.save_out;
            if (get32(&d[1]) != o.version || get32(&d[5]) != o.size) return;
            size_t had = 0;
            for (size_t i = 0; i < o.acked.size(); i++) {
                if (9 + i / 8 < d.size() && (d[9 + i / 8] >> (i % 8) & 1)) o.acked[i] = 1;
                had += o.acked[i];
            }
            const int pct = o.acked.empty() ? 100 : int(had * 100 / o.acked.size());
            if (pct == 100 && o.percent != 100) log("net: our save arrived at the far PC\n");
            o.percent = pct;
            return;
        }
        default: return;
        }
    }

    // Our save, piece by piece, to the far PC (again to one that is new).
    void send_save(Clock::time_point now) {
        std::vector<std::vector<uint8_t>> out;
        uint32_t to = 0;
        {
            std::lock_guard lock(mutex_);
            if (!far_id_ || save_.empty()) return;
            Far& f = far_[far_id_];
            SaveOut& o = f.save_out;
            if (o.version != save_version_) {
                o = SaveOut{};
                o.version = save_version_, o.size = uint32_t(save_.size());
                o.acked.assign((save_.size() + kPiece - 1) / kPiece, 0);
                o.sent.assign(o.acked.size(), Clock::time_point{});
                o.percent = 0;
                log("net: sending our save to the far PC (%zu bytes)\n", save_.size());
            }
            if (o.percent == 100) return;
            const auto resend = milliseconds(std::max(300, f.rtt_ms * 2));
            size_t in_flight = 0;
            for (size_t i = 0; i < o.acked.size(); i++)
                if (!o.acked[i] && o.sent[i] != Clock::time_point{} && now - o.sent[i] < resend) in_flight++;
            for (size_t i = 0; i < o.acked.size() && in_flight < kPiecesInFlight; i++) {
                if (o.acked[i] || (o.sent[i] != Clock::time_point{} && now - o.sent[i] < resend)) continue;
                const size_t off = i * kPiece, n = std::min(kPiece, save_.size() - off);
                std::vector<uint8_t> b(14 + save_clock_.size() + n);
                b[0] = 'F';
                put32(&b[1], o.version), put32(&b[5], o.size), put32(&b[9], uint32_t(off));
                b[13] = uint8_t(save_clock_.size());
                std::memcpy(&b[14], save_clock_.data(), save_clock_.size());
                std::memcpy(&b[14 + save_clock_.size()], &save_[off], n);
                out.push_back(std::move(b));
                o.sent[i] = now;
                in_flight++;
            }
            to = far_id_;
        }
        for (const auto& b : out) send_far(to, b.data(), b.size());
    }

    void standin_datagram(const uint8_t* b, size_t n, const sockaddr_in& from, Clock::time_point now) {
        if (n < 1) return;
        std::unique_lock lock(mutex_);
        standin_addr_ = from;
        standin_known_ = true;
        standin_heard_ = now;
        if (b[0] == 'r' && n >= 6) {
            standin_.state = b[1];
            standin_.frame = get32(b + 2);
            return;
        }
        if (b[0] != 'T' || n < 7 || size_t(b[6]) + 7 > n) return;
        counts_.taps++;
        NetPacket p;
        p.aid = b[5], p.len = b[6];
        if (p.len > sizeof p.data) return;
        std::memcpy(p.data, b + 7, p.len);
        const uint32_t to = far_id_;
        if (!to) return;
        if (is_move(p)) {
            counts_.moves_out++;
            uint8_t m[7 + sizeof p.data];
            m[0] = 'M';
            put32(m + 1, ++move_out_seq_);
            m[5] = p.aid, m[6] = p.len;
            std::memcpy(m + 7, p.data, p.len);
            lock.unlock();
            send_far(to, m, 7 + p.len);
            return;
        }
        counts_.events_out++;
        lock.unlock();
        uint8_t m[6 + sizeof p.data];
        put32(m, get32(b + 1));
        m[4] = p.aid, m[5] = p.len;
        std::memcpy(m + 6, p.data, p.len);
        if (!stream_->send(to, kEventMessage, m, 6 + p.len))
            log("net: a packet (type %02x) could not go to the far PC (not linked)\n", p.data[0]);
    }

    // ---- the stand-in's thread: its player's commands ----
    void standin_loop() {
        uint8_t b[64];
        for (;;) {
            const int n = int(::recv(lo_, reinterpret_cast<char*>(b), int(sizeof b), 0));
            if (n >= 2 && b[0] == 'c') {
                std::lock_guard lock(mutex_);
                parent_command_ = NetCommand(b[1]);
                parent_heard_ = Clock::now();
            }
        }
    }
};

Net& net() {
    static Net* n = new Net;  // (never destroyed: its thread runs to the end of the process)
    return *n;
}

std::once_flag g_started;
void start() {
    std::call_once(g_started, [] {
        if (g_net.standin) net().start_standin();
        else if (g_net.internet) net().start_player();
    });
}

}  // namespace

void configure_net(const NetConfig& config) { g_net = config; }
const NetConfig& net_config() { return g_net; }
bool net_internet_mode() { return g_net.internet && !g_net.standin; }

void net_set_local(const NetLocal& local) {
    start();
    std::lock_guard lock(net().mutex_);
    net().local_ = local;
}

NetFar net_far() {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    NetFar f;
    if (!n.far_id_) return f;
    const Far& other = n.far_[n.far_id_];
    f.linked = Clock::now() - other.heard < kLinkedFor;
    f.rtt_ms = other.rtt_ms;
    f.role = other.state.role;
    f.in_session = other.state.in_session;
    f.standin = other.state.standin;
    f.save_percent = other.save_out.percent;
    f.far_save_percent = other.save_in.have.empty() ? -1 : int(other.save_in.pieces_had * 100 / other.save_in.have.size());
    return f;
}

void net_offer_save(std::vector<uint8_t> save, const std::string& clock) {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    n.save_version_ = fnv(save) ^ uint32_t(now_us());
    n.save_ = std::move(save);
    n.save_clock_ = clock.substr(0, 40);
}

bool net_take_far_save(std::vector<uint8_t>& save, std::string& clock) {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    if (!n.far_save_new_) return false;
    n.far_save_new_ = false;
    save = n.far_save_;
    clock = n.far_save_clock_;
    return true;
}

bool net_peek_event(NetPacket& out) {
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    if (n.events_.empty()) return false;
    out = n.events_.front();
    return true;
}
void net_pop_event() {
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    if (!n.events_.empty()) n.events_.pop_front();
}
bool net_take_move(NetPacket& out) {
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    if (!n.move_fresh_) return false;
    n.move_fresh_ = false;
    out = n.move_;
    return true;
}
void net_drop_far_packets() {
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    n.events_.clear();
    n.move_fresh_ = false;
}
NetCounts net_counts() {
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    return n.counts_;
}

uint16_t net_standin_port() {
    start();
    return net().lo_port_;
}
NetStandin net_standin() {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    NetStandin s = n.standin_;
    s.alive = n.standin_heard_ != Clock::time_point{} && Clock::now() - n.standin_heard_ < kStandinGone;
    if (!s.alive) s.state = NetStandin::None;
    return s;
}
void net_standin_command(NetCommand command) {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    n.command_ = command;
}

void net_tap(const NetPacket& packet) {
    start();
    net().tap(packet);
}
NetCommand net_parent_command() {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    return n.parent_command_;
}
void net_report(uint8_t state, uint32_t frame) {
    start();
    net().report(state, frame);
}
bool net_parent_gone() {
    start();
    Net& n = net();
    std::lock_guard lock(n.mutex_);
    return Clock::now() - n.parent_heard_ > kParentGone;
}

}  // namespace ntr
