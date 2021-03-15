#include "../src/pangtao.h"


void run() {
    std::cout<<"start look"<<std::endl;
    auto addr = PangTao::Address::LookupAny("0.0.0.0:8033");
    std::cout<<"start addr"<<std::endl;
    //auto addr2 = PangTao::UnixAddress::ptr(new PangTao::UnixAddress("/tmp/unix_addr"));
    std::vector<PangTao::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);
    std::cout<<"start create tcp server"<<std::endl;
    PangTao::TcpServer::ptr tcp_server(new PangTao::TcpServer);
    std::vector<PangTao::Address::ptr> fails;
    std::cout<<"start bind"<<std::endl;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    std::cout<<"start"<<std::endl;
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    std::cout<<"start create tcp server"<<std::endl;
    PangTao::IOManager iom(2);
    std::cout<<"start create tcp server"<<std::endl;
    iom.schedule(run);
    std::cout<<"start create tcp server"<<std::endl;
    return 0;
}
