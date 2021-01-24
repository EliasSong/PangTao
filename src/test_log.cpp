#include "log.h"
#include <iostream>
#include <stdio.h>
using namespace std;

int main(){
    cout<<"helloworld"<<endl;
    PangTao::Logger::ptr logger(new PangTao::Logger);
    logger->addAppender(PangTao::LogAppender::ptr(new PangTao::StdoutLogAppender));
    PangTao::LogEvent::ptr event(new PangTao::LogEvent(__FILE__,__LINE__,0,1,2,time(0)));
    event->getSS()<<"hello log";
    logger->log(PangTao::LogLevel::DEBUG,event);
    return 0;
}