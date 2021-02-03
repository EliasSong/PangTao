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
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        std::vector<pid_t> m_threadIds;
        pid_t m_mainThreadId = 0;
        Coroutine::ptr m_mainCoroutine = nullptr;
        size_t m_threadCount = 0;
        std::atomic<size_t> m_activeThreadCount = {0};
        std::atomic<size_t> m_idleThreadCount = {0};
        bool m_stopping = true;
        bool m_autoStop = false;
        Scheduler(const std::string &name,size_t threadNum = 1, bool useCaller = true);
        virtual ~Scheduler();
        const std::string &getName() const { return m_name; }
        static Scheduler *GetThis();
        static Coroutine *GetMainCoroutine();
        void start();
        void stop();
        template <class CoroutineOrCallback>
        void schedule(CoroutineOrCallback cc, pid_t id = -1)
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
        virtual void tickle();
        virtual bool stopping();
        virtual void idle();
        void setThis();
        void run();

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
        Mutex m_mutex;
        std::vector<Thread::ptr> m_threads;
        std::string m_name;
        std::list<CoroutineInThread> m_coroutines;
    };
} // namespace PangTao