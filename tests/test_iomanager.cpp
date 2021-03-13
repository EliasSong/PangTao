#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "../src/pangtao.h"

int sock = 0;

void test_fiber() {
    PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "test_fiber sock");

    // sleep(3);

    // close(sock);
    // PANGTAO::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "49.234.64.147", &addr.sin_addr.s_addr);

    connect(sock, (const sockaddr*)&addr, sizeof(addr));

    PangTao::IOManager::GetThis()->addEvent(
        sock, PangTao::IOManager::READ,
        []() { PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "read callback"); });
    PangTao::IOManager::GetThis()->addEvent(
        sock, PangTao::IOManager::WRITE, []() {
            PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER, "write callback");
            // close(sock);
            PangTao::IOManager::GetThis()->cancelEvent(
                sock, PangTao::IOManager::READ);
            close(sock);
        });
}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
    PangTao::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

PangTao::Timer::ptr s_timer;
void test_timer() {
    PangTao::IOManager iom(2);
    s_timer = iom.addTimer(
        1000,
        []() {
            static int i = 0;
            PANGTAO_LOG_INFO(PANGTAO_ROOT_LOGGER,
                             "hello timer i = " + std::to_string(i));
            if (++i == 3) {
                s_timer->reset(2000, true);
                // s_timer->cancel();
            }
        },
        true);
}

int main(int argc, char** argv) {
    // test1();
    test_timer();
    return 0;
}
