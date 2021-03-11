#pragma once
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "endian.h"
#include "log.h"
namespace PangTao {
//地址基类
class Address {
   public:
    typedef std::shared_ptr<Address> ptr;
    static Address::ptr Create(const sockaddr* addr,socklen_t addrLen);
    virtual ~Address() {}
    int getFamily() const;
    virtual const sockaddr* getAddr() const = 0;
    virtual socklen_t getAddrLength() const = 0;
    virtual std::ostream& insert(std::ostream& s) const;
    std::string toString();
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};
//本地套接字地址类
class UnixAddress : public Address {
   public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);
    const sockaddr* getAddr() const override;
    socklen_t getAddrLength() const override;
    std::ostream& insert(std::ostream& s) const override;

   private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};
// ip地址基类
class IPAddress : public Address {
   public:
    typedef std::shared_ptr<IPAddress> ptr;
    static IPAddress::ptr Create(const char* address, uint32_t port = 0);
    virtual IPAddress::ptr broadcastAddress(uint32_t prefixLen) = 0;
    virtual IPAddress::ptr networkAddress(uint32_t prefixLen) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefixLen) = 0;
    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint32_t p) = 0;
};
// ipv4地址类
class IPv4Address : public IPAddress {
    typedef std::shared_ptr<IPv4Address> ptr;
    static IPv4Address::ptr Create(const char* address, uint32_t port = 0);
    IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
    IPv4Address(sockaddr_in addr);
    const sockaddr* getAddr() const override;
    socklen_t getAddrLength() const override;
    std::ostream& insert(std::ostream& s) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefixLen) override;
    IPAddress::ptr networkAddress(uint32_t prefixLen) override;
    IPAddress::ptr subnetMask(uint32_t prefixLen) override;
    uint32_t getPort() const override;
    void setPort(uint32_t p) override;

   private:
    sockaddr_in m_addr;
};
// ipv6地址类
class IPv6Address : public IPAddress {
    typedef std::shared_ptr<IPv6Address> ptr;
    IPv6Address();
    IPv6Address(sockaddr_in6 address);
    static IPv6Address::ptr Create(const char* address, uint32_t port = 0);
    IPv6Address(const uint8_t address[16], uint32_t port = 0);
    const sockaddr* getAddr() const override;
    socklen_t getAddrLength() const override;
    std::ostream& insert(std::ostream& s) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefixLen) override;
    IPAddress::ptr networkAddress(uint32_t prefixLen) override;
    IPAddress::ptr subnetMask(uint32_t prefixLen) override;
    uint32_t getPort() const override;
    void setPort(uint32_t p) override;

   private:
    sockaddr_in6 m_addr;
};
class UnknowAddress : public Address {
   public:
    typedef std::shared_ptr<UnknowAddress> ptr;
    UnknowAddress(int family);
    const sockaddr* getAddr() const override;
    socklen_t getAddrLength() const override;
    std::ostream& insert(std::ostream& s) const override;

   private:
    sockaddr m_addr;
};
};  // namespace PangTao