#include "address.h"

namespace PangTao {

template <class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}
template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

int Address::getFamily() const { return getAddr()->sa_family; }
std::string Address::toString() {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}
bool Address::operator<(const Address& rhs) const {
    socklen_t minLen = std::min(getAddrLength(), rhs.getAddrLength());
    int ret = memcmp(getAddr(), rhs.getAddr(), minLen);
    if (ret < 0)
        return true;
    else if (ret > 0)
        return false;
    else if (getAddrLength() < rhs.getAddrLength())
        return true;
    return false;
}
bool Address::operator==(const Address& rhs) const {
    return getAddrLength() == rhs.getAddrLength() &&
           memcmp(getAddr(), rhs.getAddr(), getAddrLength()) == 0;
}
bool Address::operator!=(const Address& rhs) const { return !(*this == rhs); }

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrLen) {
     if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknowAddress(*addr));
            break;
    }
    return result;
}
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string node;
    const char* service = NULL;

    //检查 ipv6address serivce
    if (!host.empty() && host[0] == '[') {
        const char* endipv6 =
            (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if (endipv6) {
            // TODO check out of range
            if (*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }

    //检查 node serivce
    if (node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if (service) {
            if (!memchr(service + 1, ':',
                        host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if (node.empty()) {
        node = host;
    }
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if (error) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "Lookup getaddrinfo error");
        return false;
    }

    next = results;
    while (next) {
        result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        // SYLAR_LOG_INFO(g_logger) <<
        // ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}
Address::ptr Address::LookupAny(const std::string& host, int family,
                                int type , int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}
std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host,
                                                       int family,
                                                       int type,
                                                       int protocol) {
    std::vector<Address::ptr> result;
    if (Lookup(result, host, family, type, protocol)) {
        // for(auto& i : result) {
        //    std::cout << i->toString() << std::endl;
        //}
        for (auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if (v) {
                return v;
            }
        }
    }
    return nullptr;
}

bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result,
    int family) {
    struct ifaddrs *next, *results;
    if (getifaddrs(&results) != 0) {
        // SYLAR_LOG_DEBUG(g_logger)
        //     << "Address::GetInterfaceAddresses getifaddrs "
        //        " err="
        //     << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        for (next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch (next->ifa_addr->sa_family) {
                case AF_INET: {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask =
                        ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len = CountBytes(netmask);
                } break;
                case AF_INET6: {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    in6_addr& netmask =
                        ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                    prefix_len = 0;
                    for (int i = 0; i < 16; ++i) {
                        prefix_len += CountBytes(netmask.s6_addr[i]);
                    }
                } break;
                default:
                    break;
            }

            if (addr) {
                result.insert(std::make_pair(next->ifa_name,
                                             std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
}

bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t> >& result,
    const std::string& iface, int family) {
    if (iface.empty() || iface == "*") {
        if (family == AF_INET || family == AF_UNSPEC) {
            result.push_back(
                std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if (family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(
                std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

    if (!GetInterfaceAddresses(results, family)) {
        return false;
    }

    auto its = results.equal_range(iface);
    for (; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}

IPAddress::ptr IPAddress::Create(const char* address, uint32_t port) {
 addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error) {
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

IPv4Address::ptr IPv4Address::Create(const char* address, uint32_t port) {
    IPv4Address::ptr ret(new IPv4Address);
    ret->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &ret->m_addr.sin_addr);
    if (result <= 0) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "ipv4 address create error");
        return nullptr;
    }
    return ret;
}
IPv4Address::IPv4Address(uint32_t address, uint32_t port) {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}
IPv4Address::IPv4Address(sockaddr_in addr) { m_addr = addr; }
const sockaddr* IPv4Address::getAddr() const { return (sockaddr*)&m_addr; }
sockaddr* IPv4Address::getAddr()  { return (sockaddr*)&m_addr; }
socklen_t IPv4Address::getAddrLength() const { return sizeof(m_addr); }
std::ostream& IPv4Address::insert(std::ostream& s) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    s << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
      << ((addr >> 8) & 0xff) << "." << ((addr)&0xff);
    s << " : " << byteswapOnLittleEndian(m_addr.sin_port);
    return s;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefixLen) {
    if (prefixLen > 32) return nullptr;
    sockaddr_in b_addr(m_addr);
    b_addr.sin_addr.s_addr |=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefixLen));

    return IPv4Address::ptr(new IPv4Address(b_addr));
}
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefixLen) {
    if (prefixLen > 32) return nullptr;
    sockaddr_in b_addr(m_addr);
    b_addr.sin_addr.s_addr &=
        byteswapOnLittleEndian(CreateMask<uint32_t>(prefixLen));

    return IPv4Address::ptr(new IPv4Address(b_addr));
}
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefixLen) {
    sockaddr_in subnet;
    bzero(&subnet, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr =
        ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefixLen));
    return IPv4Address::ptr(new IPv4Address(subnet));
}
uint32_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}
void IPv4Address::setPort(uint32_t p) {
    m_addr.sin_port = byteswapOnLittleEndian(p);
}
IPv6Address::IPv6Address() {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}
IPv6Address::ptr IPv6Address::Create(const char* address, uint32_t port) {
    IPv6Address::ptr ret(new IPv6Address);
    ret->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &ret->m_addr.sin6_addr);
    if (result <= 0) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "ipv6 address create error");
        return nullptr;
    }
    return ret;
}
IPv6Address::IPv6Address(const uint8_t address[16], uint32_t port) {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}
IPv6Address::IPv6Address(sockaddr_in6 address) { m_addr = address; }
sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&m_addr;
}
const sockaddr* IPv6Address::getAddr() const { return (sockaddr*)&m_addr; }
socklen_t IPv6Address::getAddrLength() const { return sizeof(m_addr); }
std::ostream& IPv6Address::insert(std::ostream& s) const {
    s << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool usedZeros = false;
    for (int i = 0; i < 8; i++) {
        if (addr[i] == 0 && !usedZeros) {
            continue;
        }
        if (i && addr[i - 1] == 0 && !usedZeros) {
            s << ":";
            usedZeros = true;
        }
        if (i) {
            s << ":";
        }
        s << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if (!usedZeros && addr[7] == 0) {
        s << "::";
    }
    s << "] : " << byteswapOnLittleEndian(m_addr.sin6_port);
    return s;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefixLen) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefixLen / 8] |=
        CreateMask<uint8_t>(prefixLen % 8);
    for (int i = prefixLen / 8 + 1; i < 16; i++) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefixLen) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefixLen / 8] &=
        CreateMask<uint8_t>(prefixLen % 8);
    // for (int i = prefixLen / 8 + 1; i < 16; i++) {
    //     baddr.sin6_addr.s6_addr[i] = 0xff;
    // }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefixLen) {
    sockaddr_in6 subnet;
    bzero(&subnet, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefixLen / 8] =
        ~CreateMask<uint8_t>(prefixLen % 8);
    for (uint32_t i = prefixLen / 8 + 1; i < 16; i++) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}
