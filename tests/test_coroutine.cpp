#include <iostream>
#include "../src/log.h"
#include "../src/coroutine.h"

void RunInCoroutine()
{
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "coroutine begin");
    PangTao::Coroutine::GetThis()->YieldToStatusHold();
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "coroutine end");
    PangTao::Coroutine::GetThis()->YieldToStatusHold();
      PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "coroutine over");
}
int main()
{
    {
        PangTao::Coroutine::GetThis();

        PangTao::Coroutine::ptr co(new PangTao::Coroutine(RunInCoroutine));
        co->swapIn();
        PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "main back");
        co->swapIn();
        PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "main back");
        co->swapIn();
    }
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "main end");

    return 0;
}