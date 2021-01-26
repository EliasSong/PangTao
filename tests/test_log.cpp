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
    PangTao::PANGTAO_LOG_DEBUG(PangTao::LoggerManager::getInstance()->getRoot(),"hello pangtao");
    // PangTao::PANGTAO_LOG_INFO(logger,"hello pangtao");
    // PangTao::PANGTAO_LOG_WARN(logger,"hello pangtao");
    return 0;
}