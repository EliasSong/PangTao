#include "../src/scheduler.h"
#include "../src/log.h"

void test_scheduler()
{
    static int count = 5;
    PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "testfunc : "+std::to_string(count));
    sleep(1);
    if (count--)
    {
        PangTao::Scheduler::GetThis()->schedule(&test_scheduler);
    }
}
int main()
{
    PangTao::Scheduler sc("");
    sc.start();
    sc.schedule(test_scheduler);
    sc.stop();
    return 0;
}