uint32_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}
void IPv6Address::setPort(uint32_t p) {
    m_addr.sin6_port = byteswapOnLittleEndian(p);
}
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;
UnixAddress::UnixAddress() {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}
void UnixAddress::setAddrLength(uint32_t v) {
    m_length = v;
}
UnixAddress::UnixAddress(const std::string& path) {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;
    if (!path.empty() && path[0] == '\0') {
        --m_length;
    }
    if (m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}
const sockaddr* UnixAddress::getAddr() const { return (sockaddr*)&m_addr; }
sockaddr* UnixAddress::getAddr()  { return (sockaddr*)&m_addr; }
socklen_t UnixAddress::getAddrLength() const { return m_length; }
std::ostream& UnixAddress::insert(std::ostream& s) const {
    if (m_length > offsetof(sockaddr_un, sun_path) &&
        m_addr.sun_path[0] == '\0') {
        return s << "\\0"
                 << std::string(m_addr.sun_path + 1,
                                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return s << m_addr.sun_path;
}
UnknowAddress::UnknowAddress(int family) {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sa_family = family;
}
UnknowAddress::UnknowAddress(const sockaddr& addr) {
    m_addr = addr;
}
const sockaddr* UnknowAddress::getAddr() const { return (sockaddr*)&m_addr; };
sockaddr* UnknowAddress::getAddr()  { return (sockaddr*)&m_addr; };
socklen_t UnknowAddress::getAddrLength() const { return sizeof(m_addr); };
std::ostream& UnknowAddress::insert(std::ostream& s) const {
    s << "[Unknow Address family = " << m_addr.sa_family << "]";
    return s;
};
}  // namespace PangTao