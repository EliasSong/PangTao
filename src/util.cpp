#include "util.h"
namespace PangTao{
    pid_t GetThreadId(){
        return syscall(SYS_gettid);
    }
    uint32_t GetCoroutineId(){
        return 0;
    }
}