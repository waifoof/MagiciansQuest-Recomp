// The co-op side channel (runtime/src/coop_channel.cpp): messages between the consoles of a session, reliable and in
// order, over the LAN link (a host and two joining PCs on loopback) and over the shared ring between copies on one PC;
// also through a link that loses and repeats datagrams, a console leaving, and one starting again in its place.
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "coop_channel.hpp"
#include "ntr/lan.hpp"
#include "wireless_link.hpp"

// (the runtime's log, which the links write to)
namespace ntr {
void log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
}
}  // namespace ntr

static int failures = 0;
#define CHECK(c)                                                            \
    do {                                                                    \
        if (!(c)) {                                                         \
            std::fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #c); \
            failures++;                                                     \
        }                                                                   \
    } while (0)

using namespace std::chrono_literals;

struct Got {
    uint32_t from;
    uint8_t type;
    std::vector<uint8_t> data;
};

// A console: its channel and what it received.
struct Node {
    explicit Node(ntr::CoopChannel* c) : ch(c) {}
    ntr::CoopChannel* ch;
    std::vector<Got> got;
    void poll() {
        ch->poll([this](uint32_t from, uint8_t type, const uint8_t* d, size_t n) {
            got.push_back({from, type, std::vector<uint8_t>(d, d + n)});
        });
    }
};

// Polls every node (about a frame apart) until done() or the time is up.
template <typename F>
bool pump(std::vector<Node*> nodes, F done, std::chrono::milliseconds limit = 5000ms) {
    const auto end = std::chrono::steady_clock::now() + limit;
    while (std::chrono::steady_clock::now() < end) {
        for (Node* n : nodes) n->poll();
        if (done()) return true;
        std::this_thread::sleep_for(2ms);
    }
    return false;
}

std::vector<uint8_t> pattern(size_t n, uint32_t seed) {
    std::vector<uint8_t> v(n);
    std::mt19937 r(seed);
    for (auto& b : v) b = uint8_t(r());
    return v;
}

// A link that loses about one datagram in four and sends one in eight twice.
class Lossy final : public ntr::CoopTransport {
public:
    explicit Lossy(ntr::CoopTransport* t) : t_(t) {}
    uint32_t coop_node() const override { return t_->coop_node(); }
    void coop_peers(std::vector<uint32_t>& out) override { t_->coop_peers(out); }
    void coop_send(uint32_t to, const uint8_t* d, size_t n) override {
        const uint32_t r = rng_() % 8;
        if (r < 2) return;
        t_->coop_send(to, d, n);
        if (r == 7) t_->coop_send(to, d, n);
    }
    bool coop_recv(uint32_t& from, std::vector<uint8_t>& d) override { return t_->coop_recv(from, d); }

private:
    ntr::CoopTransport* t_;
    std::mt19937 rng_{1234};
};

// Sends count messages of growing size (the last ones bigger than a datagram) and checks they all arrive, in order.
void stream_check(Node& from, Node& to, int count, uint8_t type, const char* what) {
    const size_t before = to.got.size();
    for (int i = 0; i < count; i++) {
        const auto m = pattern(size_t(i) * 97 % 5000 + 1, uint32_t(i));
        CHECK(from.ch->send(to.ch->node(), type, m.data(), m.size()));
    }
    const bool all = pump({&from, &to}, [&] { return to.got.size() >= before + size_t(count); }, 15000ms);
    CHECK(all);
    int bad = 0;
    for (int i = 0; i < count && before + size_t(i) < to.got.size(); i++) {
        const Got& g = to.got[before + size_t(i)];
        if (g.from != from.ch->node() || g.type != type || g.data != pattern(size_t(i) * 97 % 5000 + 1, uint32_t(i))) bad++;
    }
    CHECK(bad == 0);
    std::printf("coop: %s: %zu of %d arrived, %d out of order or wrong\n", what, to.got.size() - before, count, bad);
}

