// The host's port opened on the router by itself: NAT-PMP, else UPnP IGD (portmap.hpp).
#include "portmap.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
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
#include <iphlpapi.h>
using pm_socket_t = SOCKET;
static constexpr pm_socket_t kNoPmSocket = INVALID_SOCKET;
static void pm_close(pm_socket_t s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using pm_socket_t = int;
static constexpr pm_socket_t kNoPmSocket = -1;
static void pm_close(pm_socket_t s) { ::close(s); }
#endif

namespace ntr {

void log(const char* fmt, ...);  // (runtime_internal.hpp)

namespace {

using Clock = std::chrono::steady_clock;
constexpr int kLeaseS = 3600;  // asked for; renewed at half
constexpr const char* kDescription = "Magician's Quest co-op";

bool wsa_ready() {
#ifdef _WIN32
    static bool ok = [] {
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }();
    return ok;
#else
    return true;
#endif
}

void set_recv_timeout(pm_socket_t s, int ms) {
#ifdef _WIN32
    DWORD t = DWORD(ms);
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&t), sizeof t);
#else
    timeval t{ms / 1000, (ms % 1000) * 1000};
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof t);
#endif
}

std::string ip_text(const in_addr& a) {
    char t[INET_ADDRSTRLEN] = {};
    ::inet_ntop(AF_INET, &a, t, sizeof t);
    return t;
}

bool parse_ip(const std::string& s, in_addr& out) { return ::inet_pton(AF_INET, s.c_str(), &out) == 1; }

// The system's default gateway (IPv4), the router NAT-PMP asks.
std::string default_gateway() {
    if (const char* e = std::getenv("NTR_PORTMAP_GATEWAY")) return e;
#ifdef _WIN32
    MIB_IPFORWARDROW row{};
    if (GetBestRoute(inet_addr("8.8.8.8"), 0, &row) == NO_ERROR && row.dwForwardNextHop != 0) {
        in_addr a;
        a.s_addr = row.dwForwardNextHop;
        return ip_text(a);
    }
    return {};
#else
    std::ifstream f("/proc/net/route");
    std::string line;
    std::getline(f, line);  // (the header)
    while (std::getline(f, line)) {
        std::istringstream in(line);
        std::string iface, dest, gw;
        if (!(in >> iface >> dest >> gw)) continue;
        if (dest != "00000000") continue;
        in_addr a;
        a.s_addr = uint32_t(std::strtoul(gw.c_str(), nullptr, 16));  // (in the kernel's byte order: network order)
        if (a.s_addr) return ip_text(a);
    }
    return {};
#endif
}

// This PC's address on the way to `to` (no datagram is sent: a UDP socket's connect only picks the route).
std::string local_ip_towards(const std::string& to, uint16_t port) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port ? port : 9);
    if (!parse_ip(to, a.sin_addr)) return {};
    pm_socket_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == kNoPmSocket) return {};
    std::string out;
    if (::connect(s, reinterpret_cast<sockaddr*>(&a), sizeof a) == 0) {
        sockaddr_in me{};
#ifdef _WIN32
        int len = sizeof me;
#else
        socklen_t len = sizeof me;
#endif
        if (::getsockname(s, reinterpret_cast<sockaddr*>(&me), &len) == 0) out = ip_text(me.sin_addr);
    }
    pm_close(s);
    return out;
}

// ---- NAT-PMP (RFC 6886) ----

