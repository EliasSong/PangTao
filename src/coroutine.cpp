// #include <string>
// #include <iostream>
// #include <atomic>
// #include "coroutine.h"
// #include "configure.h"
// #include "scheduler.h"
// #include "log.h"
// #include "util.h"

// namespace PangTao
// {
//     static std::atomic<uint64_t> CoroutineId = {0};
//     static std::atomic<uint64_t> CoroutineCount = {0};
//     static thread_local Coroutine *main_coroutine = nullptr; //主协程
//     static thread_local Coroutine::ptr main_thread_coroutine;
//     class MallocStackAllocator
//     {
//     public:
//         static void *Alloc(size_t size)
//         {
//             return malloc(size);
//         }
//         static void Dealloc(void *vp, size_t size)
//         {
//             return free(vp);
//         }
//     };
//     Coroutine::Coroutine(std::function<void()> cb, size_t stackSize, bool useCaller)
//     {
//         m_id = ++CoroutineId;
//         m_cb = cb;
//         m_stackSize = stackSize ? stackSize : 1024 * 1024;
//         ++CoroutineCount;
//         m_stack = MallocStackAllocator::Alloc(m_stackSize);
//         if (getcontext(&m_context))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "Getcontext Error");
//         }
//         m_context.uc_link = nullptr;
//         m_context.uc_stack.ss_sp = m_stack;
//         m_context.uc_stack.ss_size = m_stackSize;
//         if (!useCaller)
//         {
//             makecontext(&m_context, &Coroutine::Main, 0);
//         }
//         else
//         {
//             makecontext(&m_context, &Coroutine::CallerMain, 0);
//         }

//         PANGTAO_LOG_DEBUG(PANGTAO_ROOT_LOGGER, "construct coroutine id: " + std::to_string(m_id));
//     }
//     Coroutine::Coroutine()
//     {
//         m_state = EXEC;
//         SetThis(this);
//         if (getcontext(&m_context))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "getcontext error");
//         }
//         ++CoroutineCount;
//         PANGTAO_LOG_DEBUG(PANGTAO_ROOT_LOGGER, "construct coroutine id: " + std::to_string(m_id));
//     }
//     Coroutine::~Coroutine()
//     {

//         --CoroutineCount;
//         if (m_stack)
//         {
//             PANGTAO_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
//             MallocStackAllocator::Dealloc(m_stack, m_stackSize);
//         }
//         else
//         {
//             PANGTAO_ASSERT(!m_cb);
//             PANGTAO_ASSERT(m_state == EXEC);
//             if (main_coroutine == this)
//             {
//                 SetThis(nullptr);
//             }
//         }
//         PANGTAO_LOG_DEBUG(PANGTAO_ROOT_LOGGER, "delete coroutine id: " + std::to_string(m_id));
//     }

//     void Coroutine::reset(std::function<void()> cb)
//     {
//         PANGTAO_ASSERT(m_stack);
//         PANGTAO_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
//         m_cb = cb;
//         if (getcontext(&m_context))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "getcontext error");
//         }
//         m_context.uc_link = nullptr;
//         m_context.uc_stack.ss_sp = m_stack;
//         m_context.uc_stack.ss_size = m_stackSize;
//         makecontext(&m_context, &Coroutine::Main, 0);
//         m_state = INIT;
//     }
//     void Coroutine::swapIn()
//     {
//         SetThis(this);
//         PANGTAO_ASSERT(m_state != EXEC);
//         m_state == EXEC;
//         if (swapcontext(&(Scheduler::GetMainCoroutine()->m_context), &m_context))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "swapcontext error");
//         }
//     }
//     void Coroutine::swapOut()
//     {
//         SetThis(Scheduler::GetMainCoroutine());
//         if (swapcontext(&m_context, &(Scheduler::GetMainCoroutine()->m_context)))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "swapcontext error");
//         }
//     }
//     void Coroutine::call()
//     {
//         SetThis(this);
//         m_state = EXEC;
//         if (swapcontext(&(main_thread_coroutine->m_context), &m_context))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "swapcontext error");
//         }
//     }
//     void Coroutine::back()
//     {

//         SetThis(main_thread_coroutine.get());
//         if (swapcontext(&m_context, &(main_thread_coroutine->m_context)))
//         {
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "swapcontext error");
//         }
//     }
//     uint64_t Coroutine::GetCoroutineId()
//     {
//         if (main_coroutine)
//         {
//             return main_coroutine->getId();
//         }
//         return 0;
//     }
//     Coroutine::ptr Coroutine::GetThis()
//     {

//         if (main_coroutine)
//         {
//             return main_coroutine->shared_from_this();
//         }
//         else
//         {
//             Coroutine::ptr temp_coroutine(new Coroutine);
//             PANGTAO_ASSERT(temp_coroutine.get() == main_coroutine);
//             main_thread_coroutine = temp_coroutine;
//             return main_coroutine->shared_from_this();
//         }
//     }

//     void Coroutine::SetThis(Coroutine *coroutine)
//     {
//         main_coroutine = coroutine;
//     }

//     void Coroutine::YieldToStatusReady()
//     {
//         Coroutine::ptr currentCoroutine = GetThis();
//         currentCoroutine->m_state = READY;
//         currentCoroutine->swapOut();
//     }
//     void Coroutine::YieldToStatusHold()
//     {
//         Coroutine::ptr currentCoroutine = GetThis();
//         currentCoroutine->m_state = HOLD;
//         currentCoroutine->swapOut();
//     }
//     uint64_t Coroutine::GetCoroutineCount()
//     {
//         return CoroutineCount;
//     }
//     void Coroutine::Main()
//     {
//         Coroutine::ptr currentCoroutine = GetThis();
//         PANGTAO_ASSERT(currentCoroutine);
//         try
//         {
//             currentCoroutine->m_cb();
//             currentCoroutine->m_cb = nullptr;
//             currentCoroutine->m_state = TERM;
//         }
//         catch (std::exception &e)
//         {
//             currentCoroutine->m_state = EXCEPT;
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "Coroutine main exception: " + std::string(e.what()));
//         }
//         catch (...)
//         {
//             currentCoroutine->m_state = EXCEPT;
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "Coroutine main exception");
//         }
//         auto raw_ptr = currentCoroutine.get();
//         currentCoroutine.reset();
//         raw_ptr->swapOut();
//         PANGTAO_ASSERT(false);
//         //std::cout<<"never"<<std::endl;
//     }
//     void Coroutine::CallerMain()
//     {
//         Coroutine::ptr currentCoroutine = GetThis();
//         PANGTAO_ASSERT(currentCoroutine);
//         try
//         {
//             currentCoroutine->m_cb();
//             currentCoroutine->m_cb = nullptr;
//             currentCoroutine->m_state = TERM;
//         }
//         catch (std::exception &e)
//         {
//             currentCoroutine->m_state = EXCEPT;
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "Coroutine main exception: " + std::string(e.what()));
//         }
//         catch (...)
//         {
//             currentCoroutine->m_state = EXCEPT;
//             PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "Coroutine main exception");
//         }
//         auto raw_ptr = currentCoroutine.get();
//         currentCoroutine.reset();
//         raw_ptr->back();
//         PANGTAO_ASSERT(false);
//     }

// } // namespace PangTao