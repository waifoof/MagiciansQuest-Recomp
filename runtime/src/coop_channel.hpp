// The co-op side channel's inside (the API is include/ntr/coop.hpp): a link that carries datagrams of ours between
// the consoles of a session (CoopTransport: the LAN link's UDP socket, or a shared-memory ring between copies on this
// PC), and the layer that makes them reliable and ordered (CoopChannel).
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "ntr/coop.hpp"

namespace ntr {

// Datagrams between the consoles of a session: may be lost, duplicated or reordered (UDP), or lost when a reader falls
// too far behind (the shared ring). At most kCoopDatagram bytes each.
constexpr size_t kCoopDatagram = 1200;

class CoopTransport {
public:
    virtual ~CoopTransport() = default;
    virtual uint32_t coop_node() const = 0;  // this console's node, never 0
    // The other nodes in the game's multiplayer with this one (none while this one is not in it).
    virtual void coop_peers(std::vector<uint32_t>& out) = 0;
    virtual void coop_send(uint32_t to, const uint8_t* data, size_t len) = 0;  // to one node, or every other (0)
    virtual bool coop_recv(uint32_t& from, std::vector<uint8_t>& data) = 0;    // the next one, if any
};

// The shared-memory transport between copies on this PC: node slot + 1. name: the shared memory's name (POSIX: a
// leading '/'). begin/end mark this copy in or out of the game's multiplayer (LocalLink calls them with MP_Begin/End).
class LocalCoop : public CoopTransport {
public:
    static std::unique_ptr<LocalCoop> open(int slot, const char* name);
    virtual void begin() = 0;
    virtual void end() = 0;
};
// The name for this process's copies (NTR_LINK_NAME gives a set of copies a link of their own, as local_wireless.cpp).
const char* local_coop_name();

// Reliable, ordered messages over a transport. Each sender numbers its datagrams per receiver (a stream), resends what
// is not acknowledged, and the receiver hands on only the next in order; a message bigger than a datagram goes in
// fragments. Streams start again when a console leaves the session or the receiver no longer knows the stream (it
// restarted): what was not yet acknowledged is sent again in the new one.
class CoopChannel {
public:
    explicit CoopChannel(CoopTransport* t);
    bool send(uint32_t to, uint8_t type, const void* data, size_t len);
    void poll(const std::function<void(uint32_t from, uint8_t type, const uint8_t* data, size_t len)>& fn);
    uint32_t node() const { return t_->coop_node(); }
    const std::vector<uint32_t>& peers() const { return peers_; }

private:
    using Clock = std::chrono::steady_clock;
    struct Fragment {
        uint32_t seq;
        std::vector<uint8_t> bytes;  // the datagram, header and all (its seq rewritten if the stream starts again)
        Clock::time_point sent{};
        bool ever_sent = false;
        bool held = false;  // the receiver has it, ahead of one it is missing (its ack's bitmap)
    };
    struct Out {
        uint32_t stream = 0;
        uint32_t next = 1;
        std::deque<Fragment> unacked;
        Clock::time_point progress{};  // the last ack that took something off (or the stream's start)
    };
    struct In {
        uint32_t stream = 0;
        uint32_t expect = 1;
        bool ack_due = false;
        bool assembling = false;
        uint8_t type = 0;
        std::vector<uint8_t> partial;
        std::map<uint32_t, std::vector<uint8_t>> ahead;  // fragments past a missing one, kept until it comes
    };
    Out& out(uint32_t peer);
    void restart(Out& o);
    using Done = std::vector<std::pair<uint32_t, std::pair<uint8_t, std::vector<uint8_t>>>>;
    void handle(uint32_t from, const std::vector<uint8_t>& d, Done& done);
    static void take(uint32_t from, In& in, const std::vector<uint8_t>& d, Done& done);
    void transmit(uint32_t peer, Out& o, Clock::time_point now);

    CoopTransport* t_;
    std::vector<uint32_t> peers_;
    std::map<uint32_t, Out> out_;
    std::map<uint32_t, In> in_;
};

}  // namespace ntr