// One question to the router, asked again after 250, 500, 1000 ms (RFC 6886 asks for more; a router that answers
// does so at once). The answer's bytes, or empty.
std::vector<uint8_t> natpmp_ask(const std::string& gateway, const uint8_t* q, size_t n, uint8_t want_op, int attempts = 3) {
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(5351);
    if (!parse_ip(gateway, to.sin_addr)) return {};
    pm_socket_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == kNoPmSocket) return {};
    std::vector<uint8_t> out;
    int wait = 250;
    for (int attempt = 0; attempt < attempts && out.empty(); attempt++, wait *= 2) {
        ::sendto(s, reinterpret_cast<const char*>(q), int(n), 0, reinterpret_cast<sockaddr*>(&to), sizeof to);
        const auto until = Clock::now() + std::chrono::milliseconds(wait);
        while (Clock::now() < until) {
            set_recv_timeout(s, int(std::chrono::duration_cast<std::chrono::milliseconds>(until - Clock::now()).count()) + 1);
            uint8_t b[64];
            sockaddr_in from{};
#ifdef _WIN32
            int flen = sizeof from;
#else
            socklen_t flen = sizeof from;
#endif
            const int got = int(::recvfrom(s, reinterpret_cast<char*>(b), int(sizeof b), 0, reinterpret_cast<sockaddr*>(&from), &flen));
            if (got <= 0) break;
            if (from.sin_addr.s_addr != to.sin_addr.s_addr || got < 8 || b[0] != 0 || b[1] != want_op) continue;
            out.assign(b, b + got);
            break;
        }
    }
    pm_close(s);
    return out;
}

uint16_t be16(const uint8_t* p) { return uint16_t(p[0] << 8 | p[1]); }
uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }

// ---- UPnP IGD ----

struct Url {
    std::string host;
    uint16_t port = 80;
    std::string path = "/";
};

bool parse_url(const std::string& url, Url& out) {
    const std::string scheme = "http://";
    if (url.compare(0, scheme.size(), scheme) != 0) return false;
    const size_t start = scheme.size();
    const size_t slash = url.find('/', start);
    const std::string hostport = url.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
    out.path = slash == std::string::npos ? "/" : url.substr(slash);
    const size_t colon = hostport.find(':');
    out.host = hostport.substr(0, colon);
    out.port = colon == std::string::npos ? 80 : uint16_t(std::atoi(hostport.c_str() + colon + 1));
    return !out.host.empty();
}

// A relative URL (or a path) against the description's base.
std::string resolve_url(const std::string& base, const std::string& rel) {
    if (rel.compare(0, 7, "http://") == 0) return rel;
    Url b;
    if (!parse_url(base, b)) return {};
    std::string prefix = "http://" + b.host + ":" + std::to_string(b.port);
    if (!rel.empty() && rel[0] == '/') return prefix + rel;
    const size_t last = b.path.rfind('/');
    return prefix + b.path.substr(0, last + 1) + rel;
}

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// The text between <tag> and </tag> after `from` (ignoring a namespace prefix on the tag), or empty.
std::string xml_value(const std::string& xml, const std::string& tag, size_t from = 0, size_t* end = nullptr) {
    for (size_t p = from; (p = xml.find('<', p)) != std::string::npos; p++) {
        size_t q = p + 1;
        if (q < xml.size() && (xml[q] == '/' || xml[q] == '?' || xml[q] == '!')) continue;
        const size_t name_end = xml.find_first_of(" >/", q);
        if (name_end == std::string::npos) return {};
        std::string name = xml.substr(q, name_end - q);
        if (const size_t colon = name.find(':'); colon != std::string::npos) name = name.substr(colon + 1);
        if (name != tag) continue;
        const size_t open_end = xml.find('>', name_end);
        if (open_end == std::string::npos) return {};
        if (xml[open_end - 1] == '/') {
            if (end) *end = open_end + 1;
            return {};
        }
        const size_t close = xml.find("</", open_end);
        if (close == std::string::npos) return {};
        if (end) *end = close;
        std::string v = xml.substr(open_end + 1, close - open_end - 1);
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) v.erase(v.begin());
        while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.pop_back();
        return v;
    }
    return {};
}

// A blocking TCP connect with a time limit.
pm_socket_t tcp_connect(const std::string& host, uint16_t port, int timeout_ms) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    if (!parse_ip(host, a.sin_addr)) return kNoPmSocket;  // (routers give their address as digits)
    pm_socket_t s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == kNoPmSocket) return s;
