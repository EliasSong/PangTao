#include "../src/pangtao.h"
#include <iostream>
#include <vector>

int count = 0;

PangTao::Mutex s_mutex;

void fun1()
{
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, " name: " + PangTao::Thread::GetName() + " this.name: " + PangTao::Thread::GetThis()->getName() + " id: " + std::to_string(PangTao::GetThreadId()) + " this.id: " + std::to_string(PangTao::Thread::GetThis()->getId()));
    // for(int i = 0; i < 1000000;++i){
    //     PangTao::Mutex::Lock lock(s_mutex);
    //     ++count;
    // }
}
void fun2()
{
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, " name: " + PangTao::Thread::GetName() + " this.name: " + PangTao::Thread::GetThis()->getName() + " id: " + std::to_string(PangTao::GetThreadId()) + " this.id: " + std::to_string(PangTao::Thread::GetThis()->getId()));
    // for(int i = 0; i < 1000000;++i){
    //     PangTao::Mutex::Lock lock(s_mutex);
    //     ++count;
    // }
}
int main()
{
    // std::vector<PangTao::Thread::ptr> thrs;
    // std::cout << "start testing......" << std::endl;
    // PangTao::Thread::ptr thr(new PangTao::Thread("name_" + std::to_string(0),nullptr,nullptr,nullptr));
    
    // thr->join();
    auto a = PangTao::ThreadPool::getInstance();
    a->init(20);
    // for (int i = 0; i < 200; ++i)
    // {
    //     PangTao::Thread::ptr thr(new PangTao::Thread(&fun1, "name_" + std::to_string(i)));
    //     PangTao::Thread::ptr thr2(new PangTao::Thread(&fun1, "name_" + std::to_string(i)));
    //     thrs.push_back(thr);
    //     thrs.push_back(thr2);
    // }
    // for (auto p : thrs)
    // {
    //     p->join();
    // }
    // std::cout << count << std::endl;
    pthread_exit(nullptr);
    return 0;
}