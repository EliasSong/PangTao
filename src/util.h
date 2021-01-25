#include <unistd.h>
#include <sys/syscall.h> 
#include <pthread.h>
#include <iostream>
namespace PangTao{
    pid_t GetThreadId();
    uint32_t GetCoroutineId();
}