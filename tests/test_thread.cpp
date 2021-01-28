#include "../src/thread.h"
#include "../src/util.h"
#include "../src/log.h"
#include <iostream>
#include <vector>

int count = 0;

PangTao::Mutex s_mutex;

void fun1()
{
    PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), " name: " + PangTao::Thread::GetName() + " this.name: " + PangTao::Thread::GetThis()->getName() + " id: " + std::to_string(PangTao::GetThreadId()) + " this.id: " + std::to_string(PangTao::Thread::GetThis()->getId()));
    // for(int i = 0; i < 1000000;++i){
    //     PangTao::Mutex::Lock lock(s_mutex);
    //     ++count;
    // }
}
void fun2()
{
    PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), " name: " + PangTao::Thread::GetName() + " this.name: " + PangTao::Thread::GetThis()->getName() + " id: " + std::to_string(PangTao::GetThreadId()) + " this.id: " + std::to_string(PangTao::Thread::GetThis()->getId()));
    // for(int i = 0; i < 1000000;++i){
    //     PangTao::Mutex::Lock lock(s_mutex);
    //     ++count;
    // }
}
int main()
{
    std::vector<PangTao::Thread::ptr> thrs;
    std::cout << "start testing......" << std::endl;

    for (int i = 0; i < 200; ++i)
    {
        PangTao::Thread::ptr thr(new PangTao::Thread(&fun1, "name_" + std::to_string(i)));
        PangTao::Thread::ptr thr2(new PangTao::Thread(&fun1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }
    for (auto p : thrs)
    {
        p->join();
    }
    std::cout << count << std::endl;
    return 0;
}