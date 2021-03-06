#include "../src/pangtao.h"
#include <iomanip>
void test_scheduler()
{
    static int count = 5;
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "testfunc : "+std::to_string(count));
    sleep(1);
    if (count--)
    {
        PangTao::Scheduler::GetThis()->schedule(&test_scheduler);
    }
}
int main()
{
    std::cout<<std::fixed<<std::setprecision(2)<<3.1415<<std::endl;
    PangTao::Scheduler sc("");
    sc.start();
    sc.schedule(test_scheduler);
    sc.stop();
    return 0;
}
