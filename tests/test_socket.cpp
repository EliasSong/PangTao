#include "../src/pangtao.h"


void test_socket() {
    //std::vector<PangTao::Address::ptr> addrs;
    //PangTao::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //PangTao::IPAddress::ptr addr;
    //for(auto& i : addrs) {
    //    PangTao_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<PangTao::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}
    PangTao::IPAddress::ptr addr = PangTao::Address::LookupAnyIPAddress("eliassong.cn:8080");
    //addr->setPort(8080);
    if(addr) {
        PANGTAO_LOG_INFO( PANGTAO_ROOT_LOGGER,"get address: " + addr->toString());
    } else {
       PANGTAO_LOG_INFO( PANGTAO_ROOT_LOGGER,"get address failed ");
        return;
    }

    PangTao::Socket::ptr sock = PangTao::Socket::CreateTCP(addr);
   
   // std::cout<<addr->getPort()<<std::endl;
    PANGTAO_LOG_INFO( PANGTAO_ROOT_LOGGER,"addr =  " + addr->toString());
    if(!sock->connect(addr)) {
        PANGTAO_LOG_INFO( PANGTAO_ROOT_LOGGER," connect failed" + addr->toString());
        return;
    } else {
        PANGTAO_LOG_INFO( PANGTAO_ROOT_LOGGER," connected " + addr->toString());
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        return;
    }

    buffs.resize(rt);
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER,std::string(buffs));
}

// void test2() {
//     PangTao::IPAddress::ptr addr = PangTao::Address::LookupAnyIPAddress("www.baidu.com:80");
//     if(addr) {
//         PangTao_LOG_INFO(g_looger) << "get address: " << addr->toString();
//     } else {
//         PangTao_LOG_ERROR(g_looger) << "get address fail";
//         return;
//     }

//     PangTao::Socket::ptr sock = PangTao::Socket::CreateTCP(addr);
//     if(!sock->connect(addr)) {
//         PangTao_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
//         return;
//     } else {
//         PangTao_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
//     }

//     uint64_t ts = PangTao::GetCurrentUS();
//     for(size_t i = 0; i < 10000000000ul; ++i) {
//         if(int err = sock->getError()) {
//             PangTao_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
//             break;
//         }

//         //struct tcp_info tcp_info;
//         //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
//         //    PangTao_LOG_INFO(g_looger) << "err";
//         //    break;
//         //}
//         //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
//         //    PangTao_LOG_INFO(g_looger)
//         //            << " state=" << (int)tcp_info.tcpi_state;
//         //    break;
//         //}
//         static int batch = 10000000;
//         if(i && (i % batch) == 0) {
//             uint64_t ts2 = PangTao::GetCurrentUS();
//             PangTao_LOG_INFO(g_looger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
//             ts = ts2;
//         }
//     }
// }

int main(int argc, char** argv) {
    PangTao::IOManager iom;
    iom.schedule(&test_socket);
    //iom.schedule(&test2);
    return 0;
}
