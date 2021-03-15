
#pragma once

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include "thread.h"
namespace PangTao {

/**
 * 文件描述符上下文类
 * 管理文件句柄类型(是否socket)
 * 是否阻塞,是否关闭,读/写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
   public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();
    //是否初始化完成
    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }
    //设置用户主动设置非阻塞
    void setUserNonblock(bool v) { m_userNonblock = v; }
    //获取是否用户主动设置的非阻塞
    bool getUserNonblock() const { return m_userNonblock; }
    //设置系统非阻塞
    void setSysNonblock(bool v) { m_sysNonblock = v; }
    //获取系统非阻塞
    bool getSysNonblock() const { return m_sysNonblock; }
    /**
     * 设置超时时间
     * type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * v 时间毫秒
     */
    void setTimeout(int type, uint64_t v);

    /**
     * 获取超时时间
     * type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * return 超时时间毫秒
     */
    uint64_t getTimeout(int type);

   private:
    bool init();

   private:
    bool m_isInit : 1;        // 是否初始化
    bool m_isSocket : 1;      // 是否socket
    bool m_sysNonblock : 1;   // 是否hook非阻塞
    bool m_userNonblock : 1;  // 是否用户主动设置非阻塞
    bool m_isClosed : 1;      // 是否关闭
    int m_fd;                 // 文件句柄
    uint64_t m_recvTimeout;   // 读超时时间毫秒
    uint64_t m_sendTimeout;   // 写超时时间毫秒
};
/**
 * @brief 文件句柄管理类
 */
class FdManager {
   public:
    typedef RWMutex RWMutexType;
    static FdManager* getInstance() {
        if (instance == nullptr) {
            instance = new FdManager();
        }
        return instance;
    }
    /**
     * 获取/创建文件句柄类FdCtx
     * fd 文件句柄
     * auto_create 是否自动创建
     * 返回对应文件句柄类FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);
    /**
     * 删除文件句柄类
     * fd 文件句柄
     */
    void del(int fd);

   private:
    FdManager();
    RWMutexType m_mutex;              // 读写锁
    std::vector<FdCtx::ptr> m_datas;  // 文件句柄集合
    static FdManager* instance;
};
}  // namespace PangTao