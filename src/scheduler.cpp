// #include <iostream>
// #include "log.h"
// #include "scheduler.h"
// namespace PangTao
// {
//     static thread_local Scheduler *t_scheduler = nullptr;
//     static thread_local Coroutine *t_coroutine = nullptr;
//     Scheduler::Scheduler(const std::string &name, size_t threadNum, bool useCaller)
//     {
//         PANGTAO_ASSERT(threadNum > 0);
//         m_name = name;
//         if (useCaller)
//         {
//             Coroutine::GetThis();
//             --threadNum;
//             PANGTAO_ASSERT(GetThis() == nullptr);
//             t_scheduler = this;
//             m_mainCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0, true));
//             Thread::SetName(m_name);
//             t_coroutine = m_mainCoroutine.get();
//             m_mainThreadId = GetThreadId();
//             m_threadIds.push_back(m_mainThreadId);
//         }
//         else
//         {
//             m_mainThreadId = -1;
//         }
//         m_threadCount = threadNum;
//     }
//     Scheduler::~Scheduler()
//     {
//         PANGTAO_ASSERT(m_stopping);
//         if (GetThis() == this)
//         {
//             t_scheduler = nullptr;
//         }
//     }
//     Scheduler *Scheduler::GetThis()
//     {
//         return t_scheduler;
//     }
//     Coroutine *Scheduler::GetMainCoroutine()
//     {
//         return t_coroutine;
//     }
//     void Scheduler::start()
//     {
//         Mutex::Lock lock(m_mutex);
//         if (!m_stopping)
//         {
//             return;
//         }
//         m_stopping = false;
//         PANGTAO_ASSERT(m_threads.empty());
//         m_threads.resize(m_threadCount);
//         for (size_t i = 0; i < m_threadCount; ++i)
//         {
//             m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
//             m_threadIds.push_back(m_threads[i]->getId());
//         }
//         lock.unlock();
//         // if(m_mainCoroutine){
//         //     m_mainCoroutine->call();
//         //     PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER(), "call out");
//         // }
//     }
//     void Scheduler::stop()
//     {
//         m_autoStop = true;
//         if (m_mainCoroutine && m_threadCount == 0 && (m_mainCoroutine->getState() == Coroutine::State::TERM || m_mainCoroutine->getState() == Coroutine::State::INIT))
//         {
//             m_stopping = true;
//             if (stopping())
//             {
//                 return;
//             }
//         }
//         if (m_mainThreadId != -1)
//         {
//             PANGTAO_ASSERT(GetThis() == this);
//         }
//         else
//         {
//             PANGTAO_ASSERT(GetThis() != this);
//         }
//         m_stopping = true;
//         for (int i = 0; i < m_threadCount; ++i)
//         {
//             tickle();
//         }
//         if (m_mainCoroutine)
//         {
//             tickle();
//         }
//         if (m_mainCoroutine)
//         {
//             if (!stopping())
//             {
//                 m_mainCoroutine->call();
//             }
//         }
//         std::vector<Thread::ptr> thrs;
//         {
//             Mutex::Lock lock(m_mutex);
//             thrs.swap(m_threads);
//         }
//         for (auto &i : thrs)
//         {
//             i->join();
//         }
//     }
//     void Scheduler::setThis()
//     {
//         t_scheduler = this;
//     }
//     void Scheduler::run()
//     {
//         PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "run");
//         setThis();
//         if (GetThreadId() != m_mainThreadId)
//         {
//             t_coroutine = Coroutine::GetThis().get();
//         }
//         Coroutine::ptr idleCoroutine(new Coroutine(std::bind(&Scheduler::idle, this)));
//         Coroutine::ptr cbCoroutine;
//         CoroutineInThread ft;
//         while (true)
//         {
//             ft.reset();
//             bool tickleMe = false;
//             bool isActive = false;
//             {
//                 Mutex::Lock lock(m_mutex);
//                 auto i = m_coroutines.begin();
//                 while (i != m_coroutines.end())
//                 {
//                     if (i->threadId != -1 && i->threadId != GetThreadId())
//                     {
//                         i++;
//                         tickleMe = true;
//                         continue;
//                     }
//                     PANGTAO_ASSERT(i->coroutine || i->cb);
//                     if (i->coroutine && i->coroutine->getState() == Coroutine::State::EXEC)
//                     {
//                         i++;
//                         continue;
//                     }
//                     ft = *i;
//                     m_coroutines.erase(i);
//                     ++m_activeThreadCount;
//                     isActive = true;
//                     break;
//                 }
//                 tickleMe |= i != m_coroutines.end();
//             }
//             if (tickleMe)
//             {
//                 tickle();
//             }
//             if (ft.coroutine && (ft.coroutine->getState() != Coroutine::State::TERM || ft.coroutine->getState() != Coroutine::State::EXCEPT))
//             {
//                 ft.coroutine->swapIn();
//                 --m_activeThreadCount;
//                 if (ft.coroutine->getState() == Coroutine::State::READY)
//                 {
//                     schedule(ft.coroutine);
//                 }
//                 else if (ft.coroutine->getState() != Coroutine::State::TERM && ft.coroutine->getState() != Coroutine::State::EXCEPT)
//                 {
//                     ft.coroutine->m_state = Coroutine::State::HOLD;
//                 }
//                 ft.reset();
//             }
//             else if (ft.cb)
//             {
//                 if (cbCoroutine)
//                 {
//                     cbCoroutine->reset(ft.cb);
//                 }
//                 else
//                 {
//                     cbCoroutine.reset(new Coroutine(ft.cb));
//                 }
//                 ft.reset();
               
//                 cbCoroutine->swapIn();
//                 --m_activeThreadCount;
//                 if (cbCoroutine->getState() == Coroutine::State::READY)
//                 {
//                     schedule(cbCoroutine);
//                     cbCoroutine.reset();
//                 }
//                 else if (cbCoroutine->getState() == Coroutine::State::EXCEPT || cbCoroutine->getState() == Coroutine::State::TERM)
//                 {
//                     cbCoroutine->reset(nullptr);
//                 }
//                 else
//                 {
//                     cbCoroutine->m_state = Coroutine::State::HOLD;
//                     cbCoroutine.reset();
//                 }
//             }
//             else
//             {
//                 if(isActive){
//                     --m_activeThreadCount;
//                     continue;
//                 }
//                 if (idleCoroutine->getState() == Coroutine::State::TERM)
//                 {
//                     PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "idel term");
//                     break;
//                 }
//                 ++m_idleThreadCount;
//                 idleCoroutine->swapIn();
//                 --m_idleThreadCount;
//                 if (idleCoroutine->getState() != Coroutine::State::TERM && idleCoroutine->getState() != Coroutine::State::EXCEPT)
//                 {
//                     idleCoroutine->m_state = Coroutine::State::HOLD;
//                 }
//             }
//         }
//     }
//     void Scheduler::tickle()
//     {
//         PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "tickle");
//     }
//     bool Scheduler::stopping()
//     {
//         Mutex::Lock lock(m_mutex);
//         return m_autoStop && m_stopping && m_coroutines.empty() && m_activeThreadCount == 0;
//     }
//     void Scheduler::idle()
//     {
//         PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "idle");
//         while (!stopping())
//         {
//             Coroutine::YieldToStatusHold();
//         }
//     }
// } // namespace PangTao