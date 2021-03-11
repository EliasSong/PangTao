#pragma once
#include <memory>
#include <mutex>
#include <list>
#include <string>
#include <functional>
#include <vector>
#include "thread.h"
#include "util.h"
#include "coroutine.h"
namespace PangTao
{
    //协程调度器 将协程指定到相应的线程上执行
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        std::vector<pid_t> m_threadIds;//协程调度器中的线程id
        pid_t m_mainThreadId = 0;//当前正在运行的线程id
        Coroutine::ptr m_mainCoroutine = nullptr;
        size_t m_threadCount = 0;
        std::atomic<size_t> m_activeThreadCount = {0};//活跃的线程数
        std::atomic<size_t> m_idleThreadCount = {0};//执行空闲协程的线程数
        bool m_stopping = true;//正在停止标志
        bool m_autoStop = false;//自动停止标志
        Scheduler(const std::string &name,size_t threadNum = 1, bool useCaller = true);
        virtual ~Scheduler();
        const std::string &getName() const { return m_name; }//返回当前协程调度器名字
        static Scheduler *GetThis();//返回当前线程运行的协程调度器
        static Coroutine *GetMainCoroutine();//返回当前协程调度器的调度协程
        void start();//协程调度器开始，初始化线程组
        void stop();//停止协程调度器
        template <class CoroutineOrCallback>
        void schedule(CoroutineOrCallback cc, pid_t id = -1)//-1不指定线程，协程接受任意线程调度
        {
            bool tickleFlag = false;
            {
                Mutex::Lock lock(m_mutex);
                tickleFlag = scheduleWithoutLock(cc, id);
            }
            if (tickleFlag)
            {
                tickle();
            }
        }
        template <class iterator>
        void schedule(iterator begin, iterator end)
        {
            bool tickleFlag = false;
            {
                Mutex::Lock lock(m_mutex);
                while (begin != end)
                {
                    tickleFlag = scheduleWithoutLock(&*begin) || tickleFlag;
                    begin++;
                }
            }
            if (tickleFlag)
            {
                tickle();
            }
        }
        virtual void tickle();//通知协程调度器有任务
        virtual bool stopping();
        virtual void idle();
        void setThis();
        void run();//线程池运行的入口函数

    private:
        struct CoroutineInThread
        {
            Coroutine::ptr coroutine;
            std::function<void()> cb;
            pid_t threadId;
            CoroutineInThread(Coroutine::ptr co, pid_t id) : coroutine(co), threadId(id) {}
            CoroutineInThread(Coroutine::ptr *co, pid_t id)
            {
                threadId = id;
                coroutine.swap(*co);
            }
            CoroutineInThread(std::function<void()> callback, pid_t id) : cb(callback), threadId(id) {}
            CoroutineInThread(std::function<void()> *callback, pid_t id)
            {
                cb.swap(*callback);
                threadId = id;
            }
            CoroutineInThread()
            {
                threadId = -1;
                cb = nullptr;
                coroutine = nullptr;
            }
            void reset(){
                threadId = -1;
                cb = nullptr;
                coroutine = nullptr;

            }
        };

        template <class CoroutineOrCallback>
        bool scheduleWithoutLock(CoroutineOrCallback cc, pid_t id)
        {
            bool tickleFlag = m_coroutines.empty();
            CoroutineInThread ft(cc, id);
            if (ft.cb || ft.coroutine)
            {
                m_coroutines.push_back(ft);
            }
            return tickleFlag;
        }
        Mutex m_mutex;//互斥锁
        std::vector<Thread::ptr> m_threads;//线程池
        std::string m_name;
        std::list<CoroutineInThread> m_coroutines;//协程链表
    };
} // namespace PangTao