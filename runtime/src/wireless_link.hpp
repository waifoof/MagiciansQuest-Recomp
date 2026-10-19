// The link melonDS's local wireless (Platform::MP_*) goes through: other instances on this PC (local_wireless.hpp,
// shared memory) or other PCs on the network (lan_wireless.hpp, UDP). Which one is chosen once, at the first use,
// from ntr::configure_lan (include/ntr/lan.hpp).
#pragma once

#include <cstdint>
#include <memory>

namespace ntr {

class WirelessLink {
public:
    virtual ~WirelessLink() = default;
    // The console's MAC address is changed by these three bytes (added to its last three), so that every console in
    // a session has its own; false: it keeps the firmware's.
    virtual bool mac_offset(uint8_t out[3]) const = 0;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual int send_packet(uint8_t* data, int len, uint64_t timestamp) = 0;
    virtual int recv_packet(uint8_t* data, uint64_t* timestamp) = 0;
    virtual int send_cmd(uint8_t* data, int len, uint64_t timestamp) = 0;
    virtual int send_reply(uint8_t* data, int len, uint64_t timestamp, uint16_t aid) = 0;
    virtual int send_ack(uint8_t* data, int len, uint64_t timestamp) = 0;
    virtual int recv_host_packet(uint8_t* data, uint64_t* timestamp) = 0;
    virtual uint16_t recv_replies(uint8_t* data, uint64_t timestamp, uint16_t aidmask) = 0;
};

void set_multiplayer_active(bool on);  // (melon_platform.cpp: MP_Begin, MP_End)

// The process's link (nullptr: none).
WirelessLink* wireless_link();

// Testing: a LAN link of its own (nullptr if its socket cannot open).
struct LanConfig;
std::unique_ptr<WirelessLink> make_lan_link(const LanConfig& config);

}  // namespace ntr
