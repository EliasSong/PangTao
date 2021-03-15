
#pragma once
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>

#include "address.h"

namespace PangTao {

// Socket类
class Socket : public std::enable_shared_from_this<Socket> {
   public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;
    // Socket类型
    enum Type {
        TCP = SOCK_STREAM,  // TCP
        UDP = SOCK_DGRAM    // UDP
    };
    // Socket协议
    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX,
    };
    //创建TCP Socket 指定地址
    static Socket::ptr CreateTCP(PangTao::Address::ptr address);
    //创建UDP Socket 指定地址
    static Socket::ptr CreateUDP(PangTao::Address::ptr address);
    //创建IPv4的TCP Socket
    static Socket::ptr CreateTCPSocket();
    //创建IPv4的UDP Socket
    static Socket::ptr CreateUDPSocket();
    //创建IPv6的TCP Socket
    static Socket::ptr CreateTCPSocket6();
    //创建IPv6的UDP Socket
    static Socket::ptr CreateUDPSocket6();
    //创建Unix的TCP Socket
    static Socket::ptr CreateUnixTCPSocket();
    //创建Unix的UDP Socket
    static Socket::ptr CreateUnixUDPSocket();
    /**
     * Socket构造函数
     * family 协议
     * type 类型
     * protocol 协议
     */
    Socket(int family, int type, int protocol = 0);
    virtual ~Socket();
    //获取发送超时时间(毫秒)
    int64_t getSendTimeout();
    //设置发送超时时间(毫秒)
    void setSendTimeout(int64_t v);
    //获取接受超时时间(毫秒)
    int64_t getRecvTimeout();
    //设置接受超时时间(毫秒)
    void setRecvTimeout(int64_t v);
    //获取sockopt
    bool getOption(int level, int option, void* result, socklen_t* len);
    //获取sockopt模板
    template <class T>
    bool getOption(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }
    //设置sockopt
    bool setOption(int level, int option, const void* result, socklen_t len);
    //设置sockopt模板
    template <class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }
    /**
     * 接收connect链接
     * 成功返回新连接的socket,失败返回nullptr
     * Socket必须 bind , listen  成功
     */
    virtual Socket::ptr accept();
    /**
     *   绑定地址
     *     addr 地址
     * return 是否绑定成功
     */
    virtual bool bind(const Address::ptr addr);
    /**
     * 连接地址
     *     addr 目标地址
     *     timeout_ms 超时时间(毫秒)
     */
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    virtual bool reconnect(uint64_t timeout_ms = -1);
    /**
     * 监听socket
     * backlog 未完成连接队列的最大长度
     * 返回监听是否成功
     * 必须先 bind 成功
     */
    virtual bool listen(int backlog = SOMAXCONN);

    /**
     * 关闭socket
     */
    virtual bool close();

    /**
     *   发送数据
     *     buffer 待发送数据的内存
     *     length 待发送数据的长度
     *     flags 标志字
     * return   >0 发送成功对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int send(const void* buffer, size_t length, int flags = 0);

    /**
     * 发送数据
     *     buffers 待发送数据的内存(iovec数组)
     *     length 待发送数据的长度(iovec长度)
     *     flags 标志字
     * return   >0 发送成功对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    /**
     * 发送数据
     *     buffer 待发送数据的内存
     *     length 待发送数据的长度
     *     to 发送的目标地址
     *     flags 标志字
     * return   >0 发送成功对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to,
                       int flags = 0);

    /**
     * 发送数据
     *     buffers 待发送数据的内存(iovec数组)
     *     length 待发送数据的长度(iovec长度)
     *     to 发送的目标地址
     *     flags 标志字
     * return   >0 发送成功对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int sendTo(const iovec* buffers, size_t length,
                       const Address::ptr to, int flags = 0);

    /**
     * 接受数据
     * buffer 接收数据的内存
     * length 接收数据的内存大小
     * flags 标志字
     * return   >0 接收到对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int recv(void* buffer, size_t length, int flags = 0);

    /**
     * 接受数据
     * buffers 接收数据的内存(iovec数组)
     * length 接收数据的内存大小(iovec数组长度)
     * flags 标志字
     * return   >0 接收到对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int recv(iovec* buffers, size_t length, int flags = 0);
    /**
     *  接受数据
     *  buffer 接收数据的内存
     *  length 接收数据的内存大小
     *  from 发送端地址
     *  flags 标志字
     * return   >0 接收到对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from,
                         int flags = 0);
    /**
     * 接受数据
     * buffers 接收数据的内存(iovec数组)
     * length 接收数据的内存大小(iovec数组长度)
     * from 发送端地址
     * flags 标志字
     * return   >0 接收到对应大小的数据
     *          =0 socket被关闭
     *          <0 socket出错
     */
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from,
                         int flags = 0);
    //获取远端地址
    Address::ptr getRemoteAddress();
    //获取本地地址
    Address::ptr getLocalAddress();
    //获取协议簇
    int getFamily() const { return m_family; }
    //获取类型
    int getType() const { return m_type; }
    //获取协议
    int getProtocol() const { return m_protocol; }
    //返回是否连接
    bool isConnected() const { return m_isConnected; }
    //是否有效(m_sock != -1)
    bool isValid() const;
    //返回Socket错误
    int getError();
    // 输出socket信息
    virtual std::ostream& dump(std::ostream& os) const;
    virtual std::string toString() const;
    //返回socket句柄
    int getSocket() const { return m_sock; }
    // 取消读
    bool cancelRead();
    //取消写
    bool cancelWrite();
    //取消accept
    bool cancelAccept();
    //取消所有事件
    bool cancelAll();

   protected:
    //初始化socket
    void initSock();
    //创建socket
    void newSock();
    //初始化sock
    virtual bool init(int sock);
    int m_sock;                    // socket句柄
    int m_family;                  // 协议簇
    int m_type;                    // 类型
    int m_protocol;                // 协议
    bool m_isConnected;            // 是否连接
    Address::ptr m_localAddress;   // 本地地址
    Address::ptr m_remoteAddress;  // 远端地址
};

//流式输出socket
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}  // namespace PangTao