void lan_tests() {
    ntr::LanConfig hc;
    hc.mode = ntr::LanConfig::Mode::Host;
    hc.port = 49379;
    auto host = ntr::make_lan_link(hc);
    ntr::LanConfig jc;
    jc.mode = ntr::LanConfig::Mode::Join;
    jc.host = "127.0.0.1";
    jc.port = 49379;
    auto a = ntr::make_lan_link(jc), b = ntr::make_lan_link(jc);
    CHECK(host && a && b);
    if (!host || !a || !b) return;
    CHECK(host->coop() && a->coop() && b->coop());
    ntr::CoopChannel ch(host->coop()), ca(a->coop()), cb(b->coop());
    Node H(&ch), A{&ca}, B{&cb};
    // Not in the game's multiplayer: nobody to send to.
    H.poll();
    CHECK(!ch.send(0, 1, "x", 1));
    std::this_thread::sleep_for(1200ms);  // (keepalives: the host learns where the others are)
    host->begin();
    a->begin();
    b->begin();
    CHECK(pump({&H, &A, &B}, [&] { return ch.peers().size() == 2 && ca.peers().size() == 2 && cb.peers().size() == 2; }));

    // To everyone: the host's message reaches both, a guest's reaches the host and the other guest (through the host).
    CHECK(ch.send(0, 7, "town", 4));
    CHECK(ca.send(0, 8, "req", 3));
    CHECK(pump({&H, &A, &B}, [&] { return A.got.size() >= 1 && B.got.size() >= 2 && H.got.size() >= 1; }));
    CHECK(A.got.size() == 1 && A.got[0].type == 7 && A.got[0].from == ch.node());
    CHECK(H.got.size() == 1 && H.got[0].type == 8 && H.got[0].data == std::vector<uint8_t>({'r', 'e', 'q'}));
    CHECK(B.got.size() == 2);
    // To one: only it.
    A.got.clear(), B.got.clear(), H.got.clear();
    CHECK(ch.send(cb.node(), 9, "b", 1));
    CHECK(pump({&H, &A, &B}, [&] { return B.got.size() >= 1; }));
    pump({&H, &A, &B}, [] { return false; }, 100ms);
    CHECK(A.got.empty() && B.got.size() == 1 && B.got[0].type == 9);
    // Many, some bigger than a datagram, in order; a 64 KB one.
    stream_check(H, A, 200, 3, "lan host to guest");
    stream_check(A, B, 100, 4, "lan guest to guest");
    const auto big = pattern(ntr::kCoopMaxMessage, 99);
    CHECK(ch.send(ca.node(), 5, big.data(), big.size()));
    CHECK(!ch.send(ca.node(), 5, big.data(), big.size() + 1));  // (too big)
    A.got.clear();
    CHECK(pump({&H, &A}, [&] { return !A.got.empty(); }));
    CHECK(A.got.size() == 1 && A.got[0].data == big);

    // A guest leaves the game's multiplayer: the others stop sending to it.
    b->end();
    CHECK(pump({&H, &A, &B}, [&] { return ch.peers().size() == 1 && ca.peers().size() == 1 && cb.peers().empty(); }));
    CHECK(!ch.send(cb.node(), 1, "x", 1));
    CHECK(!cb.send(0, 1, "x", 1));
}

void lossy_tests() {
    ntr::LanConfig hc;
    hc.mode = ntr::LanConfig::Mode::Host;
    hc.port = 49381;
    auto host = ntr::make_lan_link(hc);
    ntr::LanConfig jc;
    jc.mode = ntr::LanConfig::Mode::Join;
    jc.host = "127.0.0.1";
    jc.port = 49381;
    auto a = ntr::make_lan_link(jc);
    CHECK(host && a);
    if (!host || !a) return;
    Lossy lh(host->coop()), la(a->coop());
    ntr::CoopChannel ch(&lh), ca(&la);
    Node H(&ch), A{&ca};
    std::this_thread::sleep_for(1200ms);
    host->begin();
    a->begin();
    CHECK(pump({&H, &A}, [&] { return ch.peers().size() == 1 && ca.peers().size() == 1; }));
    stream_check(H, A, 150, 1, "lossy link, host to guest");
    stream_check(A, H, 150, 2, "lossy link, guest to host");
}

