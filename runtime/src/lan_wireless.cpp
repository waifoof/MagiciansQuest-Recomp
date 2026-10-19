// Local wireless between PCs (include/ntr/lan.hpp): melonDS's MP frames as UDP datagrams, with the same semantics as
// the shared-memory link between instances on one PC (local_wireless.cpp, after melonDS's LocalMP): every frame,
// command and ack reaches every other console in the session; a reply reaches the console that sent the last command.
//
// The PCs form a star: each joining PC sends everything to the host, which hands it on to the others (and keeps
// what is for itself). Each PC picks a random 32-bit id at start, which names it in every datagram and gives its
// console a MAC address of its own. A PC that is not heard from for five seconds has left.
#include "wireless_link.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
static constexpr socket_t kNoSocket = INVALID_SOCKET;
static void close_socket(socket_t s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using socket_t = int;
static constexpr socket_t kNoSocket = -1;
static void close_socket(socket_t s) { ::close(s); }
#endif

#include "local_wireless.hpp"
#include "ntr/lan.hpp"
#include "runtime_internal.hpp"

namespace ntr {

namespace {

constexpr uint32_t kMagic = 0x4E4C514D;  // "MQLN"
constexpr int kMaxFrameSize = 0x948;
constexpr int kRecvTimeoutMs = 25;      // as melonDS: how long to wait for the host's frames, or for replies
constexpr int kKeepaliveMs = 500;
constexpr int kGoneMs = 5000;
constexpr uint32_t kHostMark = 0xFFFFFFFF;  // a hello's target: it comes from the host

enum Kind : uint8_t { Hello = 0, Frame = 1, Begin = 2, End = 3, Bye = 4 };

#pragma pack(push, 1)
struct Header {
    uint32_t magic;
    uint32_t sender;     // the PC it comes from
    uint32_t target;     // a reply: the PC it is for (the last command's sender); otherwise 0
    uint8_t kind;
    uint8_t type;        // Frame: 0 frame, 1 command, 2 reply, 3 ack
    uint16_t aid;        // a reply's association id
    uint32_t length;
    uint64_t timestamp;
};
#pragma pack(pop)

struct Message {
    uint32_t sender;
    uint8_t type;
    uint16_t aid;
    uint64_t timestamp;
    std::vector<uint8_t> data;
};

LanConfig g_config;

using Clock = std::chrono::steady_clock;

class LanWireless final : public WirelessLink {
public:
    static LanWireless* create(const LanConfig& config) {
        auto* l = new LanWireless(config);
        if (!l->open()) {
            g_error = l->error_;
            delete l;
            return nullptr;
        }
        return l;
    }

    ~LanWireless() override {
        send_all(Bye, 0, 0, 0, nullptr, 0, 0);
        stop_ = true;
        if (thread_.joinable()) thread_.join();
        if (sock_ != kNoSocket) close_socket(sock_);
    }

    bool mac_offset(uint8_t out[3]) const override {
        out[0] = uint8_t(id_ >> 8);
        out[1] = uint8_t(id_ >> 16);
        out[2] = uint8_t(id_ >> 24);
        return true;
    }

    void begin() override {
        {
            std::lock_guard lock(mutex_);
            frames_.clear();
            replies_.clear();
            in_session_ = true;
        }
        send_all(Begin, 0, 0, 0, nullptr, 0, 0);
        log("lan: joined the game's multiplayer (%d other PCs in it)\n", status().in_session);
    }

    void end() override {
        {
            std::lock_guard lock(mutex_);
            in_session_ = false;
        }
        send_all(End, 0, 0, 0, nullptr, 0, 0);
        log("lan: left the game's multiplayer\n");
    }

    int send_packet(uint8_t* data, int len, uint64_t ts) override { return send_frame(0, 0, data, len, ts); }
    int send_cmd(uint8_t* data, int len, uint64_t ts) override {
        {
            std::lock_guard lock(mutex_);
            replies_.clear();  // (replies to this command start now)
            mp_host_ = id_;
        }
        return send_frame(1, 0, data, len, ts);
    }
    int send_reply(uint8_t* data, int len, uint64_t ts, uint16_t aid) override { return send_frame(2, aid, data, len, ts); }
    int send_ack(uint8_t* data, int len, uint64_t ts) override { return send_frame(3, 0, data, len, ts); }

    int recv_packet(uint8_t* data, uint64_t* ts) override { return recv(data, false, ts); }

    int recv_host_packet(uint8_t* data, uint64_t* ts) override {
        {
            std::lock_guard lock(mutex_);
            if (mp_host_ != 0 && mp_host_ != id_ && !session_.count(mp_host_)) return -1;  // the host left
        }
        return recv(data, true, ts);
    }

    uint16_t recv_replies(uint8_t* data, uint64_t timestamp, uint16_t aidmask) override {
        uint16_t ret = 0;
        std::vector<uint32_t> replied;
        std::unique_lock lock(mutex_);
        if (session_.empty()) return 0;  // (nobody else is there)
        const auto deadline = Clock::now() + std::chrono::milliseconds(kRecvTimeoutMs);
        for (;;) {
            while (!replies_.empty()) {
                Message m = std::move(replies_.front());
                replies_.pop_front();
                if (m.timestamp < timestamp - 32) continue;  // stale
                if (!m.data.empty() && m.aid >= 1 && m.aid <= 15) {
                    std::memcpy(&data[(m.aid - 1) * 1024], m.data.data(), std::min<size_t>(m.data.size(), 1024));
                    ret |= uint16_t(1u << m.aid);
                }
                if (std::find(replied.begin(), replied.end(), m.sender) == replied.end()) replied.push_back(m.sender);
            }
            bool all = true;
            for (const auto& [id, peer] : session_) all = all && std::find(replied.begin(), replied.end(), id) != replied.end();
            if (all || (ret & aidmask) == aidmask) return ret;
            if (cv_replies_.wait_until(lock, deadline) == std::cv_status::timeout && replies_.empty()) return ret;
        }
    }

    LanStatus status() {
        std::lock_guard lock(mutex_);
        LanStatus s;
        s.active = true;
        s.hosting = config_.mode == LanConfig::Mode::Host;
        s.peers = int(peers_.size());
        s.in_session = int(session_.size());
        return s;
    }

    static std::string g_error;

private:
    struct Peer {
        sockaddr_in addr{};
        Clock::time_point seen;
    };

    explicit LanWireless(const LanConfig& config) : config_(config) {
        std::random_device rd;
        do id_ = (uint32_t(rd()) << 16) ^ uint32_t(rd()); while (id_ == 0);
    }

    bool open() {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return fail("Windows sockets did not start");
#endif
        sock_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_ == kNoSocket) return fail("no UDP socket");
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(config_.mode == LanConfig::Mode::Host ? config_.port : 0);
        if (::bind(sock_, reinterpret_cast<sockaddr*>(&local), sizeof local) != 0)
            return fail("UDP port " + std::to_string(config_.port) + " is taken (another program, or a host already running here)");
#ifdef _WIN32
        DWORD timeout = 100;
        ::setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof timeout);
#else
        timeval timeout{0, 100000};
        ::setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif
        if (config_.mode == LanConfig::Mode::Join && (config_.host.empty() || config_.host == "auto")) {
            // (no address: the host is looked for on this network, by broadcast, until one answers)
            int on = 1;
            ::setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&on), sizeof on);
            host_addr_ = {};
            host_addr_.sin_family = AF_INET;
            host_addr_.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            host_addr_.sin_port = htons(config_.port);
            searching_ = true;
            log("lan: looking for a host on this network (UDP port %u; this PC's id %08x)\n", unsigned(config_.port), id_);
        } else if (config_.mode == LanConfig::Mode::Join) {
            addrinfo hints{}, *res = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            if (::getaddrinfo(config_.host.c_str(), nullptr, &hints, &res) != 0 || !res)
                return fail("cannot find the host \"" + config_.host + "\"");
            host_addr_ = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
            host_addr_.sin_port = htons(config_.port);
            ::freeaddrinfo(res);
            char text[INET_ADDRSTRLEN] = {};
            ::inet_ntop(AF_INET, &host_addr_.sin_addr, text, sizeof text);
            log("lan: joining %s:%u (this PC's id %08x)\n", text, unsigned(config_.port), id_);
        } else {
            log("lan: hosting on UDP port %u (this PC's id %08x)\n", unsigned(config_.port), id_);
        }
        thread_ = std::thread([this] { receive_loop(); });
        return true;
    }

    bool fail(const std::string& why) {
        error_ = why;
        log("lan: %s\n", why.c_str());
        if (sock_ != kNoSocket) close_socket(sock_);
        sock_ = kNoSocket;
        return false;
    }

    int send_frame(uint8_t type, uint16_t aid, uint8_t* data, int len, uint64_t ts) {
        if (len > kMaxFrameSize) {
            log("lan: frame too big (%d bytes)\n", len);
            return 0;
        }
        uint32_t target = 0;
        if (type == 2) {
            std::lock_guard lock(mutex_);
            target = mp_host_;
        }
        send_all(Frame, type, aid, target, data, len, ts);
        return len;
    }

    void put(std::vector<uint8_t>& buf, uint8_t kind, uint8_t type, uint16_t aid, uint32_t sender, uint32_t target,
             const uint8_t* data, int len, uint64_t ts) {
        const Header h{kMagic, sender, target, kind, type, aid, uint32_t(len), ts};
        buf.resize(sizeof h + size_t(len));
        std::memcpy(buf.data(), &h, sizeof h);
        if (len) std::memcpy(buf.data() + sizeof h, data, size_t(len));
    }

    void send_to(const sockaddr_in& to, const std::vector<uint8_t>& buf) {
        ::sendto(sock_, reinterpret_cast<const char*>(buf.data()), int(buf.size()), 0,
                 reinterpret_cast<const sockaddr*>(&to), sizeof to);
    }

    // Our own datagram: to the host (joining), or to every PC that joined (hosting; a reply only to its target).
    void send_all(uint8_t kind, uint8_t type, uint16_t aid, uint32_t target, const uint8_t* data, int len, uint64_t ts) {
        if (sock_ == kNoSocket) return;
        std::vector<uint8_t> buf;
        put(buf, kind, type, aid, id_, target, data, len, ts);
        if (config_.mode == LanConfig::Mode::Join) {
            sockaddr_in to;
            {
                std::lock_guard lock(mutex_);
                if (searching_ && kind != Hello) return;  // (no host yet)
                to = host_addr_;
            }
            send_to(to, buf);
            return;
        }
        std::vector<sockaddr_in> to;
        {
            std::lock_guard lock(mutex_);
            for (const auto& [id, peer] : peers_)
                if (kind != Frame || type != 2 || id == target) to.push_back(peer.addr);
        }
        for (const sockaddr_in& a : to) send_to(a, buf);
    }

    void receive_loop() {
        std::vector<uint8_t> buf(65536);
        auto last_hello = Clock::now() - std::chrono::seconds(1);
        while (!stop_) {
            const auto now = Clock::now();
            if (now - last_hello >= std::chrono::milliseconds(kKeepaliveMs)) {
                last_hello = now;
                send_all(Hello, 0, 0, config_.mode == LanConfig::Mode::Host ? kHostMark : 0, nullptr, 0, 0);
                prune(now);
            }
            sockaddr_in from{};
#ifdef _WIN32
            int from_len = sizeof from;
#else
            socklen_t from_len = sizeof from;
#endif
            const int n = int(::recvfrom(sock_, reinterpret_cast<char*>(buf.data()), int(buf.size()), 0,
                                         reinterpret_cast<sockaddr*>(&from), &from_len));
            if (n < int(sizeof(Header))) continue;
            Header h;
            std::memcpy(&h, buf.data(), sizeof h);
            if (h.magic != kMagic || h.sender == id_ || sizeof h + h.length > size_t(n)) continue;
            handle(h, buf.data() + sizeof h, from, buf.data(), n);
        }
    }

    void handle(const Header& h, const uint8_t* payload, const sockaddr_in& from, const uint8_t* raw, int raw_len) {
        const bool hosting = config_.mode == LanConfig::Mode::Host;
        std::vector<sockaddr_in> forward;
        {
            std::lock_guard lock(mutex_);
            if (searching_) {
                if (h.kind != Hello || h.target != kHostMark) return;  // (only a host's hello, while looking for one)
                host_addr_ = from;
                searching_ = false;
                char text[INET_ADDRSTRLEN] = {};
                ::inet_ntop(AF_INET, &from.sin_addr, text, sizeof text);
                log("lan: found the host at %s\n", text);
            }
            Peer& p = peers_[h.sender];
            if (p.seen == Clock::time_point{}) log("lan: PC %08x is here\n", h.sender);
            p.addr = hosting ? from : host_addr_;  // (joining: everything goes through the host)
            p.seen = Clock::now();
            if (h.kind == Begin) session_[h.sender] = true;
            if (h.kind == End || h.kind == Bye) session_.erase(h.sender);
            if (h.kind == Bye) {
                peers_.erase(h.sender);
                log("lan: PC %08x left\n", h.sender);
            }
            // Hosting: hand it on to the other PCs (a reply only to the PC it is for).
            if (hosting && h.kind != Hello)
                for (const auto& [id, peer] : peers_)
                    if (id != h.sender && (h.kind != Frame || h.type != 2 || id == h.target)) forward.push_back(peer.addr);
            if (h.kind == Frame && in_session_) {
                Message m{h.sender, h.type, h.aid, h.timestamp, std::vector<uint8_t>(payload, payload + h.length)};
                if (h.type == 2) {
                    if (h.target == id_) {
                        replies_.push_back(std::move(m));
                        cv_replies_.notify_all();
                    }
                } else {
                    if (h.type == 1) mp_host_ = h.sender;
                    frames_.push_back(std::move(m));
                    if (frames_.size() > 512) frames_.pop_front();
                    cv_frames_.notify_all();
                }
            }
        }
        const std::vector<uint8_t> copy(raw, raw + raw_len);
        for (const sockaddr_in& a : forward) send_to(a, copy);
    }

    void prune(Clock::time_point now) {
        std::lock_guard lock(mutex_);
        for (auto it = peers_.begin(); it != peers_.end();) {
            if (now - it->second.seen > std::chrono::milliseconds(kGoneMs)) {
                log("lan: PC %08x stopped answering\n", it->first);
                session_.erase(it->first);
                it = peers_.erase(it);
            } else {
                ++it;
            }
        }
    }

    int recv(uint8_t* data, bool block, uint64_t* ts) {
        std::unique_lock lock(mutex_);
        if (frames_.empty()) {
            if (!block) return 0;
            cv_frames_.wait_for(lock, std::chrono::milliseconds(kRecvTimeoutMs), [this] { return !frames_.empty(); });
            if (frames_.empty()) return 0;
        }
        Message m = std::move(frames_.front());
        frames_.pop_front();
        std::memcpy(data, m.data.data(), m.data.size());
        if (ts) *ts = m.timestamp;
        return int(m.data.size());
    }

    LanConfig config_;
    uint32_t id_ = 0;
    socket_t sock_ = kNoSocket;
    sockaddr_in host_addr_{};
    std::string error_;
    std::thread thread_;
    std::atomic<bool> stop_{false};

    std::mutex mutex_;
    std::condition_variable cv_frames_, cv_replies_;
    std::map<uint32_t, Peer> peers_;
    std::map<uint32_t, bool> session_;  // other PCs in the game's multiplayer
    std::deque<Message> frames_, replies_;
    bool in_session_ = false;
    bool searching_ = false;  // joining with no address: still looking for the host (host_addr_ is the broadcast)
    uint32_t mp_host_ = 0;  // the PC that sent the last command (whose console hosts the game's session)
};

