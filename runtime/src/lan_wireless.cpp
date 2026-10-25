// Local wireless between PCs (include/ntr/lan.hpp): melonDS's MP frames as UDP datagrams, with the same semantics as
// the shared-memory link between instances on one PC (local_wireless.cpp, after melonDS's LocalMP): every frame,
// command and ack reaches every other console in the session; a reply reaches the console that sent the last command.
//
// The PCs form a star: each joining PC sends everything to the host, which hands it on to the others (and keeps
// what is for itself). Each PC picks a random 32-bit id at start, which names it in every datagram and gives its
// console a MAC address of its own. A PC that is not heard from for five seconds has left.
//
// The co-op side channel (coop_channel.cpp) rides on the same socket as datagrams of a kind of their own (Coop), to
// one PC or to all, handed on by the host like the frames; they never mix with the game's frames.
//
// Who is in the game's multiplayer is told by Begin and End, and again in every keepalive (Hello), so a lost Begin
// or End is made good within half a second. Each carries the sender's count of its own Begins and Ends (an epoch, in
// the timestamp), so a late datagram (reordered on the way) never undoes a newer one.
//
// Testing (env): NTR_LAN_LOSS=<percent>[:coop] drops that share of the datagrams this PC sends (or hands on), of every
// kind or only the side channel's; NTR_LAN_DELAY=<ms>[:<jitter ms>] holds each one that long (plus a random part up to
// the jitter, which reorders them); ":coop" limits both to the side channel. Only for tests of a bad network; off
// unless set. NTR_LAN_WAIT_MS=<ms> waits that long for the host's frames and for replies instead of 25 ms (to try
// links with a longer round trip).
#include "wireless_link.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
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

#include "coop_channel.hpp"
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
constexpr uint8_t kHelloState = 0x80, kHelloInSession = 0x01;  // a hello's type: it tells the sender's state (epoch)

enum Kind : uint8_t { Hello = 0, Frame = 1, Begin = 2, End = 3, Bye = 4, Coop = 5 };
constexpr size_t kMaxCoopQueued = 4096;  // side-channel datagrams waiting for the game's thread

#pragma pack(push, 1)
struct Header {
    uint32_t magic;
    uint32_t sender;     // the PC it comes from
    uint32_t target;     // a reply: the PC it is for (the last command's sender); Coop: the PC it is for, 0 all
    uint8_t kind;
    uint8_t type;        // Frame: 0 frame, 1 command, 2 reply, 3 ack; Hello: kHelloState | kHelloInSession
    uint16_t aid;        // a reply's association id
    uint32_t length;
    uint64_t timestamp;  // Frame: the game's; Hello (with kHelloState), Begin, End: the sender's epoch
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

class LanWireless final : public WirelessLink, public CoopTransport {
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
        {
            std::lock_guard lock(impair_mutex_);
            impair_stop_ = true;
        }
        impair_cv_.notify_all();
        if (impair_thread_.joinable()) impair_thread_.join();
        if (sock_ != kNoSocket) close_socket(sock_);
    }

    bool mac_offset(uint8_t out[3]) const override {
        out[0] = uint8_t(id_ >> 8);
        out[1] = uint8_t(id_ >> 16);
        out[2] = uint8_t(id_ >> 24);
        return true;
    }

    void begin() override {
        uint64_t epoch;
        {
            std::lock_guard lock(mutex_);
            frames_.clear();
            replies_.clear();
            in_session_ = true;
            epoch = ++epoch_;
        }
        send_all(Begin, 0, 0, 0, nullptr, 0, epoch);
        log("lan: joined the game's multiplayer (%d other PCs in it)\n", status().in_session);
    }

