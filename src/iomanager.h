#pragma once
#include "scheduler.h"
//#include "timer.h"

namespace PangTao {

// IO协程调度器 epoll实现
class IOManager : public Scheduler {
   public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    // IO事件
    enum Event {
        NONE = 0x0,   //无事件
        READ = 0x1,   //读事件
        WRITE = 0x4,  //写事件
    };

   private:
    // Socket事件上线文类
    struct FdContext {
        typedef Mutex MutexType;
        //事件上下文
        struct EventContext {
            Scheduler* scheduler = nullptr;  //事件执行的调度器
            Coroutine::ptr fiber;            //事件协程
            std::function<void()> cb;        //事件的回调函数
        };
        //获取事件上下文类
        EventContext& getContext(Event event);
        //重置传入事件上下文
        void resetContext(EventContext& ctx);
        //事件触发方法
        void triggerEvent(Event event);
        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联的句柄
        int fd = 0;
        /// 当前的事件
        Event events = NONE;
        /// 事件的锁
        MutexType mutex;
    };

   public:
    //构造函数 threads 线程数量 use_caller 是否将调用线程包含进去name
    //调度器的名称
    IOManager(size_t threads = 1, bool use_caller = true,
              const std::string& name = "");
    //析构函数
    ~IOManager();
    //添加事件到调度器 fd socket句柄 event 事件类型 cb 事件回调函数
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    //删除事件fd socket句柄 event 事件类型
    bool delEvent(int fd, Event event);
    //取消指定事件并触发
    bool cancelEvent(int fd, Event event);
    //取消所有事件并触发
    bool cancelAll(int fd);
    //返回当前IO调度器指针
    static IOManager* GetThis();

   protected:
    //重载通知调度器函数
    void tickle() override;
    //重载停止过程函数
    bool stopping() override;
    //重载空闲协程
    void idle() override;
    //void onTimerInsertedAtFront() override;
    //重置socket句柄上下文的容器大小
    void contextResize(size_t size);
    //判断是否可以停止 timeout 最近要出发的定时器事件间隔
    bool stopping(uint64_t& timeout);

   private:
    int m_epfd = 0;      // epoll 文件句柄
    int m_tickleFds[2];  // 连接调度器和通知事件
    std::atomic<size_t> m_pendingEventCount = {0};  //当前等待执行的事件数量
    RWMutexType m_mutex;                            // IOManager的Mutex
    std::vector<FdContext*> m_fdContexts;  // socket事件上下文的容器
};

}  // namespace PangTao
