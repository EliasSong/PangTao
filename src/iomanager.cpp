#include "iomanager.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"
#include "pangtao.h"

namespace PangTao {

// static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

enum EpollCtlOp {};

static std::ostream& operator<<(std::ostream& os, const EpollCtlOp& op) {
    switch ((int)op) {
#define XX(ctl) \
    case ctl:   \
        return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<<(std::ostream& os, EPOLL_EVENTS events) {
    if (!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E)          \
    if (events & E) {  \
        if (!first) {  \
            os << "|"; \
        }              \
        os << #E;      \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(
    IOManager::Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            PANGTAO_ASSERT(false);
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    PANGTAO_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(name, threads, use_caller) {
    m_epfd = epoll_create(5000);
    PANGTAO_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    PANGTAO_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    //设置为非阻塞
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    PANGTAO_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    PANGTAO_ASSERT(!rt);

    contextResize(32);

    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    //如果添加的文件描述符已经存在与上下文容器，则取出对应文件描述符的上下文
    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }
    //不然就扩容上下文容器， 取出对应文件描述符的上下文
    else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //如果该文件描述符对应的事件与传入事件相同，说明重复
    if (fd_ctx->events & event) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "addevent error");
        PANGTAO_ASSERT(!(fd_ctx->events & event));
    }
    //添加事件 修改或者添加事件到上下文容器
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "epollctl error");
        return -1;
    }
    //等待的事件数++
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    PANGTAO_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
    //设置事件对应的事件调度器
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Coroutine::GetThis();
        PANGTAO_ASSERT(event_ctx.fiber->getState() == Coroutine::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    //如果传入的文件描述符不在上下文容器中
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "epolldel error");
        // SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
        //     << (EpollCtlOp)op << ", " << fd << ", " <<
        //     (EPOLL_EVENTS)epevent.events << "):"
        //     << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    //重设上下文容器
    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}
//取消指定事件并触发
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "epollctl error");
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}
//取消所有事件并触发
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER, "epollctl error");
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    PANGTAO_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    //向管道中写数据，使得epoll_wait返回
    int rt = write(m_tickleFds[1], "T", 1);
    PANGTAO_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    // timeout = getNextTimer();
    // return timeout == ~0ull && m_pendingEventCount == 0 &&
    //        Scheduler::stopping();
           return m_pendingEventCount == 0 &&
           Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}
//空闲协程入口，用于IO调度器的核心逻辑跟epoll_wait
void IOManager::idle() {
    //设定监听文件描述符数量
    const uint64_t MAX_EVNETS = 256;
    epoll_event* events = new epoll_event[MAX_EVNETS]();
    std::shared_ptr<epoll_event> shared_events(
        events, [](epoll_event* ptr) { delete[] ptr; });

    while (true) {
        uint64_t next_timeout = 0;
        if (stopping(next_timeout)) {
            break;
        }
        int rt = 0;
        do {
            //最大等待时间
            static const int MAX_TIMEOUT = 3000;
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT
                                                               : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            if (rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while (true);

        // std::vector<std::function<void()>> cbs;
        // listExpiredCb(cbs);
        // if (!cbs.empty()) {
        //     schedule(cbs.begin(), cbs.end());
        //     cbs.clear();
        // }

        //遍历激活的文件描述符个数
        for (int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            //如果是通知管道描述符
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            //激活的文件描述符从epoll监听书上摘下或者修改
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                continue;
            }
            //激活读写事件
            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        //协程切换
        Coroutine::ptr cur = Coroutine::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();
    }
}

//void IOManager::onTimerInsertedAtFront() { tickle(); }

}  // namespace PangTao