#ifdef _WIN32
    u_long nb = 1;
    ioctlsocket(s, FIONBIO, &nb);
#else
    const int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
    ::connect(s, reinterpret_cast<sockaddr*>(&a), sizeof a);
    fd_set w;
    FD_ZERO(&w);
    FD_SET(s, &w);
    timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    int err = 0;
#ifdef _WIN32
    int elen = sizeof err;
#else
    socklen_t elen = sizeof err;
#endif
    if (::select(int(s) + 1, nullptr, &w, nullptr, &tv) != 1 ||
        ::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &elen) != 0 || err != 0) {
        pm_close(s);
        return kNoPmSocket;
    }
#ifdef _WIN32
    nb = 0;
    ioctlsocket(s, FIONBIO, &nb);
#else
    fcntl(s, F_SETFL, flags);
#endif
    set_recv_timeout(s, timeout_ms);
    return s;
}

std::string dechunk(const std::string& body) {
    std::string out;
    size_t p = 0;
    for (;;) {
        const size_t eol = body.find("\r\n", p);
        if (eol == std::string::npos) break;
        const size_t n = std::strtoul(body.substr(p, eol - p).c_str(), nullptr, 16);
        if (n == 0) break;
        out += body.substr(eol + 2, n);
        p = eol + 2 + n + 2;
    }
    return out;
}

// An HTTP/1.1 request (Connection: close); the status code and the body.
int http(const std::string& url, const std::string& method, const std::string& extra_headers, const std::string& body,
         std::string& out) {
    Url u;
    if (!parse_url(url, u)) return -1;
    pm_socket_t s = tcp_connect(u.host, u.port, 2000);
    if (s == kNoPmSocket) return -1;
    std::string req = method + " " + u.path + " HTTP/1.1\r\nHost: " + u.host + ":" + std::to_string(u.port) +
                      "\r\nConnection: close\r\n" + extra_headers;
    if (!body.empty() || method == "POST") req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    req += "\r\n" + body;
    for (size_t sent = 0; sent < req.size();) {
        const int n = int(::send(s, req.data() + sent, int(req.size() - sent), 0));
        if (n <= 0) {
            pm_close(s);
            return -1;
        }
        sent += size_t(n);
    }
    std::string resp;
    char buf[4096];
    const auto until = Clock::now() + std::chrono::seconds(3);
    for (;;) {
        const int n = int(::recv(s, buf, int(sizeof buf), 0));
        if (n <= 0 || resp.size() > (1u << 20) || Clock::now() > until) break;
        resp.append(buf, size_t(n));
    }
    pm_close(s);
    const size_t head_end = resp.find("\r\n\r\n");
    if (resp.compare(0, 5, "HTTP/") != 0 || head_end == std::string::npos) return -1;
    const int code = std::atoi(resp.c_str() + resp.find(' ') + 1);
    const std::string head = lower(resp.substr(0, head_end));
    out = resp.substr(head_end + 4);
    if (head.find("transfer-encoding: chunked") != std::string::npos) out = dechunk(out);
    return code;
}