void local_tests() {
#ifdef _WIN32
    const std::string name = "Local\\MagiciansQuest-Recomp-coop-test-" + std::to_string(GetCurrentProcessId());
#else
    const std::string name = "/MagiciansQuest-Recomp-coop-test-" + std::to_string(::getpid());
#endif
    auto l0 = ntr::LocalCoop::open(0, name.c_str()), l1 = ntr::LocalCoop::open(1, name.c_str());
    CHECK(l0 && l1);
    if (!l0 || !l1) return;
    ntr::CoopChannel c0(l0.get()), c1(l1.get());
    Node N0(&c0), N1{&c1};
    N0.poll();
    CHECK(c0.peers().empty() && !c0.send(0, 1, "x", 1));  // (not in a session)
    l0->begin();
    l1->begin();
    CHECK(pump({&N0, &N1}, [&] { return c0.peers().size() == 1 && c1.peers().size() == 1; }));
    CHECK(c0.node() == 1 && c1.node() == 2);
    stream_check(N0, N1, 300, 1, "same PC, 0 to 1");
    stream_check(N1, N0, 300, 2, "same PC, 1 to 0");
    // More than the ring holds at once (512 KB): the copy that fell behind gets it all the same.
    {
        std::vector<uint8_t> m = pattern(60000, 7);
        for (int i = 0; i < 20; i++) CHECK(c0.send(0, 6, m.data(), m.size()));
        N1.got.clear();
        CHECK(pump({&N0}, [] { return false; }, 50ms) || true);  // (the sender alone: the ring wraps past the reader)
        CHECK(pump({&N0, &N1}, [&] { return N1.got.size() >= 20; }, 20000ms));
        bool same = N1.got.size() == 20;
        for (const Got& g : N1.got) same = same && g.data == m;
        CHECK(same);
        std::printf("coop: same PC, 20 x 60000 bytes past a 512 KB ring: %zu arrived\n", N1.got.size());
    }

    // Copy 1 stops answering (as if it had crashed) and a new one takes its slot before copy 0 notices: copy 0's
    // stream to it starts again by itself, with what the old one never acknowledged.
    for (int i = 0; i < 5; i++) c0.send(0, 3, "old", 3);
    N0.poll();
    l1.release();  // (left as it is: its slot stays marked in the session, as a crashed copy's would until noticed)
    auto l1b = ntr::LocalCoop::open(1, name.c_str());
    CHECK(l1b);
    if (!l1b) return;
    ntr::CoopChannel c1b(l1b.get());
    Node N1b(&c1b);
    l1b->begin();
    CHECK(pump({&N0, &N1b}, [&] { return c1b.peers().size() == 1; }));
    CHECK(c0.send(0, 4, "new", 3));
    CHECK(pump({&N0, &N1b}, [&] {
        for (const Got& g : N1b.got)
            if (g.type == 4) return true;
        return false;
    }));
    int old = 0;
    for (const Got& g : N1b.got) old += g.type == 3;
    CHECK(old == 5 && !N1b.got.empty() && N1b.got.back().type == 4);
    std::printf("coop: same PC, a copy started again in the slot: %zu messages reached it\n", N1b.got.size());
    l0->end();
    CHECK(pump({&N0, &N1b}, [&] { return c1b.peers().empty(); }));
#ifndef _WIN32
    ::shm_unlink(name.c_str());
#endif
}

int main() {
    lan_tests();
    lossy_tests();
    local_tests();
    if (failures) return 1;
    std::printf("coop: all checks passed\n");
    return 0;
}
