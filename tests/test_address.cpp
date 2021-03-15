#include "../src/pangtao.h"



void test() {
    std::vector<PangTao::Address::ptr> addrs;
    std::cout<<"test Lookup"<<std::endl;
    bool v = PangTao::Address::Lookup(addrs, "www.bilibili.com");
    if(!v) {
         std::cout<<"look fail"<<std::endl;
        return;
    }
    std::cout<<"lookup result : "<<std::endl;
    for(size_t i = 0; i < addrs.size(); ++i) {
        std::cout<<i<<" - "<<addrs[i]->toString()<<std::endl;
    }
    std::cout<<"test Lookup"<<std::endl;
    auto addr = PangTao::Address::LookupAny("www.bilibili.com");
    if(addr) {
        std::cout<<"addr"<<" - "<<addr->toString()<<std::endl;
    } 
}



int main(int argc, char** argv) {
    test();
    return 0;
}