// SSDP: the routers' description URLs (LOCATION) answering an M-SEARCH for an internet gateway, within about 2 s.
std::vector<std::string> ssdp_search() {
    std::vector<std::string> found;
    pm_socket_t s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == kNoPmSocket) return found;
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(1900);
    const char* target = std::getenv("NTR_PORTMAP_SSDP");
    parse_ip(target ? target : "239.255.255.250", to.sin_addr);
    unsigned char ttl = 2;
    ::setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&ttl), sizeof ttl);
    static const char* kTargets[] = {"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
                                     "urn:schemas-upnp-org:device:InternetGatewayDevice:2",
                                     "urn:schemas-upnp-org:service:WANIPConnection:1",
                                     "urn:schemas-upnp-org:service:WANPPPConnection:1"};
    auto until = Clock::now() + std::chrono::milliseconds(2000);
    auto again = Clock::now();
    int round = 0;
    while (Clock::now() < until) {
        if (round < 2 && Clock::now() >= again) {  // (twice: a datagram can be lost)
            again = Clock::now() + std::chrono::milliseconds(600);
            for (const char* st : kTargets) {
                const std::string msg = std::string("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 1\r\nST: ") +
                                        st + "\r\n\r\n";
                ::sendto(s, msg.data(), int(msg.size()), 0, reinterpret_cast<sockaddr*>(&to), sizeof to);
            }
            round++;
        }
        set_recv_timeout(s, 300);
        char b[2048];
        const int n = int(::recv(s, b, int(sizeof b) - 1, 0));
        if (n <= 0) continue;
        const std::string text(b, size_t(n));
        const std::string low = lower(text);
        const size_t loc = low.find("\nlocation:");
        if (loc == std::string::npos) continue;
        size_t v = loc + 10;
        while (v < text.size() && text[v] == ' ') v++;
        const size_t eol = text.find_first_of("\r\n", v);
        const std::string url = text.substr(v, eol == std::string::npos ? std::string::npos : eol - v);
        bool known = false;
        for (const auto& f : found) known = known || f == url;
        if (!known) found.push_back(url);
        if (found.size() >= 4) break;
        until = std::min(until, Clock::now() + std::chrono::milliseconds(400));  // (other routers answer as soon)
    }
    pm_close(s);
    return found;
}

// A SOAP action on the service; the response's body (empty and false on an HTTP error, the UPnP error code in *code).
bool soap(const std::string& control, const std::string& service, const std::string& action, const std::string& args,
          std::string& out, int* upnp_error = nullptr) {
    const std::string body = "<?xml version=\"1.0\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                             "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:" + action +
                             " xmlns:u=\"" + service + "\">" + args + "</u:" + action + "></s:Body></s:Envelope>\r\n";
    const std::string headers = "Content-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \"" + service + "#" + action + "\"\r\n";
    const int code = http(control, "POST", headers, body, out);
    if (upnp_error) *upnp_error = code == 200 ? 0 : std::atoi(xml_value(out, "errorCode").c_str());
    return code == 200;
}

std::string mapping_args(uint16_t external, uint16_t internal, const std::string& client, int lease) {
    return "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + std::to_string(external) +
           "</NewExternalPort><NewProtocol>UDP</NewProtocol><NewInternalPort>" + std::to_string(internal) +
           "</NewInternalPort><NewInternalClient>" + client + "</NewInternalClient><NewEnabled>1</NewEnabled>"
           "<NewPortMappingDescription>" + kDescription + "</NewPortMappingDescription><NewLeaseDuration>" +
           std::to_string(lease) + "</NewLeaseDuration>";
}

}  // namespace

void PortMapper::start(uint16_t port) {
    stop();
    {
        std::lock_guard lock(mutex_);
        port_ = port;
        stop_ = false;
        status_ = {};
        if (const char* e = std::getenv("NTR_PORTMAP"); e && std::strcmp(e, "0") == 0) return;
        status_.state = PortMapStatus::Trying;
    }
    thread_ = std::thread([this] { run(); });
}

void PortMapper::stop() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

PortMapStatus PortMapper::status() {
    std::lock_guard lock(mutex_);
    return status_;
}

bool PortMapper::stopping() {
    std::lock_guard lock(mutex_);
    return stop_;
}

void PortMapper::set(PortMapStatus::State state, const std::string& method, const std::string& error) {
    std::lock_guard lock(mutex_);
    status_.state = state;
    status_.method = method;
    status_.error = error;
}

