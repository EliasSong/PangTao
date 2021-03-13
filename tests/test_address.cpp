#include "../src/pangtao.h"



void test() {
    std::vector<PangTao::Address::ptr> addrs;

    std::cout<<"begin"<<std::endl;
    bool v = PangTao::Address::Lookup(addrs, "www.baidu.com");

     std::cout<<"end"<<std::endl;
    if(!v) {
         std::cout<<"look fail"<<std::endl;
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        std::cout<<i<<" - "<<addrs[i]->toString()<<std::endl;
        //SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = PangTao::Address::LookupAny("localhost:4080");
    if(addr) {
      
        std::cout<<"addr"<<" - ";
    } else {
       
    }
}



int main(int argc, char** argv) {

    test();
    return 0;
}