    void end() override {
        uint64_t epoch;
        {
            std::lock_guard lock(mutex_);
            in_session_ = false;
            epoch = ++epoch_;
        }
        send_all(End, 0, 0, 0, nullptr, 0, epoch);
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
        const auto deadline = Clock::now() + std::chrono::milliseconds(wait_ms_);
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

    // The co-op side channel (CoopTransport): this PC's id names it; the PCs in the game's multiplayer are its peers.
    CoopTransport* coop() override { return this; }
    uint32_t coop_node() const override { return id_; }
    void coop_peers(std::vector<uint32_t>& out) override {
        out.clear();
        std::lock_guard lock(mutex_);
        if (!in_session_) return;
        for (const auto& [id, in] : session_) out.push_back(id);
    }
    void coop_send(uint32_t to, const uint8_t* data, size_t len) override {
        if (len <= kCoopDatagram) send_all(Coop, 0, 0, to, data, int(len), 0);
    }
    bool coop_recv(uint32_t& from, std::vector<uint8_t>& data) override {
        std::lock_guard lock(mutex_);
        if (coop_.empty()) return false;
        from = coop_.front().sender;
        data = std::move(coop_.front().data);
        coop_.pop_front();
        return true;
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
        uint64_t epoch = 0;  // the newest epoch heard from it (its Begin, End, Hello)
        uint64_t frame_ts = 0;  // the newest timestamp of its frames, commands and acks (a reply is not counted)
    };

    explicit LanWireless(const LanConfig& config) : config_(config) {
        std::random_device rd;
        do id_ = (uint32_t(rd()) << 16) ^ uint32_t(rd()); while (id_ == 0);
        rng_.seed(rd());
        if (const char* e = std::getenv("NTR_LAN_LOSS")) {
            loss_ = std::atof(e);
            loss_coop_only_ = std::strstr(e, ":coop") != nullptr;
        }
        if (const char* e = std::getenv("NTR_LAN_WAIT_MS")) wait_ms_ = std::max(1, std::atoi(e));
        if (const char* e = std::getenv("NTR_LAN_DELAY")) {
            delay_ms_ = std::atoi(e);
            if (const char* j = std::strchr(e, ':')) jitter_ms_ = std::atoi(j + 1);
        }
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
        if (loss_ > 0 || delay_ms_ > 0 || jitter_ms_ > 0) {
            log("lan: testing a bad network: %.1f%% of the datagrams sent%s dropped, the others held %d ms (+0..%d)\n",
                loss_, loss_coop_only_ ? " (side channel only)" : "", delay_ms_, jitter_ms_);
            if (delay_ms_ > 0 || jitter_ms_ > 0) impair_thread_ = std::thread([this] { impair_loop(); });
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
        if ((loss_ > 0 || delay_ms_ > 0 || jitter_ms_ > 0) && buf.size() >= sizeof(Header) &&
            (!loss_coop_only_ || buf[offsetof(Header, kind)] == Coop)) {
            std::lock_guard lock(impair_mutex_);
            impair_sent_++;
            if (loss_ > 0 && std::uniform_real_distribution<double>(0, 100)(rng_) < loss_) {
                impair_dropped_++;
                return;
            }
            if (delay_ms_ > 0 || jitter_ms_ > 0) {
                const int ms = delay_ms_ + (jitter_ms_ > 0 ? std::uniform_int_distribution<int>(0, jitter_ms_)(rng_) : 0);
                held_.emplace(Clock::now() + std::chrono::milliseconds(ms), std::make_pair(to, buf));
                impair_cv_.notify_all();
                return;
            }
        }
        send_now(to, buf);
    }

    void send_now(const sockaddr_in& to, const std::vector<uint8_t>& buf) {
        ::sendto(sock_, reinterpret_cast<const char*>(buf.data()), int(buf.size()), 0,
                 reinterpret_cast<const sockaddr*>(&to), sizeof to);
    }

    // (testing, NTR_LAN_DELAY: sends the held datagrams when their time comes)
    void impair_loop() {
        std::unique_lock lock(impair_mutex_);
        while (!impair_stop_) {
            if (held_.empty()) {
                impair_cv_.wait(lock);
                continue;
            }
            const auto due = held_.begin()->first;
            if (Clock::now() < due) {
                impair_cv_.wait_until(lock, due);
                continue;
            }
            auto item = std::move(held_.begin()->second);
            held_.erase(held_.begin());
            lock.unlock();
            send_now(item.first, item.second);
            lock.lock();
        }
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
                if (for_peer(kind, type, target, id)) to.push_back(peer.addr);
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
                uint8_t state;
                uint64_t epoch;
                {
                    std::lock_guard lock(mutex_);
                    state = uint8_t(kHelloState | (in_session_ ? kHelloInSession : 0));
                    epoch = epoch_;
                }
                send_all(Hello, state, 0, config_.mode == LanConfig::Mode::Host ? kHostMark : 0, nullptr, 0, epoch);
                prune(now);
                if ((loss_ > 0 || delay_ms_ > 0 || jitter_ms_ > 0) && ++hellos_ % 20 == 0) {
                    std::lock_guard lock(impair_mutex_);
                    log("lan: testing a bad network: %llu of %llu datagrams dropped\n",
                        (unsigned long long)impair_dropped_, (unsigned long long)impair_sent_);
                }
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
            // A frame, command or ack older than one already here from the same console came late (the network
            // reordered them): dropped, as a radio never delivers them out of order (the game's exchange broke on
            // reordered ones: "Unable to connect"). Not handed on either. A reply has its own staleness test.
            if (h.kind == Frame && h.type != 2) {
                if (h.timestamp < p.frame_ts) {
                    if (++late_ <= 5 || late_ % 100 == 0)
                        log("lan: a frame from PC %08x came late; dropped (%llu so far)\n", h.sender, (unsigned long long)late_);
                    return;
                }
                p.frame_ts = h.timestamp;
            }
            if (h.kind == Begin) p.frame_ts = 0;
            // In the game's multiplayer or not: from Begin and End, and from the state every hello tells (a lost Begin
            // or End made good); a datagram older than the newest state heard (its epoch) changes nothing.
            const bool state = h.kind == Begin || h.kind == End || (h.kind == Hello && (h.type & kHelloState));
            if (state && h.timestamp >= p.epoch) {
                p.epoch = h.timestamp;
                const bool in = h.kind == Begin || (h.kind == Hello && (h.type & kHelloInSession));
                if (in && !session_.count(h.sender)) {
                    session_[h.sender] = true;
                    if (h.kind == Hello) log("lan: PC %08x is in the game's multiplayer (told by its keepalive)\n", h.sender);
                } else if (!in && session_.count(h.sender)) {
                    session_.erase(h.sender);
                    if (h.kind == Hello) log("lan: PC %08x left the game's multiplayer (told by its keepalive)\n", h.sender);
                }
            }
            if (h.kind == Bye) session_.erase(h.sender);
            if (h.kind == Bye) {
                peers_.erase(h.sender);
                log("lan: PC %08x left\n", h.sender);
            }
            // Hosting: hand it on to the other PCs (a reply only to the PC it is for).
            if (hosting && h.kind != Hello)
                for (const auto& [id, peer] : peers_)
                    if (id != h.sender && for_peer(h.kind, h.type, h.target, id)) forward.push_back(peer.addr);
            if (h.kind == Coop && (h.target == 0 || h.target == id_)) {
                coop_.push_back(Message{h.sender, 0, 0, 0, std::vector<uint8_t>(payload, payload + h.length)});
                if (coop_.size() > kMaxCoopQueued) coop_.pop_front();
            }
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

    // Whether a datagram goes to (or is handed on to) the PC id: a reply and a side-channel datagram for one PC only
    // to that one.
    static bool for_peer(uint8_t kind, uint8_t type, uint32_t target, uint32_t id) {
        if (kind == Frame && type == 2) return id == target;
        if (kind == Coop) return target == 0 || id == target;
        return true;
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
            cv_frames_.wait_for(lock, std::chrono::milliseconds(wait_ms_), [this] { return !frames_.empty(); });
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
    std::deque<Message> frames_, replies_, coop_;
    bool in_session_ = false;
    bool searching_ = false;  // joining with no address: still looking for the host (host_addr_ is the broadcast)
    uint32_t mp_host_ = 0;  // the PC that sent the last command (whose console hosts the game's session)
    uint64_t epoch_ = 0;    // this PC's Begins and Ends so far
    uint64_t late_ = 0;     // frames dropped for coming late
    int wait_ms_ = kRecvTimeoutMs;  // (testing, NTR_LAN_WAIT_MS: another wait for the host's frames and for replies)

    // (testing a bad network: NTR_LAN_LOSS, NTR_LAN_DELAY)
    double loss_ = 0;
    bool loss_coop_only_ = false;
    int delay_ms_ = 0, jitter_ms_ = 0;
    std::mutex impair_mutex_;
    std::condition_variable impair_cv_;
    std::mt19937 rng_;
    std::multimap<Clock::time_point, std::pair<sockaddr_in, std::vector<uint8_t>>> held_;
    std::thread impair_thread_;
    bool impair_stop_ = false;
    uint64_t impair_sent_ = 0, impair_dropped_ = 0, hellos_ = 0;
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
    void begin() override {
        w_->begin();
        if (coop_ || coop()) coop_->begin();
    }
    void end() override {
        w_->end();
        if (coop_) coop_->end();
    }
    int send_packet(uint8_t* d, int l, uint64_t t) override { return w_->send_packet(d, l, t); }
    int recv_packet(uint8_t* d, uint64_t* t) override { return w_->recv_packet(d, t); }
    int send_cmd(uint8_t* d, int l, uint64_t t) override { return w_->send_cmd(d, l, t); }
    int send_reply(uint8_t* d, int l, uint64_t t, uint16_t aid) override { return w_->send_reply(d, l, t, aid); }
    int send_ack(uint8_t* d, int l, uint64_t t) override { return w_->send_ack(d, l, t); }
    int recv_host_packet(uint8_t* d, uint64_t* t) override { return w_->recv_host_packet(d, t); }
    uint16_t recv_replies(uint8_t* d, uint64_t t, uint16_t m) override { return w_->recv_replies(d, t, m); }
    // The co-op side channel: a shared ring of its own, made at the first use (a session, or the channel's first
    // call), with the same slot as the game's wireless.
    CoopTransport* coop() override {
        if (!coop_ && !coop_failed_) {
            coop_ = LocalCoop::open(w_->slot(), local_coop_name());
            coop_failed_ = !coop_;
            if (coop_ && multiplayer_active()) coop_->begin();
        }
        return coop_.get();
    }

private:
    LocalWireless* w_;
    std::unique_ptr<LocalCoop> coop_;
    bool coop_failed_ = false;
};

}  // namespace

void configure_lan(const LanConfig& config) { g_config = config; }

namespace {
std::atomic<bool> g_multiplayer{false};
}
void set_multiplayer_active(bool on) { g_multiplayer.store(on, std::memory_order_relaxed); }
bool multiplayer_active() { return g_multiplayer.load(std::memory_order_relaxed); }

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
