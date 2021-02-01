#include <iostream>
#include "../src/log.h"
#include "../src/coroutine.h"

void RunInCoroutine()
{
    PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "coroutine begin");
    PangTao::Coroutine::GetThis()->YieldToStatusHold();
    PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "coroutine end");
    PangTao::Coroutine::GetThis()->YieldToStatusHold();
      PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "coroutine over");
}
int main()
{
    {
        PangTao::Coroutine::GetThis();

        PangTao::Coroutine::ptr co(new PangTao::Coroutine(RunInCoroutine));
        co->swapIn();
        PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "main back");
        co->swapIn();
        PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "main back");
        co->swapIn();
    }
    PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(), "main end");

    return 0;
}