void PortMapper::run() {
    if (!wsa_ready()) {
        set(PortMapStatus::Failed, {}, "no sockets");
        return;
    }
    // NAT-PMP first (one datagram, an answer at once), then UPnP.
    bool mapped = try_natpmp(false);
    std::string why_natpmp, why_upnp;
    if (!mapped && !stopping()) {
        why_natpmp = status().error;
        mapped = try_upnp(false);
        if (!mapped) why_upnp = status().error;
    }
    if (!mapped) {
        const std::string why = "the router did not open it (" + why_upnp + "; NAT-PMP: " + why_natpmp + ")";
        set(PortMapStatus::Failed, {}, why);
        log("portmap: UDP %u: %s\n", unsigned(port_), why.c_str());
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return stop_; });
        return;
    }
    {
        const PortMapStatus s = status();
        log("portmap: UDP %u open on the router as %s:%u (%s, lease %d s)\n", unsigned(port_), s.external_ip.c_str(),
            unsigned(s.external_port), s.method.c_str(), lease_s_);
    }
    // Renewed at half the lease (a lease of 0, for ever, every half hour all the same: a router restarted forgets it).
    for (;;) {
        const int wait_s = lease_s_ > 0 ? std::max(30, lease_s_ / 2) : 1800;
        {
            std::unique_lock lock(mutex_);
            if (cv_.wait_for(lock, std::chrono::seconds(wait_s), [this] { return stop_; })) break;
        }
        const std::string method = status().method;
        const bool ok = method == "NAT-PMP" ? try_natpmp(true) : try_upnp(true);
        if (!ok) log("portmap: renewing UDP %u failed: %s\n", unsigned(port_), status().error.c_str());
    }
    remove();
}

bool PortMapper::try_natpmp(bool renew) {
    const std::string gw = renew ? gateway_ : default_gateway();
    if (gw.empty()) {
        set(PortMapStatus::Trying, {}, "no default gateway");
        return false;
    }
    // The public address (op 0), then the mapping (op 1, UDP).
    std::string external_ip;
    {
        const uint8_t q[2] = {0, 0};
        const auto a = natpmp_ask(gw, q, sizeof q, 128);
        if (a.size() >= 12 && be16(&a[2]) == 0) {
            in_addr ip;
            std::memcpy(&ip.s_addr, &a[8], 4);
            external_ip = ip_text(ip);
        } else if (a.empty() && !renew) {
            set(PortMapStatus::Trying, {}, "no answer from " + gw);
            return false;
        }
    }
    const uint16_t want = renew && status().external_port ? status().external_port : port_;
    const uint8_t q[12] = {0, 1, 0, 0, uint8_t(port_ >> 8), uint8_t(port_), uint8_t(want >> 8), uint8_t(want),
                           uint8_t(kLeaseS >> 24), uint8_t(kLeaseS >> 16), uint8_t(kLeaseS >> 8), uint8_t(kLeaseS)};
    const auto a = natpmp_ask(gw, q, sizeof q, 129);
    if (a.size() < 16 || be16(&a[2]) != 0) {
        set(PortMapStatus::Trying, {}, a.size() >= 4 ? "refused (result " + std::to_string(be16(&a[2])) + ")" : "no answer from " + gw);
        return false;
    }
    std::lock_guard lock(mutex_);
    gateway_ = gw;
    lease_s_ = int(be32(&a[12]));
    status_.state = PortMapStatus::Mapped;
    status_.method = "NAT-PMP";
    status_.external_port = be16(&a[10]);
    if (!external_ip.empty()) status_.external_ip = external_ip;
    status_.error.clear();
    return true;
}

