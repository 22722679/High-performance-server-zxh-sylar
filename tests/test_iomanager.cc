#include "sylar/sylar.h"
#include "sylar/iomanager.h"
#include <sys/types.h>      //socket头文件
#include <sys/socket.h>     //socket头文件
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sock = 0;

void test_fiber(){
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock = " << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;                                 // 协议版本
    addr.sin_port = htons(80);                                 // 端口号,htons转成字节序
    inet_pton(AF_INET, "182.61.200.7", &addr.sin_addr.s_addr); // ip地址

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))){
    }else if(errno == EINPROGRESS){
        SYLAR_LOG_INFO(g_logger) << "add event errno = " << errno << " " << strerror(errno);
        sylar::IOManager::GetThis()->addEvent(sock,sylar::IOManager::READ, []() {
           SYLAR_LOG_INFO(g_logger) << "read callback";
        });
        sylar::IOManager::GetThis()->addEvent(sock,sylar::IOManager::WRITE, []() {
            SYLAR_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
            close(sock);
        });
    } else{
        SYLAR_LOG_INFO(g_logger) << "else" << errno << " " << strerror(errno);
    }
}

void test1() {
    std::cout << "EPOLLIN = " << EPOLLIN
              << "  EPOLLOUT= " << EPOLLOUT << std::endl;
    sylar::IOManager iom(2,false);
    iom.schedule(&test_fiber);


}

int main(int argc,char** argv) {
    test1();
    return 0;
}