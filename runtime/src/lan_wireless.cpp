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
#include <cctype>
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

#include "connect_code.hpp"
#include "coop_channel.hpp"
#include "local_wireless.hpp"
#include "net_port.hpp"
#include "portmap.hpp"
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
constexpr uint32_t kStunCookie = 0x2112A442;
// Public STUN servers (several companies, so one gone or blocked is not the end), asked in turn. NTR_STUN_SERVERS
// (host:port,...) replaces them (tests).
constexpr const char* kStunServers[] = {"stun.l.google.com:19302", "stun.cloudflare.com:3478", "stun1.l.google.com:19302",
                                        "global.stun.twilio.com:3478", "stun.nextcloud.com:3478"};
constexpr int kPunchFailMs = 15000;  // reaching the other PC: "no answer yet" after this long

enum Kind : uint8_t { Hello = 0, Frame = 1, Begin = 2, End = 3, Bye = 4, Coop = 5, Net = 6 };
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

class LanWireless final : public WirelessLink, public CoopTransport, public NetPort {
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
        if (stun_thread_.joinable()) stun_thread_.join();
        if (sock_ != kNoSocket) close_socket(sock_);
        mapper_.stop();
    }

    void shutdown() { mapper_.stop(); }

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

    // Internet mode's own datagrams (coop_net.cpp): to one PC or all, handed on by the host like the side channel's.
    uint32_t net_node() const override { return id_; }
    void net_send(uint32_t to, const uint8_t* data, size_t len) override {
        if (len <= kNetDatagram) send_all(Net, 0, 0, to, data, int(len), 0);
    }
    bool net_recv(uint32_t& from, std::vector<uint8_t>& data) override {
        std::lock_guard lock(mutex_);
        if (net_.empty()) return false;
        from = net_.front().sender;
        data = std::move(net_.front().data);
        net_.pop_front();
        return true;
    }
    void net_peers(std::vector<uint32_t>& out) override {
        out.clear();
        std::lock_guard lock(mutex_);
        for (const auto& [id, peer] : peers_) out.push_back(id);
    }

    LanStatus status() {
        std::lock_guard lock(mutex_);
        LanStatus s;
        s.active = true;
        s.hosting = config_.mode == LanConfig::Mode::Host;
        s.peers = int(peers_.size());
        s.in_session = int(session_.size());
        if (config_.mode == LanConfig::Mode::Host && config_.port_mapping) {
            const PortMapStatus m = mapper_.status();
            s.router = int(m.state);
            const std::string port = std::to_string(config_.port);
            switch (m.state) {
            case PortMapStatus::Trying: s.router_text = "opening UDP " + port + " on the router"; break;
            case PortMapStatus::Mapped:
                s.router_text = "UDP " + (m.external_port == config_.port ? port : port + " (as " + std::to_string(m.external_port) + ")") +
                                " open on the router (" + m.method + (m.external_ip.empty() ? "" : ", " + m.external_ip) + ")";
                break;
            case PortMapStatus::Failed: s.router_text = "UDP " + port + " not opened on the router: forward it by hand"; break;
            default: break;
            }
        }
        if (config_.stun) {
            s.stun = stun_state_;
            ConnectCode c;
            if (code_locked(c)) {
                s.code = encode_connect_code(c);
                s.code_open = c.open;
                if (c.local) s.code_note = "no internet address learned (no answer from the STUN servers): this code is your address on this network";
                else if (c.symmetric && !c.open)
                    s.code_note = "your router gives a new port for each address (a \"symmetric\" NAT): a direct connection is unlikely "
                                  "unless the other side's port is open (UPnP) or forwarded";
            }
            const auto now = Clock::now();
            if (config_.mode == LanConfig::Mode::Host && !punch_.empty()) {
                bool reached = false;
                for (const auto& [id, peer] : peers_) reached = reached || same(peer.addr, punch_.front());
                s.punch = reached ? 2 : now - punch_since_ > std::chrono::milliseconds(kPunchFailMs) ? 3 : 1;
            } else if (config_.mode == LanConfig::Mode::Join && code_join_) {
                s.punch = !searching_ ? 2 : now - punch_since_ > std::chrono::milliseconds(kPunchFailMs) ? 3 : 1;
            }
            if (s.punch == 1) s.punch_text = "reaching the other PC";
            else if (s.punch == 2) s.punch_text = "reached the other PC";
            else if (s.punch == 3)
                s.punch_text = std::string("no answer from the other PC yet") +
                               (punch_symmetric_ ? " (its router gives a new port for each address, a \"symmetric\" NAT: it can "
                                                   "only reach a host whose port is open)"
                                                 : ": check the codes") +
                               "; if it goes on, open the host's port (UPnP on its router, or forward UDP " +
                               std::to_string(config_.port) + " to it) or use a VPN such as Tailscale";
        }
        return s;
    }

    // The other PC's code or address (lan_set_peer).
    bool set_peer(const std::string& text, std::string* error) {
        ConnectCode code;
        sockaddr_in addr{};
        bool is_code = decode_connect_code(text, code);
        if (is_code) {
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(code.ip);
            addr.sin_port = htons(code.port);
        } else if (!resolve(text, config_.port, addr)) {
            if (error) *error = "not a connect code or an address";
            return false;
        }
        std::lock_guard lock(mutex_);
        punch_since_ = Clock::now();
        if (config_.mode == LanConfig::Mode::Host) {
            punch_.assign(1, addr);
            punch_ttl_ = false;
            punch_symmetric_ = is_code && code.symmetric;
            log("lan: reaching the guest at %s (%s)\n", addr_text(addr).c_str(), is_code ? "its code" : "its address");
            punch_now_ = true;
        } else if (config_.mode == LanConfig::Mode::Join) {
            host_addr_ = addr;
            code_join_ = is_code;
            searching_ = is_code;  // (a code: the host is whoever answers from it, or from this network)
            punch_ttl_ = is_code && !code.open;
            keep_port_ = punch_ttl_;  // (our code goes to the host)
            if (is_code) {
                int on = 1;
                ::setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&on), sizeof on);
            }
            log("lan: joining %s (%s%s)\n", addr_text(addr).c_str(), is_code ? "its code" : "its address",
                punch_ttl_ ? ": its port is not open, so our first datagrams go only as far as our own router" : "");
        }
        return true;
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
        sock_ = make_socket(config_.mode == LanConfig::Mode::Host ? config_.port : 0);
        if (sock_ == kNoSocket)
            return fail(config_.mode == LanConfig::Mode::Host
                            ? "UDP port " + std::to_string(config_.port) + " is taken (another program, or a host already running here)"
                            : "no UDP socket");
        opened_ = Clock::now();
        {
            sockaddr_in me{};
#ifdef _WIN32
            int len = sizeof me;
#else
            socklen_t len = sizeof me;
#endif
            ::getsockname(sock_, reinterpret_cast<sockaddr*>(&me), &len);
            bound_port_ = ntohs(me.sin_port);
            local_ip_ = local_address();
        }
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
        } else if (config_.mode == LanConfig::Mode::Join && is_code(config_.host)) {
            set_peer(config_.host, nullptr);
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
            if (config_.port_mapping) mapper_.start(config_.port);  // (the port opened on the router, portmap.hpp)
            if (!config_.peer.empty()) set_peer(config_.peer, nullptr);
        }
        if (config_.stun) stun_thread_ = std::thread([this] { resolve_stun_servers(); });
        if (loss_ > 0 || delay_ms_ > 0 || jitter_ms_ > 0) {
            log("lan: testing a bad network: %.1f%% of the datagrams sent%s dropped, the others held %d ms (+0..%d)\n",
                loss_, loss_coop_only_ ? " (side channel only)" : "", delay_ms_, jitter_ms_);
            if (delay_ms_ > 0 || jitter_ms_ > 0) impair_thread_ = std::thread([this] { impair_loop(); });
        }
        thread_ = std::thread([this] { receive_loop(); });
        return true;
    }

    // A UDP socket bound to `port` (0: any), its reads waiting at most 100 ms.
    static socket_t make_socket(uint16_t port) {
        socket_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == kNoSocket) return s;
        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(port);
        if (::bind(s, reinterpret_cast<sockaddr*>(&local), sizeof local) != 0) {
            close_socket(s);
            return kNoSocket;
        }