std::string LanWireless::g_error;

LanWireless* lan_link() {
    static LanWireless* link = g_config.mode == LanConfig::Mode::Off ? nullptr : LanWireless::create(g_config);
    return link;
}

// The shared-memory link between instances on this PC, as a WirelessLink.
class LocalLink final : public WirelessLink {
public:
    explicit LocalLink(LocalWireless* w) : w_(w) {}
    bool mac_offset(uint8_t out[3]) const override {
        const int n = w_->slot();
        if (n <= 0) return false;
        out[0] = uint8_t(n);
        out[1] = uint8_t(n * 0x44);
        out[2] = uint8_t(n * 0x10);
        return true;
    }
    void begin() override { w_->begin(); }
    void end() override { w_->end(); }
    int send_packet(uint8_t* d, int l, uint64_t t) override { return w_->send_packet(d, l, t); }
    int recv_packet(uint8_t* d, uint64_t* t) override { return w_->recv_packet(d, t); }
    int send_cmd(uint8_t* d, int l, uint64_t t) override { return w_->send_cmd(d, l, t); }
    int send_reply(uint8_t* d, int l, uint64_t t, uint16_t aid) override { return w_->send_reply(d, l, t, aid); }
    int send_ack(uint8_t* d, int l, uint64_t t) override { return w_->send_ack(d, l, t); }
    int recv_host_packet(uint8_t* d, uint64_t* t) override { return w_->recv_host_packet(d, t); }
    uint16_t recv_replies(uint8_t* d, uint64_t t, uint16_t m) override { return w_->recv_replies(d, t, m); }

private:
    LocalWireless* w_;
};

}  // namespace

void configure_lan(const LanConfig& config) { g_config = config; }

std::unique_ptr<WirelessLink> make_lan_link(const LanConfig& config) { return std::unique_ptr<WirelessLink>(LanWireless::create(config)); }

LanStatus lan_status() {
    if (LanWireless* l = g_config.mode == LanConfig::Mode::Off ? nullptr : lan_link()) return l->status();
    LanStatus s;
    s.error = LanWireless::g_error;
    return s;
}

WirelessLink* wireless_link() {
    static WirelessLink* link = []() -> WirelessLink* {
        if (g_config.mode != LanConfig::Mode::Off) return lan_link();
        if (LocalWireless* w = LocalWireless::get()) return new LocalLink(w);
        return nullptr;
    }();
    return link;
}

}  // namespace ntr
