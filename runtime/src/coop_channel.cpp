// The co-op side channel (include/ntr/coop.hpp, coop_channel.hpp): reliable, ordered messages of ours between the
// consoles of a session, over the LAN link's UDP socket or the shared-memory ring between copies on one PC.
//
// A datagram: 'Q', kind, 2 bytes 0, u32 stream, u32 sequence number; a data datagram then has flags (1 the first
// fragment of a message, 2 the last) and the message's type, then up to kPayload bytes of it. An ack names the
// stream and the last sequence number received in order (cumulative); "unknown" tells a sender that its stream is not
// the one the receiver follows (the receiver started again), so it starts a new one. A receiver keeps fragments that
// arrive past a missing one (up to kAhead), and its ack carries a bitmap of them (bit i: sequence number ack + 2 + i),
// so a sender resends only what was lost.
#include "coop_channel.hpp"

#include <algorithm>
#include <cstring>
#include <random>

#include "ntr/coop.hpp"
#include "runtime_internal.hpp"
#include "wireless_link.hpp"

namespace ntr {

namespace {

constexpr uint8_t kMagic = 'Q';
enum Kind : uint8_t { Data = 1, Ack = 2, Unknown = 3 };
constexpr size_t kHeader = 12, kDataHeader = kHeader + 2, kPayload = kCoopDatagram - kDataHeader;
constexpr uint32_t kAhead = 64;       // fragments a receiver keeps past a missing one (the ack's bitmap)
constexpr uint8_t kFirst = 1, kLast = 2;
constexpr size_t kWindow = 48;        // fragments in flight per receiver
constexpr size_t kMaxQueued = 8192;   // fragments waiting for one receiver before it is given up on (~9 MB)
constexpr auto kResend = std::chrono::milliseconds(150);
constexpr auto kStuck = std::chrono::milliseconds(500);  // no ack this long: an "unknown" may restart the stream

void put32(uint8_t* p, uint32_t v) { std::memcpy(p, &v, 4); }
uint32_t get32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

void header(uint8_t* p, uint8_t kind, uint32_t stream, uint32_t seq) {
    p[0] = kMagic, p[1] = kind, p[2] = 0, p[3] = 0;
    put32(p + 4, stream);
    put32(p + 8, seq);
}

uint32_t random_id() {
    static std::mt19937 rng{std::random_device{}()};
    uint32_t v;
    do v = uint32_t(rng()); while (v == 0);
    return v;
}

}  // namespace

CoopChannel::CoopChannel(CoopTransport* t) : t_(t) {}

CoopChannel::Out& CoopChannel::out(uint32_t peer) {
    auto it = out_.find(peer);
    if (it != out_.end()) return it->second;
    Out& o = out_[peer];
    o.stream = random_id();
    o.progress = Clock::now();
    return o;
}

// A new stream to the same receiver: what it has not acknowledged goes again, numbered from 1.
void CoopChannel::restart(Out& o) {
    o.stream = random_id();
    o.next = 1;
    for (Fragment& f : o.unacked) {
        f.seq = o.next++;
        put32(f.bytes.data() + 4, o.stream);
        put32(f.bytes.data() + 8, f.seq);
        f.ever_sent = false;
        f.held = false;
    }
    o.progress = Clock::now();
}

bool CoopChannel::send(uint32_t to, uint8_t type, const void* message, size_t len) {
    const auto* data = static_cast<const uint8_t*>(message);
    if (len > kCoopMaxMessage || peers_.empty()) return false;
    if (to != 0 && std::find(peers_.begin(), peers_.end(), to) == peers_.end()) return false;
    const auto now = Clock::now();
    for (uint32_t peer : peers_) {
        if (to != 0 && peer != to) continue;
        Out& o = out(peer);
        size_t at = 0;
        do {
            const size_t n = std::min(kPayload, len - at);
            Fragment f;
            f.seq = o.next++;
            f.bytes.resize(kDataHeader + n);
            header(f.bytes.data(), Data, o.stream, f.seq);
            f.bytes[kHeader] = uint8_t((at == 0 ? kFirst : 0) | (at + n == len ? kLast : 0));
            f.bytes[kHeader + 1] = type;
            if (n) std::memcpy(f.bytes.data() + kDataHeader, data + at, n);
            o.unacked.push_back(std::move(f));
            at += n;
        } while (at < len);
        if (o.unacked.size() > kMaxQueued) {
            log("coop: node %08x takes nothing in; %zu fragments for it dropped\n", peer, o.unacked.size());
            out_.erase(peer);
            continue;
        }
        transmit(peer, o, now);
    }
    return true;
}

void CoopChannel::transmit(uint32_t peer, Out& o, Clock::time_point now) {
    size_t n = 0;
    for (Fragment& f : o.unacked) {
        if (n++ >= kWindow) break;
        if (f.held || (f.ever_sent && now - f.sent < kResend)) continue;
        t_->coop_send(peer, f.bytes.data(), f.bytes.size());
        f.sent = now;
        f.ever_sent = true;
    }
}

// The next fragment in order: a message's part, or its end.
void CoopChannel::take(uint32_t from, In& in, const std::vector<uint8_t>& d, Done& done) {
    in.expect++;
    const uint8_t flags = d[kHeader];
    if (flags & kFirst) {
        in.partial.clear();
        in.assembling = true;
        in.type = d[kHeader + 1];
    }
    if (!in.assembling) return;  // (the rest of a message whose start went to an earlier stream)
    in.partial.insert(in.partial.end(), d.begin() + kDataHeader, d.end());
    if (flags & kLast) {
        done.push_back({from, {in.type, std::move(in.partial)}});
        in.partial.clear();
        in.assembling = false;
    }
}

void CoopChannel::handle(uint32_t from, const std::vector<uint8_t>& d, Done& done) {
    if (d.size() < kHeader || d[0] != kMagic || from == 0) return;
    const uint8_t kind = d[1];
    const uint32_t stream = get32(d.data() + 4), seq = get32(d.data() + 8);
    if (kind == Ack || kind == Unknown) {
        auto it = out_.find(from);
        if (it == out_.end() || it->second.stream != stream) return;
        Out& o = it->second;
        if (kind == Ack) {
            bool moved = false;
            while (!o.unacked.empty() && o.unacked.front().seq <= seq) o.unacked.pop_front(), moved = true;
            if (moved || o.unacked.empty()) o.progress = Clock::now();
            uint64_t held = 0;
            if (d.size() >= kHeader + 8) std::memcpy(&held, d.data() + kHeader, 8);
            for (Fragment& f : o.unacked)
                if (f.seq >= seq + 2 && f.seq < seq + 2 + kAhead && (held >> (f.seq - seq - 2) & 1)) f.held = true;
        } else if (Clock::now() - o.progress > kStuck) {
            // (only when stuck: an "unknown" can also come from a new stream's datagrams arriving out of order)
            log("coop: node %08x lost our stream; starting it again (%zu fragments)\n", from, o.unacked.size());
            restart(o);
        }
        return;
    }
    if (kind != Data || d.size() < kDataHeader) return;
    In& in = in_[from];
    if (stream != in.stream) {
        if (seq != 1) {  // (a stream we do not follow: tell the sender)
            uint8_t u[kHeader];
            header(u, Unknown, stream, seq);
            t_->coop_send(from, u, sizeof u);
            return;
        }
        in = In{};
        in.stream = stream;
    }
    in.ack_due = true;
    if (seq < in.expect) return;  // (a repeat)
    if (seq > in.expect) {        // (past one that was lost: kept until it comes again)
        if (seq <= in.expect + kAhead) in.ahead.emplace(seq, d);
        return;
    }
    take(from, in, d, done);
    for (auto it = in.ahead.begin(); it != in.ahead.end() && it->first <= in.expect;) {
        if (it->first == in.expect) take(from, in, it->second, done);
        it = in.ahead.erase(it);
    }
}

void CoopChannel::poll(const std::function<void(uint32_t, uint8_t, const uint8_t*, size_t)>& fn) {
    std::vector<uint32_t> now_peers;
    t_->coop_peers(now_peers);
    // A console that left: its streams end (if it comes back, both sides start new ones).
    for (uint32_t p : peers_)
        if (std::find(now_peers.begin(), now_peers.end(), p) == now_peers.end()) out_.erase(p), in_.erase(p);
    peers_ = std::move(now_peers);
    std::vector<std::pair<uint32_t, std::pair<uint8_t, std::vector<uint8_t>>>> done;
    uint32_t from = 0;
    std::vector<uint8_t> d;
    if (peers_.empty()) {  // (not in a session: nothing is kept)
        out_.clear();
        in_.clear();
        while (t_->coop_recv(from, d)) {
        }
        return;
    }
    for (int i = 0; i < 4096 && t_->coop_recv(from, d); i++) handle(from, d, done);
    for (auto& [peer, in] : in_) {
        if (!in.ack_due) continue;
        in.ack_due = false;
        uint8_t a[kHeader + 8];
        header(a, Ack, in.stream, in.expect - 1);
        uint64_t held = 0;
        for (const auto& [seq, frag] : in.ahead)
            if (seq >= in.expect + 1 && seq < in.expect + 1 + kAhead) held |= uint64_t(1) << (seq - in.expect - 1);
        std::memcpy(a + kHeader, &held, 8);
        t_->coop_send(peer, a, sizeof a);
    }
    const auto now = Clock::now();
    for (auto& [peer, o] : out_) transmit(peer, o, now);
    for (auto& [who, m] : done) fn(who, m.first, m.second.data(), m.second.size());
}

// ---- the process's channel, over the link the game's multiplayer uses ----

namespace {
CoopChannel* channel() {
    static CoopChannel* ch = []() -> CoopChannel* {
        WirelessLink* w = wireless_link();
        CoopTransport* t = w ? w->coop() : nullptr;
        return t ? new CoopChannel(t) : nullptr;
    }();
    return ch;
}
}  // namespace

bool coop_send(uint8_t type, const void* data, size_t len, uint32_t to) {
    CoopChannel* ch = channel();
    return ch && ch->send(to, type, data, len);
}

void coop_poll(CoopReceive fn, void* user) {
    if (CoopChannel* ch = channel())
        ch->poll([&](uint32_t from, uint8_t type, const uint8_t* data, size_t len) {
            if (fn) fn(from, type, data, len, user);
        });
}

uint32_t coop_node() {
    CoopChannel* ch = channel();
    return ch ? ch->node() : 0;
}

size_t coop_peers(uint32_t* out, size_t max) {
    CoopChannel* ch = channel();
    if (!ch) return 0;
    const auto& p = ch->peers();
    const size_t n = std::min(max, p.size());
    std::copy(p.begin(), p.begin() + std::ptrdiff_t(n), out);
    return n;
}

}  // namespace ntr
