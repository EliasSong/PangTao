#include "address.h"

namespace PangTao {

template <class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
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

Address::ptr Address::Create(const sockaddr* addr,socklen_t addrLen){

}

IPAddress::ptr IPAddress::Create(const char* address,uint32_t port){
addrinfo hints,*results;
bzero(&hints,sizeof(hints));
hints.ai_flags = AI_NUMERICHOST;
hints.ai_family = AF_UNSPEC;
int error = getaddrinfo(address,nullptr,&hints,&results);
if(error){
    PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER,"ip address create error");
    return nullptr;
}
try{
    IPAddress::ptr 
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
IPv6Address::Create(const char* address, uint32_t port) {
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
const sockaddr* UnknowAddress::getAddr() const { return &m_addr; };
socklen_t UnknowAddress::getAddrLength() const { return sizeof(m_addr); };
std::ostream& UnknowAddress::insert(std::ostream& s) const {
    s << "[Unknow Address family = " << m_addr.sa_family << "]";
    return s;
};
}  // namespace PangTao