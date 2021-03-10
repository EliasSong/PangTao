#include "thread.h"

#include "log.h"
#include "util.h"
namespace PangTao {
static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "unknow";
ThreadPool *ThreadPool::instance = nullptr;
pthread_mutex_t ThreadPool::m_mutex = PTHREAD_MUTEX_INITIALIZER;
ThreadPool::GC ThreadPool::m_gc;

Semaphore::Semaphore(uint32_t count) {
    if (sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem init failed");
    }
}
Semaphore::~Semaphore() { sem_destroy(&m_semaphore); }
void Semaphore::wait() {
    if (sem_wait(&m_semaphore)) {
        throw std::logic_error("sem wait failed");
    }
}
void Semaphore::notify() {
    if (sem_post(&m_semaphore)) {
        throw std::logic_error("sem post failed");
    }
}

Thread *Thread::GetThis() { return t_thread; }
const std::string &Thread::GetName() { return t_thread_name; }

void Thread::SetName(const std::string &name) {
    if (t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(const std::string &name, std::function<void()> cb) {
    m_cb = cb;
    if (!name.empty()) {
        m_name = name;
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER,
                          "pthread_create thread failed. name = " + name);
        throw std::logic_error("pthread_create thread failed.");
    }
    // m_semaphore.wait();
}
Thread::Thread(const std::string &name, std::queue<Task *> *tks,
               pthread_mutex_t *m, pthread_cond_t *taskQueueNotEmpty)
    : m_taskQueue(tks),
      m_taskQueueMutex(m),
      m_taskQueueNotEmpty(taskQueueNotEmpty) {
    if (!name.empty()) {
        m_name = name;
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER,
                          "pthread_create thread failed. name = " + name);
        throw std::logic_error("pthread_create thread failed.");
    }
}
Thread::~Thread() {
    if (m_thread) {
        pthread_detach(m_thread);
    }
}
void Thread::join() {
    if (m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if (rt) {
            PANGTAO_LOG_ERROR(PANGTAO_ROOT_LOGGER,
                              "pthread_join thread failed. name = " + m_name);
            throw std::logic_error("pthread_join thread failed.");
        }
        m_thread = 0;
    }
}
void *Thread::run(void *arg) {
    Thread *th = (Thread *)arg;
    t_thread = th;
    th->m_id = PangTao::GetThreadId();
    t_thread_name = th->m_name;
    if (th->m_cb) {
        pthread_setname_np(pthread_self(), th->m_name.substr(0, 15).c_str());
        std::function<void()> cb;
        cb.swap(th->m_cb);
        // thread->m_semaphore.notify();
        cb();
    } else {
        while (!th->stopFlag) {
            pthread_mutex_lock(th->m_taskQueueMutex);
            while (th->m_taskQueue->empty()) {
                pthread_cond_wait(th->m_taskQueueNotEmpty,
                                  th->m_taskQueueMutex);
            }
            if (!th->m_taskQueue->empty()) {
                Task *t = th->m_taskQueue->front();
                th->m_taskQueue->pop();
                std::cout << "in name : " << th->m_name
                          << " in tid : " << th->m_id << " ";
                t->run();
                pthread_mutex_unlock(th->m_taskQueueMutex);

                delete t;
            } else {
                pthread_mutex_unlock(th->m_taskQueueMutex);
            }
        }
    }
    return 0;
}

ThreadPool::ThreadPool() {
    std::cout << "===================Construct Pool=================="
              << std::endl;
    threads.resize(minThreadNum, nullptr);
}
ThreadPool::~ThreadPool() {
    std::cout << "====================Destory Pool==================="
              << std::endl;
}
void ThreadPool::init(int num) {
    if (num < minThreadNum || num > maxThreadNum) {
        PANGTAO_LOG_ERROR(
            PANGTAO_ROOT_LOGGER,
            "Threadpool init failed because of thread number is illegal!")
        return;
    }
    threadNum = num;
    threads.resize(threadNum, nullptr);
    for (int i = 0; i < threadNum; i++) {
        threads[i] = new Thread("Thread - " + std::to_string(i), &taskQueue,
                                &taskQueueMutex, &taskQueueNotEmpty);
    }
    manageThread = new Thread("ManageThread", [this]() {
        int i = 0;
        while (1) {
            // if (taskQueue.size() > taskQueueSizeLimit) {
            //     increaseThread();
            //     continue;
            // }
            // //cout<<"cur thread num : "<<this->threads.size()<<endl;
            // if (taskQueue.size() < taskQueueSizeLimit) {
            //     decreaseThread();
            //     continue;
            // }
            i++;
            pthread_mutex_lock(&(this->taskQueueMutex));
            std::cout<<"product task "<<i<<std::endl;
            this->taskQueue.push(new Task(
                [i, this]() {
                    std::cout << "task : " << i
                              << " cur queue size : " << this->taskQueue.size()
                              << std::endl;
                },
                nullptr));
            pthread_mutex_unlock(&(this->taskQueueMutex));
            pthread_cond_broadcast(&(this->taskQueueNotEmpty));
            usleep(1000);
        }
    });
}

}  // namespace PangTao
