// Co-op between PCs: the host's port opened on the router by itself (docs/COOP_INTERNET.md, "The port opened on the
// router"), so a player hosting over the internet need not forward it by hand. Two ways, tried in this order:
// NAT-PMP (RFC 6886: one UDP question to the router, port 5351; also answered by routers that speak its successor
// PCP) and UPnP IGD (the router found by SSDP, then SOAP over HTTP: AddPortMapping on its WANIPConnection or
// WANPPPConnection service). Our own small client of both (no library). The mapping is renewed before its lease ends
// and removed when the link closes; a game that crashes leaves it until the lease ends (an hour).
//
// Testing (env): NTR_PORTMAP=0 tries nothing; NTR_PORTMAP_GATEWAY=<IPv4> asks that router for NAT-PMP instead of the
// system's default gateway; NTR_PORTMAP_SSDP=<IPv4> sends the SSDP search there instead of the multicast group.
#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace ntr {

struct PortMapStatus {
    enum State : uint8_t { Off = 0, Trying = 1, Mapped = 2, Failed = 3 } state = Off;
    std::string method;          // "UPnP" or "NAT-PMP"
    std::string external_ip;     // the router's public address, as it says (empty: not told)
    uint16_t external_port = 0;  // the port open on the router (normally the same as ours)
    std::string error;           // why not (Failed)
};

class PortMapper {
public:
    PortMapper() = default;
    ~PortMapper() { stop(); }
    PortMapper(const PortMapper&) = delete;
    PortMapper& operator=(const PortMapper&) = delete;

    // Opens UDP `port` (on this PC) on the router, on a thread of its own, and keeps it open.
    void start(uint16_t port);
    // Removes the mapping (waits up to about a second for the router).
    void stop();
    PortMapStatus status();

private:
    void run();
    bool try_natpmp(bool renew);
    bool try_upnp(bool renew);
    void remove();
    bool stopping();
    void set(PortMapStatus::State state, const std::string& method, const std::string& error = {});

    uint16_t port_ = 0;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
    PortMapStatus status_;
    // What was mapped (for renewal and removal).
    std::string gateway_;      // NAT-PMP: the router's address
    std::string control_url_;  // UPnP: the service's control URL
    std::string service_;      // UPnP: its service type
    std::string local_ip_;
    int lease_s_ = 0;
};

}  // namespace ntr
