#include "scheduler.h"
namespace PangTao
{
    class IOManager : public Scheduler
    {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        enum Event
        {
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x2

        };
        IOManager(const std::string &name, size_t threadNum = 1, bool useCaller = true);
        ~IOManager();
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool deleteEvent(int fd, Event event);
        bool cancelEvent(int fd, Event event);
        bool deleteAllEvent(int fd);
        static IOManager *GetThis();
        virtual void tickle();
        virtual bool stopping();
        virtual void idle();

    private:
        struct FdContext
        {
            struct EventContext
            {
                Scheduler *scheduler = nullptr;
                Coroutine::ptr coroutine;
                std::function<void()> cb;
            };

            int fd;
            Event my_events = NONE;
            EventContext read;
            EventContext write;
            Mutex mutex;
        };
    
    };

} // namespace PangTao