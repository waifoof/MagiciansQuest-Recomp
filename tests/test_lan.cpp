// The LAN link (runtime/src/lan_wireless.cpp): a host and two joining PCs on this machine's loopback, through the
// calls melonDS's local wireless makes (Platform::MP_*).
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

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

int main() {
    using namespace std::chrono_literals;
    ntr::LanConfig hc;
    hc.mode = ntr::LanConfig::Mode::Host;
    hc.port = 49377;
    auto host = ntr::make_lan_link(hc);
    CHECK(host);
    ntr::LanConfig jc;
    jc.mode = ntr::LanConfig::Mode::Join;
    jc.host = "127.0.0.1";
    jc.port = 49377;
    auto a = ntr::make_lan_link(jc), b = ntr::make_lan_link(jc);
    CHECK(a && b);
    if (!host || !a || !b) return 1;
    uint8_t m1[3], m2[3];
    CHECK(a->mac_offset(m1) && b->mac_offset(m2) && std::memcmp(m1, m2, 3) != 0);
    std::this_thread::sleep_for(1200ms);  // (keepalives: the host learns where the others are)
    host->begin();
    a->begin();
    b->begin();
    std::this_thread::sleep_for(200ms);

    // A command from the host reaches both; their replies come back to it.
    uint8_t cmd[16] = {1, 2, 3, 4};
    CHECK(host->send_cmd(cmd, 16, 1000) == 16);
    uint8_t buf[0x948 * 16] = {};
    uint64_t ts = 0;
    CHECK(a->recv_host_packet(buf, &ts) == 16 && buf[3] == 4 && ts == 1000);
    CHECK(b->recv_host_packet(buf, &ts) == 16 && buf[0] == 1);
    uint8_t ra[8] = {0xA}, rb[8] = {0xB};
    CHECK(a->send_reply(ra, 8, 1001, 1) == 8);
    CHECK(b->send_reply(rb, 8, 1001, 2) == 8);
    std::memset(buf, 0, sizeof buf);
    const uint16_t got = host->recv_replies(buf, 1000, 0x6);
    CHECK(got == 0x6);
    CHECK(buf[0] == 0xA && buf[1024] == 0xB);

    // A frame from one joining PC reaches the other (through the host) and the host.
    uint8_t f[32] = {7};
    CHECK(a->send_packet(f, 32, 2000) == 32);
    std::this_thread::sleep_for(100ms);
    std::memset(buf, 0, 64);
    CHECK(b->recv_packet(buf, &ts) == 32 && buf[0] == 7);
    std::memset(buf, 0, 64);
    CHECK(host->recv_packet(buf, &ts) == 32 && buf[0] == 7);
    CHECK(a->recv_packet(buf, &ts) == 0);  // (not its own)

    // A joining PC hosting the game's session: its command reaches the others, their replies come back to it.
    CHECK(b->send_cmd(cmd, 16, 3000) == 16);
    CHECK(a->recv_host_packet(buf, &ts) == 16 && ts == 3000);
    CHECK(host->recv_host_packet(buf, &ts) == 16 && ts == 3000);
    CHECK(a->send_reply(ra, 8, 3001, 1) == 8);
    CHECK(host->send_reply(rb, 8, 3001, 3) == 8);
    std::memset(buf, 0, sizeof buf);
    CHECK(b->recv_replies(buf, 3000, 0x0A) == 0x0A && buf[0] == 0xA && buf[2 * 1024] == 0xB);

    // One leaves: the others stop waiting for it.
    a->end();
    std::this_thread::sleep_for(100ms);
    const auto t0 = std::chrono::steady_clock::now();
    CHECK(host->send_cmd(cmd, 16, 4000) == 16);
    CHECK(b->recv_host_packet(buf, &ts) == 16);
    CHECK(b->send_reply(rb, 8, 4001, 2) == 8);
    CHECK(host->recv_replies(buf, 4000, 0x6) == 0x4);
    CHECK(std::chrono::steady_clock::now() - t0 < 20ms);  // (returned once everyone in the session replied)

    if (failures) return 1;
    std::printf("lan: all checks passed\n");
    return 0;
}
