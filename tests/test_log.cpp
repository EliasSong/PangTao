#include <iostream>
#include <stdio.h>
#include "../src/log.h"
#include "../src/util.h"
using namespace std;

int main()
{
    cout << "helloworld" << endl;
    // auto logger = PangTao::LoggerManager::getInstance()->getRoot();
    // logger->setLevel(PangTao::LogLevel::INFO);
    // logger->addAppender(PangTao::LogAppender::ptr(new PangTao::StdoutLogAppender));
    // logger->addAppender(PangTao::LogAppender::ptr(new PangTao::FileLogAppender("./log.txt")));
    //PANGTAO_LOG_DEBUG(PANGTAO_ROOT_LOGGER,"hello pangtao");
    // PangTao::PANGTAO_LOG_INFO(logger,"hello pangtao");
    // PangTao::PANGTAO_LOG_WARN(logger,"hello pangtao");
    PangTao::Logger::ptr logger(new PangTao::Logger("system"));

    logger->setFormatter("%d%T%m %n");
    logger->addAppender(std::make_shared<PangTao::StdoutLogAppender>());
    
    PangTao::LoggerManager::getInstance()->registerLogger("system",logger);
    auto temp = PangTao::LoggerManager::getInstance()->getLogger("system");
    std::cout<<temp->getName()<<std::endl;
    PANGTAO_LOG_DEBUG(temp,"my register logger test");

    return 0;
}