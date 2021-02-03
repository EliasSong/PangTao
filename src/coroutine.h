#pragma once
#include <ucontext.h>
#include <memory>
#include <iostream>
#include <functional>
#include "thread.h"
#include "util.h"

namespace PangTao
{
    class Scheduler;
    class Coroutine : public std::enable_shared_from_this<Coroutine>
    {
        friend class Scheduler;

    public:
        typedef std::shared_ptr<Coroutine> ptr;
        Coroutine(std::function<void()> cb, size_t stackSize = 0,bool useCaller = false);
        Coroutine();
        ~Coroutine();
        void reset(std::function<void()> cb);
        void swapIn();
        void swapOut();
        void call();
        void back();
        uint64_t getId() const { return m_id; }

        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };
        const State getState() const { return m_state; }
        static std::shared_ptr<Coroutine> GetThis();
        static void SetThis(Coroutine *coroutine);
        static void YieldToStatusReady();
        static void YieldToStatusHold();
        static uint64_t GetCoroutineCount();
        static uint64_t GetCoroutineId();
        static void Main();
        static void CallerMain();

    private:
        uint64_t m_id = 0;
        uint32_t m_stackSize = 0;
        State m_state = INIT;
        ucontext_t m_context;
        void *m_stack = nullptr;
        std::function<void()> m_cb;
    };
} // namespace PangTao