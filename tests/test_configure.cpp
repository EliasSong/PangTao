#include "../src/configure.h"
#include "../src/log.h"
#include <iostream>
#include <unordered_map>

int main()
{

    PangTao::ConfigureVar<std::string>::ptr formatter_configure =
        PangTao::Configure::Lookup((std::string) "%d%T%t[%F]%T[%p] %f [%c]:%l%T%m %n",
                                   "LOGGER_ROOT_FORMATTER",
                                   "fomatter");
    PANGTAO_ROOT_LOGGER->setFormatter(formatter_configure->getVal());
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, formatter_configure->getName() + " " + formatter_configure->toString() + " " + formatter_configure->getDesc());

    formatter_configure->addListener(10, [&](const std::string &old_val, const std::string &new_val) {
        PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, old_val + " -> " + new_val);
        PANGTAO_ROOT_LOGGER->setFormatter(new_val);
    });
    formatter_configure->setVal("%d%T%m %n");
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, formatter_configure->getName() + " " + formatter_configure->toString() + " " + formatter_configure->getDesc());
    return 0;
}