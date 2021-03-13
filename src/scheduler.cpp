#include <iostream>
#include "log.h"
#include "scheduler.h"
namespace PangTao
{
    static thread_local Scheduler *t_scheduler = nullptr;
    static thread_local Coroutine *t_coroutine = nullptr;//主协程
    Scheduler::Scheduler(const std::string &name, size_t threadNum, bool useCaller)
    {
        PANGTAO_ASSERT(threadNum > 0);
        m_name = name;
        if (useCaller)//如果包含了调用该调度器的线程
        {
            Coroutine::GetThis();
            --threadNum;
            PANGTAO_ASSERT(GetThis() == nullptr);
            t_scheduler = this;
            m_mainCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0, true));
            Thread::SetName(m_name);
            t_coroutine = m_mainCoroutine.get();
            m_mainThreadId = GetThreadId();
            m_threadIds.push_back(m_mainThreadId);
        }
        else
        {
            m_mainThreadId = -1;
        }
        m_threadCount = threadNum;
    }
    Scheduler::~Scheduler()
    {
        PANGTAO_ASSERT(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }
    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }
    Coroutine *Scheduler::GetMainCoroutine()
    {
        return t_coroutine;
    }
    void Scheduler::start()
    {
        Mutex::Lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;
        PANGTAO_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);//线程池初始化
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads[i].reset(new Thread(m_name + "_" + std::to_string(i),std::bind(&Scheduler::run, this)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();
    }
    void Scheduler::stop()
    {
        m_autoStop = true;
        if (m_mainCoroutine && m_threadCount == 0 && (m_mainCoroutine->getState() == Coroutine::State::TERM || m_mainCoroutine->getState() == Coroutine::State::INIT))
        {
            m_stopping = true;
            //如果正在停止调度器过程中 则返回
            if (stopping())
            {
                return;
            }
        }
        //如果useCaller 则保证stop由
        if (m_mainThreadId != -1)
        {
            PANGTAO_ASSERT(GetThis() == this);
        }
        else
        {
            PANGTAO_ASSERT(GetThis() != this);
        }
        m_stopping = true;
        //通知调度器
        for (int i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }
        if (m_mainCoroutine)
        {
            tickle();
        }
        if (m_mainCoroutine)
        {
            if (!stopping())
            {
                m_mainCoroutine->call();
            }
        }
        std::vector<Thread::ptr> thrs;
        {
            Mutex::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        for (auto &i : thrs)
        {
            i->join();
        }
    }
    void Scheduler::setThis()
    {
        t_scheduler = this;
    }
    void Scheduler::run()
    {
        PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "run");
        setThis();
        if (GetThreadId() != m_mainThreadId)
        {
            t_coroutine = Coroutine::GetThis().get();
        }
        Coroutine::ptr idleCoroutine(new Coroutine(std::bind(&Scheduler::idle, this)));
        Coroutine::ptr cbCoroutine;//用于处理匿名函数的协程
        CoroutineInThread ft;
        while (true)
        {
            //std::cout<<"==========================run=========================="<<std::endl;
            ft.reset();
            bool tickleMe = false;
            bool isActive = false;
            {
                Mutex::Lock lock(m_mutex);
                auto i = m_coroutines.begin();
                //遍历协程链表
                while (i != m_coroutines.end())
                {
                    //如果i协程指定了线程运行，并且正在运行的线程不是该协程指定的线程，跳转到下一个协程
                    if (i->threadId != -1 && i->threadId != GetThreadId())
                    {
                        i++;
                        tickleMe = true;//通知协程调度器
                        continue;
                    }
                    PANGTAO_ASSERT(i->coroutine || i->cb);
                    //如果i协程正在运行，跳转到下一个协程
                    if (i->coroutine && i->coroutine->getState() == Coroutine::State::EXEC)
                    {
                        i++;
                        continue;
                    }
                    //该协程ft可以运行 该线程被激活
                    ft = *i;
                    m_coroutines.erase(i);
                    ++m_activeThreadCount;
                    isActive = true;
                    break;
                }
                tickleMe |= (i != m_coroutines.end());
            }
            //有新任务
            if (tickleMe)
            {
                tickle();
            }
            //如果ft协程没有异常或者终止
            if (ft.coroutine && (ft.coroutine->getState() != Coroutine::State::TERM || ft.coroutine->getState() != Coroutine::State::EXCEPT))
            {
                //执行该协程
                ft.coroutine->swapIn();
                --m_activeThreadCount;
                if (ft.coroutine->getState() == Coroutine::State::READY)
                {
                    schedule(ft.coroutine);//协程运行结束后插入协程链表队尾
                }
                else if (ft.coroutine->getState() != Coroutine::State::TERM && ft.coroutine->getState() != Coroutine::State::EXCEPT)
                {
                    ft.coroutine->m_state = Coroutine::State::HOLD;//协程在后台
                }
                ft.reset();
            }
            //如果ft是个函数，用cbCoroutine挂载该协程
            else if (ft.cb)
            {
                if (cbCoroutine)
                {
                    cbCoroutine->reset(ft.cb);
                }
                else
                {
                    cbCoroutine.reset(new Coroutine(ft.cb));
                }
                ft.reset();
               
                cbCoroutine->swapIn();
                --m_activeThreadCount;
                if (cbCoroutine->getState() == Coroutine::State::READY)
                {
                    schedule(cbCoroutine);
                    cbCoroutine.reset();
                }
                else if (cbCoroutine->getState() == Coroutine::State::EXCEPT || cbCoroutine->getState() == Coroutine::State::TERM)
                {
                    cbCoroutine->reset(nullptr);
                }
                else
                {
                    cbCoroutine->m_state = Coroutine::State::HOLD;
                    cbCoroutine.reset();
                }
            }
            //没有协程需要调度，当前线程切换到空闲协程
            else
            {
                if(isActive){
                    --m_activeThreadCount;
                    continue;
                }
                //std::cout<<"===================="<<idleCoroutine->getState()<<"================="<<std::endl;
                if (idleCoroutine->getState() == Coroutine::State::TERM)
                {
                    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "idel term");
                    break;
                }
                ++m_idleThreadCount;
                idleCoroutine->swapIn();
                --m_idleThreadCount;
                if (idleCoroutine->getState() != Coroutine::State::TERM && idleCoroutine->getState() != Coroutine::State::EXCEPT)
                {
                    idleCoroutine->m_state = Coroutine::State::HOLD;
                }
            }
        }
    }
    void Scheduler::tickle()
    {
        PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "tickle");
    }
    bool Scheduler::stopping()
    {
        Mutex::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_coroutines.empty() && m_activeThreadCount == 0;
    }
    void Scheduler::idle()//空闲协程的入口函数，后台等待其他协程结束
    {
        PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "idle");
        while (!stopping())
        {
            Coroutine::YieldToStatusHold();
        }
    }
} // namespace PangTao