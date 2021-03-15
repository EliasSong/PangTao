#pragma once
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "endian.h"
#include "log.h"
namespace PangTao {
//地址基类
class IPAddress;
class Address {
   public:
    typedef std::shared_ptr<Address> ptr;
    //创建基类地址
    static Address::ptr Create(const sockaddr* addr, socklen_t addrLen);
    /**
     * 通过host地址返回对应条件的所有Address
     * result 保存满足条件的Address
     * host 域名,服务器名
     * family 协议
     * type socket类型
     * protocol 协议号
     */
    static bool Lookup(std::vector<Address::ptr>& result,
                       const std::string& host, int family = AF_INET,
                       int type = 0, int protocol = 0);
    /**
     * 通过host地址返回对应条件任意一个Address
     * host 域名,服务器名
     * family 协议
     * type socket类型
     * protocol 协议号
     */
    static Address::ptr LookupAny(const std::string& host, int family = AF_INET,
                                  int type = 0, int protocol = 0);
    /**
     * 通过host地址返回对应条件任意一个Address
     * host 域名,服务器名
     * family 协议
     * type socket类型
     * protocol 协议号
     */
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(
        const std::string& host, int family = AF_INET, int type = 0,
        int protocol = 0);
    /**
     * 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
     *  result 保存本机所有地址
     *  family 协议
     */
    static bool GetInterfaceAddresses(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result,
        int family = AF_INET);
    /**
     * 获取指定网卡的地址和子网掩码位数
     * result 保存指定网卡所有地址
     * iface 网卡名称
     * family 协议族(AF_INT, AF_INT6, AF_UNIX)
     */
    static bool GetInterfaceAddresses(
        std::vector<std::pair<Address::ptr, uint32_t> >& result,
        const std::string& iface, int family = AF_INET);

    virtual ~Address() {}
    //返回协议类型
    int getFamily() const;
    //返回地址指针
    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrLength() const = 0;
    //输出地址
    virtual std::ostream& insert(std::ostream& s) const = 0;
    //转为可读字符串
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
    sockaddr* getAddr() override;
    socklen_t getAddrLength() const override;
    std::ostream& insert(std::ostream& s) const override;
    void setAddrLength(uint32_t v);
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
   public:
    typedef std::shared_ptr<IPv4Address> ptr;
    static IPv4Address::ptr Create(const char* address, uint32_t port = 0);
    IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
    IPv4Address(sockaddr_in addr);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr()  override;
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
   public:
    typedef std::shared_ptr<IPv6Address> ptr;
    IPv6Address();
    IPv6Address(sockaddr_in6 address);
    static IPv6Address::ptr Create(const char* address, uint32_t port = 0);
    IPv6Address(const uint8_t address[16], uint32_t port = 0);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr()  override;
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
//未知地址
class UnknowAddress : public Address {
   public:
    typedef std::shared_ptr<UnknowAddress> ptr;
    UnknowAddress(int family);
    UnknowAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr()  override;
    socklen_t getAddrLength() const override;
    std::ostream& insert(std::ostream& s) const override;
   
   private:
    struct sockaddr m_addr;
};
};  // namespace PangTao