#ifdef _WIN32
        DWORD timeout = 100;
        ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof timeout);
        // (an ICMP "port unreachable" for an earlier datagram must not fail the next read: Windows reports it there)
        BOOL report = FALSE;
        DWORD bytes = 0;
        WSAIoctl(s, _WSAIOW(IOC_VENDOR, 12), &report, sizeof report, nullptr, 0, &bytes, nullptr, nullptr);
#else
        timeval timeout{0, 100000};
        ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif
        return s;
    }

    // Joining a host by its address, with no answer yet (or none for a while): a new socket, on a new port. A router
    // that got our datagrams before the host's port was open on it (a port opened by UPnP a moment after we started,
    // or the host's game started again) keeps taking them for itself for as long as they keep coming (Linux's
    // connection tracking: an entry for the router's own port, renewed by every datagram), so the same port would
    // never get through; from a new port they reach the host as soon as its port is open.
    void maybe_new_port(Clock::time_point now) {
        if (config_.mode != LanConfig::Mode::Join || (searching_ && !code_join_) || keep_port_) return;
        if (now - std::max(heard_host_, opened_) < std::chrono::milliseconds(kNewPortMs)) return;
        socket_t s = make_socket(0);
        if (s == kNoSocket) return;
        socket_t old;
        {
            std::lock_guard lock(sock_mutex_);
            old = sock_;
            sock_ = s;
        }
        close_socket(old);
        opened_ = now;
        sockaddr_in me{};
#ifdef _WIN32
        int len = sizeof me;
#else
        socklen_t len = sizeof me;
#endif
        ::getsockname(s, reinterpret_cast<sockaddr*>(&me), &len);
        log("lan: no answer from the host for %d s: trying again from a new port (%u)\n", kNewPortMs / 1000, unsigned(ntohs(me.sin_port)));
    }

    // This PC's address on its network: the one its route to the internet leaves from (no datagram is sent).
    static uint32_t local_address() {
        socket_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == kNoSocket) return 0;
        sockaddr_in to{};
        to.sin_family = AF_INET;
        to.sin_port = htons(9);
        ::inet_pton(AF_INET, "192.0.2.1", &to.sin_addr);  // (any outside address: only the route is looked up)
        uint32_t ip = 0;
        if (::connect(s, reinterpret_cast<sockaddr*>(&to), sizeof to) == 0) {
            sockaddr_in me{};
#ifdef _WIN32
            int len = sizeof me;
#else
            socklen_t len = sizeof me;
#endif
            if (::getsockname(s, reinterpret_cast<sockaddr*>(&me), &len) == 0) ip = ntohl(me.sin_addr.s_addr);
        }
        close_socket(s);
        return ip;
    }

    static bool is_code(const std::string& text) {
        ConnectCode c;
        return decode_connect_code(text, c);
    }

    static std::string addr_text(const sockaddr_in& a) {
        char text[INET_ADDRSTRLEN] = {};
        ::inet_ntop(AF_INET, &a.sin_addr, text, sizeof text);
        return std::string(text) + ":" + std::to_string(ntohs(a.sin_port));
    }

    static bool same(const sockaddr_in& a, const sockaddr_in& b) {
        return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
    }

    // "name", "a.b.c.d" or either with ":port" (else `port`).
    static bool resolve(const std::string& text, uint16_t port, sockaddr_in& out) {
        std::string host = text;
        while (!host.empty() && std::isspace(static_cast<unsigned char>(host.back()))) host.pop_back();
        while (!host.empty() && std::isspace(static_cast<unsigned char>(host.front()))) host.erase(host.begin());
        if (const size_t colon = host.rfind(':'); colon != std::string::npos) {
            port = uint16_t(std::atoi(host.c_str() + colon + 1));
            host = host.substr(0, colon);
        }
        if (host.empty() || port == 0) return false;
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if (::getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res) return false;
        out = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
        out.sin_port = htons(port);
        ::freeaddrinfo(res);
        return true;
    }

    // ---- STUN (RFC 5389 Binding, on the co-op socket itself: the address the router gives it) ----

    void resolve_stun_servers() {
        std::vector<std::string> names;
        if (const char* e = std::getenv("NTR_STUN_SERVERS")) {
            std::string list = e;
            for (size_t p = 0; p <= list.size();) {
                const size_t comma = list.find(',', p);
                const std::string one = list.substr(p, comma == std::string::npos ? std::string::npos : comma - p);
                if (!one.empty()) names.push_back(one);
                if (comma == std::string::npos) break;
                p = comma + 1;
            }
        } else {
            names.assign(std::begin(kStunServers), std::end(kStunServers));
        }
        std::vector<StunAsk> asks;
        for (const std::string& n : names) {
            if (stop_) return;
            StunAsk a;
            if (!resolve(n, 3478, a.addr)) {
                log("lan: STUN server %s not found\n", n.c_str());
                continue;
            }
            bool dup = false;
            for (const auto& b : asks) dup = dup || same(a.addr, b.addr);
            if (!dup) asks.push_back(a);
        }
        std::lock_guard lock(mutex_);
        stun_ = std::move(asks);
        stun_state_ = stun_.empty() ? 3 : 1;
        stun_started_ = Clock::now();
        if (stun_.empty()) log("lan: no STUN server could be found (no internet?): the connect code is this PC's local address\n");
    }

    // The receiving thread, every 100 ms: asks the next servers (each once, again after 500 ms, at most 5 times),
    // until two have answered (to compare their ports) or all gave up; again every 60 s while not in a session (the
    // router may have changed it).
    void stun_tick(Clock::time_point now) {
        std::vector<std::pair<sockaddr_in, std::vector<uint8_t>>> out;
        {
            std::lock_guard lock(mutex_);
            if (stun_.empty()) return;
            if ((stun_state_ == 2 || stun_state_ == 3) && !in_session_ && now - stun_started_ > std::chrono::seconds(60)) {
                for (auto& a : stun_) a.tries = 0, a.answered = false;
                stun_state_ = stun_state_ == 2 ? 2 : 1;
                stun_started_ = now;
            }
            int answered = 0, pending = 0;
            for (const auto& a : stun_) {
                answered += a.answered;
                pending += !a.answered && a.tries < 5;
            }
            if (answered >= 2 || pending == 0) {
                if (stun_state_ == 1) stun_state_ = answered ? 2 : 3;
                if (stun_state_ == 3 && !stun_logged_fail_) {
                    stun_logged_fail_ = true;
                    log("lan: no STUN server answered (UDP blocked, or no internet): the connect code is this PC's local address\n");
                }
                return;
            }
            // Ask the first two that have not answered (in order: the next only once one gave up).
            int asking = 0;
            for (auto& a : stun_) {
                if (a.answered || a.tries >= 5) continue;
                if (++asking > 2 - answered) break;
                if (a.tries > 0 && now - a.sent < std::chrono::milliseconds(500)) continue;
                a.tries++;
                a.sent = now;
                std::random_device rd;
                for (auto& b : a.tid) b = uint8_t(rd());
                std::vector<uint8_t> q(20);
                q[0] = 0x00, q[1] = 0x01;  // Binding Request, no attributes
                q[4] = 0x21, q[5] = 0x12, q[6] = 0xA4, q[7] = 0x42;
                std::memcpy(&q[8], a.tid, 12);
                out.emplace_back(a.addr, std::move(q));
            }
        }
        for (auto& [to, q] : out) send_now(to, q);
    }

    // The code in the log when it is known or changes; and (tests) NTR_LAN_PEER_FILE: a file whose text, when it
    // changes, is given to set_peer (as the settings do while the game runs).
    void log_code(Clock::time_point now) {
        if (now - code_checked_ < std::chrono::seconds(1)) return;
        code_checked_ = now;
        std::string code;
        {
            std::lock_guard lock(mutex_);
            ConnectCode c;
            if (code_locked(c)) code = encode_connect_code(c) + (c.open ? " (port open)" : "") + (c.symmetric ? " (symmetric NAT)" : "") +
                                       (c.local ? " (a local address)" : "");
        }
        if (code != logged_code_) {
            logged_code_ = code;
            if (!code.empty()) log("lan: this PC's connect code: %s\n", code.c_str());
        }
        const LanStatus st = status();
        if (st.punch != logged_punch_) {
            logged_punch_ = st.punch;
            if (st.punch >= 2) log("lan: %s\n", st.punch_text.c_str());
        }
        if (const char* f = std::getenv("NTR_LAN_PEER_FILE")) {
            std::string text;
            if (FILE* fp = std::fopen(f, "rb")) {
                char b[256];
                const size_t n = std::fread(b, 1, sizeof b - 1, fp);
                std::fclose(fp);
                text.assign(b, n);
                while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
            }
            if (!text.empty() && text != peer_file_text_) {
                peer_file_text_ = text;
                std::string why;
                if (!set_peer(text, &why)) log("lan: %s: %s\n", text.c_str(), why.c_str());
            }
        }
    }

    // A datagram that may be a STUN answer (true: it was one).
    bool stun_answer(const uint8_t* d, int n, const sockaddr_in& from) {
        if (n < 20 || d[0] != 0x01 || d[1] != 0x01) return false;
        if (uint32_t(d[4]) << 24 != (kStunCookie & 0xFF000000u) || d[5] != 0x12 || d[6] != 0xA4 || d[7] != 0x42) return false;
        std::lock_guard lock(mutex_);
        for (auto& a : stun_) {
            if (!same(a.addr, from) || std::memcmp(a.tid, d + 8, 12) != 0) continue;
            const int len = std::min(n - 20, int(d[2] << 8 | d[3]));
            for (int p = 20; p + 4 <= 20 + len;) {
                const int type = d[p] << 8 | d[p + 1], alen = d[p + 2] << 8 | d[p + 3];
                if (p + 4 + alen > n) break;
                const uint8_t* v = d + p + 4;
                if ((type == 0x0020 || type == 0x8020 || type == 0x0001) && alen >= 8 && v[1] == 0x01) {
                    uint16_t port = uint16_t(v[2] << 8 | v[3]);
                    uint32_t ip = uint32_t(v[4]) << 24 | uint32_t(v[5]) << 16 | uint32_t(v[6]) << 8 | v[7];
                    if (type != 0x0001) {
                        port ^= uint16_t(kStunCookie >> 16);
                        ip ^= kStunCookie;
                    }
                    if (!a.answered || type != 0x0001) {
                        a.ip = ip;
                        a.port = port;
                    }
                    a.answered = true;
                }
                p += 4 + ((alen + 3) & ~3);
            }
            if (a.answered) {
                // The first server to answer gives our address; two that see different ports: a symmetric NAT.
                int answered = 0;
                bool differ = false;
                for (const auto& b : stun_)
                    if (b.answered) {
                        answered++;
                        differ = differ || b.ip != a.ip || b.port != a.port;
                    }
                if (answered == 1) {
                    if (a.ip != public_ip_ || a.port != public_port_)
                        log("lan: STUN: this PC is %u.%u.%u.%u:%u on the internet\n", a.ip >> 24, (a.ip >> 16) & 255,
                            (a.ip >> 8) & 255, a.ip & 255, unsigned(a.port));
                    public_ip_ = a.ip, public_port_ = a.port;
                } else {
                    if (differ && !symmetric_)
                        log("lan: STUN: another server sees another port (%u): the router gives a new port for each "
                            "address (a symmetric NAT)\n", unsigned(a.port));
                    symmetric_ = differ;
                }
            }
            return true;
        }
        return true;  // (a STUN answer to nothing asked: dropped)
    }

    // This PC's code (under mutex_; false: not known yet).
    bool code_locked(ConnectCode& c) {
        const PortMapStatus m = config_.mode == LanConfig::Mode::Host && config_.port_mapping ? mapper_.status() : PortMapStatus{};
        // (not while the router is still being asked: the code says whether the port is open, and a guest given a code
        // that says "not open" waits for the host to give its own code)
        if (m.state == PortMapStatus::Trying) return false;
        uint32_t mapped_ip = 0;
        if (m.state == PortMapStatus::Mapped && !m.external_ip.empty()) {
            in_addr a;
            if (::inet_pton(AF_INET, m.external_ip.c_str(), &a) == 1) mapped_ip = ntohl(a.s_addr);
        }
        if (public_ip_) {
            c.ip = public_ip_;
            c.port = public_port_;
            // (the router's own public address must be ours: behind a second router, e.g. the provider's, a port opened
            // on the first is of no use)
            if (m.state == PortMapStatus::Mapped && (!mapped_ip || mapped_ip == public_ip_)) {
                c.port = m.external_port;
                c.open = true;
            }
            c.symmetric = symmetric_;
            return true;
        }
        if (stun_state_ == 1 || stun_state_ == 0) return false;
        if (mapped_ip && !private_ip(mapped_ip)) {
            c.ip = mapped_ip;
            c.port = m.external_port;
            c.open = true;
            return true;
        }
        // No internet address: this PC's on its network (for a LAN, or a VPN's network).
        c.ip = local_ip_;
        c.port = bound_port_;
        c.local = true;
        return c.ip != 0;
    }

    static bool private_ip(uint32_t ip) {
        return (ip >> 24) == 10 || (ip >> 20) == (172 << 4 | 1) || (ip >> 16) == (192 << 8 | 168) ||
               (ip >> 22) == (100 << 2 | 1) || (ip >> 24) == 127 || (ip >> 16) == (169 << 8 | 254);
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

    void send_now(const sockaddr_in& to, const std::vector<uint8_t>& buf, int ttl = 0) {
        std::lock_guard lock(sock_mutex_);
        int old = 64;
        if (ttl > 0) {
#ifdef _WIN32
            int len = sizeof old;
#else
            socklen_t len = sizeof old;
#endif
            ::getsockopt(sock_, IPPROTO_IP, IP_TTL, reinterpret_cast<char*>(&old), &len);
            ::setsockopt(sock_, IPPROTO_IP, IP_TTL, reinterpret_cast<const char*>(&ttl), sizeof ttl);
        }
        ::sendto(sock_, reinterpret_cast<const char*>(buf.data()), int(buf.size()), 0,
                 reinterpret_cast<const sockaddr*>(&to), sizeof to);
        if (ttl > 0) ::setsockopt(sock_, IPPROTO_IP, IP_TTL, reinterpret_cast<const char*>(&old), sizeof old);
    }

    // Hole punching (docs/COOP_INTERNET.md, "Connect codes"): the first datagram to arrive at a router for a port
    // nobody inside has sent from to that address is taken by the router for itself, and a Linux router (most home
    // routers) keeps doing so while more come, and then gives our partner's own datagrams to us another port (the
    // pair of addresses is taken): the way is blocked for good. So the guest, which has the host's code first, sends
    // its hellos with a short life (2, 3, 4 routers: they open its own router's port towards the host and die on the
    // way), until the host, given the guest's code, sends to it (its datagrams arrive through the port the guest's
    // router opened) and the guest answers in full. NTR_PUNCH_TTL=<n> sets one life (tests).
    int punch_ttl() {
        if (const char* e = std::getenv("NTR_PUNCH_TTL")) return std::max(1, std::atoi(e));
        static constexpr int kTtl[] = {2, 3, 4};
        return kTtl[punch_ttl_turn_++ % 3];
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
            bool code_search, low;
            {
                std::lock_guard lock(mutex_);
                if (searching_ && kind != Hello) return;  // (no host yet)
                to = host_addr_;
                code_search = searching_ && code_join_;
                low = punch_ttl_;
            }
            if (code_search) {
                // (by code: the host's address, and this network in case the host is on it)
                send_now(to, buf, low ? punch_ttl() : 0);
                sockaddr_in all{};
                all.sin_family = AF_INET;
                all.sin_addr.s_addr = htonl(INADDR_BROADCAST);
                all.sin_port = htons(config_.port);
                send_now(all, buf);
                return;
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

    // Hosting with a guest's code (or address) to reach: a hello to it, until it is one of the PCs heard from.
    void punch_tick(uint8_t state, uint64_t epoch) {
        if (config_.mode != LanConfig::Mode::Host) return;
        std::vector<sockaddr_in> to;
        {
            std::lock_guard lock(mutex_);
            for (const sockaddr_in& a : punch_) {
                bool known = false;
                for (const auto& [id, peer] : peers_) known = known || same(peer.addr, a);
                if (!known) to.push_back(a);
            }
        }
        if (to.empty()) return;
        std::vector<uint8_t> buf;
        put(buf, Hello, state, 0, id_, kHostMark, nullptr, 0, epoch);
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
                maybe_new_port(now);
                punch_tick(state, epoch);
                if ((loss_ > 0 || delay_ms_ > 0 || jitter_ms_ > 0) && ++hellos_ % 20 == 0) {
                    std::lock_guard lock(impair_mutex_);
                    log("lan: testing a bad network: %llu of %llu datagrams dropped\n",
                        (unsigned long long)impair_dropped_, (unsigned long long)impair_sent_);
                }
            }
            if (config_.stun) {
                stun_tick(now);
                log_code(now);
            }
            {
                bool at_once = false;
                uint8_t state;
                uint64_t epoch;
                {
                    std::lock_guard lock(mutex_);
                    std::swap(at_once, punch_now_);
                    state = uint8_t(kHelloState | (in_session_ ? kHelloInSession : 0));
                    epoch = epoch_;
                }
                if (at_once) punch_tick(state, epoch);
            }
            sockaddr_in from{};
#ifdef _WIN32
            int from_len = sizeof from;
#else
            socklen_t from_len = sizeof from;
#endif
            socket_t sock;
            {
                std::lock_guard lock(sock_mutex_);
                sock = sock_;
            }
            const int n = int(::recvfrom(sock, reinterpret_cast<char*>(buf.data()), int(buf.size()), 0,
                                         reinterpret_cast<sockaddr*>(&from), &from_len));
            if (n >= 20 && buf[0] == 0x01 && stun_answer(buf.data(), n, from)) continue;
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
            if (!hosting) heard_host_ = Clock::now();  // (joining: everything comes from the host)
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
            if (h.kind == Net && (h.target == 0 || h.target == id_)) {
                net_.push_back(Message{h.sender, 0, 0, 0, std::vector<uint8_t>(payload, payload + h.length)});
                if (net_.size() > kMaxCoopQueued) net_.pop_front();
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
        if (kind == Coop || kind == Net) return target == 0 || id == target;
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

    struct StunAsk {
        sockaddr_in addr{};
        uint8_t tid[12] = {};
        Clock::time_point sent{};
        int tries = 0;
        bool answered = false;
        uint32_t ip = 0;
        uint16_t port = 0;
    };

    LanConfig config_;
    PortMapper mapper_;
    std::thread stun_thread_;
    std::vector<StunAsk> stun_;  // (under mutex_, as everything of STUN and punching below)
    int stun_state_ = 0;
    bool stun_logged_fail_ = false;
    Clock::time_point stun_started_{};
    uint32_t public_ip_ = 0;
    uint16_t public_port_ = 0;
    bool symmetric_ = false;
    uint32_t local_ip_ = 0;   // this PC's address on its network (the route to the internet)
    uint16_t bound_port_ = 0;
    // Punching: a host's guest to reach (its code or address); a joining PC's host by code.
    std::vector<sockaddr_in> punch_;
    bool punch_now_ = false;   // (send to it at once)
    bool punch_ttl_ = false;   // joining by the code of a host whose port is not open: our hellos to it go no
                               // further than our own router(s) until it answers (see punch_ttl)
    bool code_join_ = false;
    bool punch_symmetric_ = false;  // (the guest's code says its router is a symmetric NAT)
    Clock::time_point punch_since_{};
    int punch_ttl_turn_ = 0;
    Clock::time_point code_checked_{};
    std::string logged_code_, peer_file_text_;
    int logged_punch_ = 0;
    uint32_t id_ = 0;
    socket_t sock_ = kNoSocket;
    std::mutex sock_mutex_;  // (sock_ is replaced by maybe_new_port, on the receiving thread)
    Clock::time_point opened_{}, heard_host_{};
    bool keep_port_ = false;  // (the port must stay: our address was given to the host as a connect code)
    static constexpr int kNewPortMs = 8000;
    sockaddr_in host_addr_{};
    std::string error_;
    std::thread thread_;
    std::atomic<bool> stop_{false};

    std::mutex mutex_;
    std::condition_variable cv_frames_, cv_replies_;
    std::map<uint32_t, Peer> peers_;
    std::map<uint32_t, bool> session_;  // other PCs in the game's multiplayer
    std::deque<Message> frames_, replies_, coop_, net_;
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

// Co-op over the internet (docs/COOP_INTERNET.md): the game's radio talks to the stand-in copy on this PC over the
// same-PC link (shared memory: the exchange takes microseconds); the UDP link to the other PC carries only the side
// channel (and coop_net.cpp's datagrams). Its idea of who is in the game's multiplayer follows this console's.
class InternetLink final : public WirelessLink {
public:
    InternetLink(LocalWireless* w, LanWireless* lan) : w_(w), lan_(lan) {}
    bool mac_offset(uint8_t out[3]) const override { return lan_->mac_offset(out); }
    void begin() override {
        w_->begin();
        lan_->begin();
    }
    void end() override {
        w_->end();
        lan_->end();
    }
    int send_packet(uint8_t* d, int l, uint64_t t) override { return w_->send_packet(d, l, t); }
    int recv_packet(uint8_t* d, uint64_t* t) override { return w_->recv_packet(d, t); }
    int send_cmd(uint8_t* d, int l, uint64_t t) override { return w_->send_cmd(d, l, t); }
    int send_reply(uint8_t* d, int l, uint64_t t, uint16_t aid) override { return w_->send_reply(d, l, t, aid); }
    int send_ack(uint8_t* d, int l, uint64_t t) override { return w_->send_ack(d, l, t); }
    int recv_host_packet(uint8_t* d, uint64_t* t) override { return w_->recv_host_packet(d, t); }
    uint16_t recv_replies(uint8_t* d, uint64_t t, uint16_t m) override { return w_->recv_replies(d, t, m); }
    CoopTransport* coop() override { return lan_; }

private:
    LocalWireless* w_;
    LanWireless* lan_;
};

}  // namespace

NetPort* lan_net_port() { return g_config.mode == LanConfig::Mode::Off ? nullptr : lan_link(); }

void configure_lan(const LanConfig& config) { g_config = config; }

namespace {
std::atomic<bool> g_multiplayer{false};
}
void set_multiplayer_active(bool on) { g_multiplayer.store(on, std::memory_order_relaxed); }
bool multiplayer_active() { return g_multiplayer.load(std::memory_order_relaxed); }

std::unique_ptr<WirelessLink> make_lan_link(const LanConfig& config) { return std::unique_ptr<WirelessLink>(LanWireless::create(config)); }

bool lan_set_peer(const std::string& text, std::string* error) {
    LanWireless* l = g_config.mode == LanConfig::Mode::Off ? nullptr : lan_link();
    if (!l) {
        if (error) *error = "co-op between PCs is off";
        return false;
    }
    return l->set_peer(text, error);
}

void lan_shutdown() {
    if (LanWireless* l = g_config.mode == LanConfig::Mode::Off ? nullptr : lan_link()) l->shutdown();
}

LanStatus lan_status() {
    if (LanWireless* l = g_config.mode == LanConfig::Mode::Off ? nullptr : lan_link()) return l->status();
    LanStatus s;
    s.error = LanWireless::g_error;
    return s;
}

WirelessLink* wireless_link() {
    static WirelessLink* link = []() -> WirelessLink* {
        if (g_config.mode != LanConfig::Mode::Off && net_internet_mode()) {
            LanWireless* lan = lan_link();
            LocalWireless* w = LocalWireless::get();
            if (lan && w) {
                log("lan: over the internet: the game's radio talks to the stand-in on this PC; the other PC gets our "
                    "own datagrams\n");
                return new InternetLink(w, lan);
            }
            log("lan: over the internet: no link (%s)\n", lan ? "the same-PC link did not open" : "no UDP socket");
            return lan;
        }
        if (g_config.mode != LanConfig::Mode::Off) return lan_link();
        if (LocalWireless* w = LocalWireless::get()) return new LocalLink(w);
        return nullptr;
    }();
    return link;
}

}  // namespace ntr
