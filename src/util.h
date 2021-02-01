#pragma once
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <iostream>
#include <string>
#include <vector>
#include "coroutine.h"
#define PANGTAO_ASSERT(x)                                                                            \
    if (!(x))                                                                                        \
    {                                                                                                \
        PangTao::PANGTAO_LOG_ERROR(PangTao::PANGTAO_ROOT_LOGGER(), "ASSERTION: " + std::string(#x)); \
        PangTao::PANGTAO_LOG_ERROR(PangTao::PANGTAO_ROOT_LOGGER(), PangTao::BacktraceToString(100)); \
    }

namespace PangTao
{
    pid_t GetThreadId();
    uint32_t GetCoroutineId();
    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);
    std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "");
} // namespace PangTao