bool PortMapper::try_upnp(bool renew) {
    if (!renew) {
        const auto locations = ssdp_search();
        if (locations.empty()) {
            set(PortMapStatus::Trying, {}, "UPnP: no router answered (UPnP off on the router?)");
            return false;
        }
        std::string why = "UPnP: the router has no internet connection service";
        for (const std::string& loc : locations) {
            std::string xml;
            if (http(loc, "GET", "", "", xml) != 200) {
                why = "UPnP: the router's description could not be read";
                continue;
            }
            const std::string base = xml_value(xml, "URLBase").empty() ? loc : xml_value(xml, "URLBase");
            // Each <service>: its type and control URL.
            for (size_t p = 0; (p = xml.find("<service>", p)) != std::string::npos; p++) {
                const size_t end = xml.find("</service>", p);
                if (end == std::string::npos) break;
                const std::string svc = xml.substr(p, end - p);
                const std::string type = xml_value(svc, "serviceType");
                if (type.find("WANIPConnection") == std::string::npos && type.find("WANPPPConnection") == std::string::npos)
                    continue;
                const std::string control = resolve_url(base, xml_value(svc, "controlURL"));
                Url cu;
                if (!parse_url(control, cu)) continue;
                const std::string me = local_ip_towards(cu.host, cu.port);
                if (me.empty()) continue;
                // The public address (also tells whether this service is the one connected).
                std::string out;
                std::string ext_ip;
                if (soap(control, type, "GetExternalIPAddress", "", out)) ext_ip = xml_value(out, "NewExternalIPAddress");
                // The mapping: the same port outside, else one of the next ten (another PC holds it), with a lease, else
                // for ever (routers that take no lease: error 725).
                int err = 0;
                for (int i = 0; i < 10; i++) {
                    const uint16_t ext = uint16_t(port_ + i);
                    int lease = kLeaseS;
                    bool ok = soap(control, type, "AddPortMapping", mapping_args(ext, port_, me, lease), out, &err);
                    if (!ok && err == 725) {
                        lease = 0;
                        ok = soap(control, type, "AddPortMapping", mapping_args(ext, port_, me, lease), out, &err);
                    }
                    if (ok) {
                        std::lock_guard lock(mutex_);
                        control_url_ = control;
                        service_ = type;
                        local_ip_ = me;
                        lease_s_ = lease;
                        status_.state = PortMapStatus::Mapped;
                        status_.method = "UPnP";
                        status_.external_ip = ext_ip;
                        status_.external_port = ext;
                        status_.error.clear();
                        return true;
                    }
                    if (err != 718) break;  // (718: the port is taken by another PC's mapping: try the next)
                }
                why = "UPnP: the router refused (error " + std::to_string(err) + ")";
            }
        }
        set(PortMapStatus::Trying, {}, why);
        return false;
    }
    std::string control, service, me;
    uint16_t ext;
    int lease;
    {
        std::lock_guard lock(mutex_);
        control = control_url_, service = service_, me = local_ip_, ext = status_.external_port, lease = lease_s_;
    }
    std::string out;
    int err = 0;
    if (soap(control, service, "AddPortMapping", mapping_args(ext, port_, me, lease), out, &err)) return true;
    std::lock_guard lock(mutex_);
    status_.error = "UPnP: renewal refused (error " + std::to_string(err) + ")";
    return false;
}

void PortMapper::remove() {
    const PortMapStatus s = status();
    if (s.state != PortMapStatus::Mapped) return;
    if (s.method == "NAT-PMP") {
        // A mapping with a lifetime of 0 removes it (the external port must be 0 too: RFC 6886 3.4).
        const uint8_t q[12] = {0, 1, 0, 0, uint8_t(port_ >> 8), uint8_t(port_), 0, 0, 0, 0, 0, 0};
        natpmp_ask(gateway_, q, sizeof q, 129, 2);
    } else {
        std::string out;
        soap(control_url_, service_, "DeletePortMapping",
             "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + std::to_string(s.external_port) +
                 "</NewExternalPort><NewProtocol>UDP</NewProtocol>",
             out);
    }
    log("portmap: UDP %u closed on the router again (%s)\n", unsigned(port_), s.method.c_str());
    std::lock_guard lock(mutex_);
    status_.state = PortMapStatus::Off;
}

}  // namespace ntr
