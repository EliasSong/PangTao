#pragma once
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>

namespace PangTao {
//信号量
class Semaphore {
   public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();
    void wait();
    void notify();

   private:
    Semaphore(const Semaphore &) = delete;
    Semaphore(const Semaphore &&) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
    sem_t m_semaphore;
};

//锁调用类
template <class T>
struct ScopedLockImpl {
   public:
    ScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl() { unlock(); }
    void lock() {
        if (!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

   private:
    T &m_mutex;
    bool m_locked;
};

//读锁实现类
template <class T>
struct ReadScopedLockImpl {
   public:
    ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl() { unlock(); }
    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

   private:
    T &m_mutex;
    bool m_locked;
};

//写锁实现类
template <class T>
struct WriteScopedLockImpl {
   public:
    WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLockImpl() { unlock(); }
    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

   private:
    T &m_mutex;
    bool m_locked;
};

//互斥锁
class Mutex {
   public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
    ~Mutex() { pthread_mutex_destroy(&m_mutex); }
    void lock() { pthread_mutex_lock(&m_mutex); }
    void unlock() { pthread_mutex_unlock(&m_mutex); }

   private:
    pthread_mutex_t m_mutex;
};
//读写锁
class RWMutex {
   public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }
    ~RWMutex() { pthread_rwlock_destroy(&m_lock); }
    void rdlock() { pthread_rwlock_rdlock(&m_lock); }
    void wrlock() { pthread_rwlock_wrlock(&m_lock); }
    void unlock() { pthread_rwlock_unlock(&m_lock); }

   private:
    pthread_rwlock_t m_lock;
};
//自旋锁
class Spinlock {
   public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() { pthread_spin_init(&m_mutex, 0); }
    ~Spinlock() { pthread_spin_destroy(&m_mutex); }
    void lock() { pthread_spin_lock(&m_mutex); }
    void unlock() { pthread_spin_unlock(&m_mutex); }

   private:
    pthread_spinlock_t m_mutex;
};
// CAS
class CASLock {
   public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() { m_mutex.clear(); }
    ~CASLock() {}
    void lock() {
        while (std::atomic_flag_test_and_set_explicit(
            &m_mutex, std::memory_order_acquire)) {
        }
    }
    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }

   private:
    volatile std::atomic_flag m_mutex;
};
//抽象任务类
class Task {
   public:
    typedef std::shared_ptr<Task> ptr;
    std::function<void()> run = nullptr;  //任务方法
    void *data = nullptr;                 //任务数据
    Task(std::function<void()> func, void *arg) {
        run = func;
        data = arg;
    }
    Task(){};
    ~Task() {
        if (data) delete (char *)data;
    }
};
//线程类
class Thread {
   public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(const std::string &name,
           std::function<void()> cb);  //独立线程构造方法 参数：线程名，线程方法
    Thread(
        const std::string &name, std::queue<Task *> *tks, pthread_mutex_t *m,
        pthread_cond_t *
            taskQueueNotEmpty);  //用于线程池的线程构造方法
                                 //参数：线程名，线程池任务队列指针，用于管理线程池任务队列互斥锁指针，线程池任务队列不为空的条件变量
    ~Thread();
    const std::string &getName() const { return m_name; };
    pid_t getId() const { return m_id; }
    void join();               //线程运行
    static Thread *GetThis();  //返回线程局部变量
    static const std::string &GetName();
    static void SetName(const std::string &name);

   private:
    pthread_t m_thread = 0;      //线程描述符
    pid_t m_id = -1;             //线程号，使用syscall返回
    std::function<void()> m_cb;  //线程方法
    std::string m_name = "unknow";
    Thread(const Thread &) = delete;
    Thread(const Thread &&) = delete;
    Thread &operator=(const Thread &) = delete;
    std::queue<Task *> *m_taskQueue = nullptr;  //指向任务队列的指针
    pthread_mutex_t *m_taskQueueMutex =
        nullptr;  //指向管理任务队列的互斥锁的指针
    pthread_cond_t *m_taskQueueNotEmpty =
        nullptr;                  //指向任务队列不为空条件的指针
    static void *run(void *arg);  //线程入口函数
    bool stopFlag = false;        //停止线程
    // Semaphore m_semaphore;
};

class ThreadPool {
   public:
    //内部类，用于线程池单例内存回收
    class GC {
       public:
        ~GC() {
            if (ThreadPool::instance) {
                delete ThreadPool::instance;
            }
        }
    };
    static ThreadPool *getInstance() {
        if (!instance) {
            pthread_mutex_lock(&m_mutex);
            if (!instance) {
                instance = new ThreadPool();
            }
            pthread_mutex_unlock(&m_mutex);
        }
        return instance;
    }
    void init(int num);            //线程池初始化
    void increaseThread(){};  //线程池扩容
    // {
    // if (threads.size() >= maxThreadNum) return;
    // int curIdx = threads.size();
    // for (int i = curIdx + 1; i < curIdx + threadStep && i < maxThreadNum;
    //      i++) {
    //     threads.push_back(new Thread(i + 1, &taskQueue, &taskQueueMutex,
    //                                  &taskQueueNotEmpty));
    // }
    //}
    void decreaseThread(){};  //线程池缩减
                            //{
                            // if (threads.size() <= minThreadNum) return;
                            // int curIdx = threads.size();
    // for (int i = curIdx - 1; i > curIdx - threadStep && i > minThreadNum;
    //      i--) {
    //     threads[i]->finishFlag = true;
    //     threads.pop_back();
    // }
    //}

   private:
    static ThreadPool *instance;     //线程池单例
    static GC m_gc;                  //内部类静态对象
    static pthread_mutex_t m_mutex;  //构造线程池的锁
    pthread_mutex_t taskQueueMutex =
        PTHREAD_MUTEX_INITIALIZER;  //管理任务队列的锁
    pthread_cond_t taskQueueNotFull =
        PTHREAD_COND_INITIALIZER;  //任务队列不满的条件变量
    pthread_cond_t taskQueueNotEmpty =
        PTHREAD_COND_INITIALIZER;   //任务队列不为空的条件变量
    std::queue<Task *> taskQueue;   //任务队列
    std::vector<Thread *> threads;  //线程池
    Thread *manageThread = nullptr;  //线程池中的管理线程，用于线程池扩容，缩减
    int threadNum = -1;            //线程池线程初始数目
    int minThreadNum = 10;         //线程池线程最小数目
    int maxThreadNum = 100;        //线程池线程最大数目
    int threadStep = 5;            //线程池数目变化步长
    int taskQueueSizeLimit = 100;  //任务队列最大长度
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
};

}  // namespace PangTao