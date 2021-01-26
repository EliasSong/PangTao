#include "../src/configure.h"
#include "../src/log.h"
#include <iostream>
#include <unordered_map>
int main()
{
    int new_val = 10;
    int old_val = 5;

    PangTao::ConfigureVar<int>::ptr g_int_value_configure =
        PangTao::Configure::Lookup((int)old_val, "SYSTEM_PORT", "system port");
    // PangTao::ConfigureVar<int>::ptr g_int_value_configure =
    //     PangTao::Configure::Lookup((int)8080, "SYSTEM_PORT", "system port");

    // PangTao::ConfigureVar<double>::ptr g_double_value_configure =
    //     PangTao::Configure::Lookup((double)0.001, "SYSTEM_DOUBLE", "system DOUBLE");
    PangTao::PANGTAO_LOG_INFO(PangTao::LoggerManager::getInstance()->getRoot(), g_int_value_configure->getName() + " " + g_int_value_configure->toString() + " " + g_int_value_configure->getDesc());
    g_int_value_configure->addListener(10, [](const int &old_val, const int &new_val) {
        PangTao::PANGTAO_LOG_INFO(PangTao::LoggerManager::getInstance()->getRoot(), std::to_string(old_val) + " -> " + std::to_string(new_val));
    });
    g_int_value_configure->setVal(new_val);
    PangTao::PANGTAO_LOG_INFO(PangTao::LoggerManager::getInstance()->getRoot(), g_int_value_configure->getName() + " " + g_int_value_configure->toString() + " " + g_int_value_configure->getDesc());
    return 0;
}