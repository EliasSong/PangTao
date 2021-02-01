#include <iostream>
#include "../src/util.h"
#include "../src/log.h"

void test_assert(){
    //PangTao::PANGTAO_LOG_INFO(PangTao::PANGTAO_ROOT_LOGGER(),PangTao::BacktraceToString(10));

}
int main(){
    test_assert();
    PANGTAO_ASSERT(1==0);
    return 0;
}