#pragma once
#include <ucontext.h>

#include <functional>
#include <iostream>
#include <memory>

#include "thread.h"
#include "util.h"

namespace PangTao {
class Scheduler;  //协程调度器 设置为协程友元
class Coroutine : public std::enable_shared_from_this<Coroutine> {
    friend class Scheduler;

   public:
    typedef std::shared_ptr<Coroutine> ptr;
    Coroutine(std::function<void()> cb, size_t stackSize = 0,
              bool useCaller = false);
    Coroutine();
    ~Coroutine();
    void reset(std::function<void()> cb);  //重设协程方法
    void swapIn();                         //运行当前协程
    void swapOut();  //挂起当前协程 并保存当前协程上下文
    void call();     //将调用线程包含进去时运行协程
    void back();     //将调用线程包含进去时挂起协程
    uint64_t getId() const { return m_id; }  //返回协程id
    //协程状态 初始化 阻塞 运行 终止 就绪 异常
    enum State { INIT, HOLD, EXEC, TERM, READY, EXCEPT };
    const State getState() const { return m_state; }
    static std::shared_ptr<Coroutine> GetThis();
    static void SetThis(Coroutine *coroutine);  //设置当前线程中的协程
    static void YieldToStatusReady();  //退出当前协程并设置协程状态为ready
    static void YieldToStatusHold();  //退出当前协程并设置协程状态为阻塞
    static uint64_t GetCoroutineCount();  //返回当前协程数
    static uint64_t GetCoroutineId();     //返回当前运行协程id
    static void Main();                   //协程入口函数
    static void CallerMain();  //协程使用指定协程调用的入口函数

   private:
    uint64_t m_id = 0;           //协程id
    uint32_t m_stackSize = 0;    //协程内存空间大小
    State m_state = INIT;        //协程状态
    ucontext_t m_context;        //协程上下文
    void *m_stack = nullptr;     //指向协程内存空间的指针
    std::function<void()> m_cb;  //协程方法
};
}  // namespace PangTao