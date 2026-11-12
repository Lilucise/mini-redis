#include <iostream>

#include <sys/socket.h>    // socket/bind/listen/accept
#include <netinet/in.h>    // sockaddr_in
#include <arpa/inet.h>     // htons/htonl
#include <unistd.h>        // read/write/close
#include <cstdio>          // perror
#include <cstring>

int main(){
    // 申请通信端点
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1) {perror("socket error"); return 1;}

    // 设置socket
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    std::cout << "socket OK!" << std::endl;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 监听本机所有网卡
    addr.sin_port = htons(9999);                // 端口9999
    int bind_check = bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    if(bind_check == -1) {perror("bind error"); return 1;}
    // hton host->network字节序

    std::cout << "bind OK!" << std::endl;

    // 监听
    int listen_check = listen(listen_fd, 128);     // 128是握手后未accept的排队上限
    if(listen_check == -1) {perror("listen error"); return 1;}

    std::cout << "listen OK!" << std::endl;

    int accept_num = 0;

    // 轮流处理已握手数据
    while(true){
        int conn_fd = accept(listen_fd, nullptr, nullptr);      // 建立连接
        if(conn_fd == -1) {perror("accept error"); return 1;}
        
        accept_num++;
        std::cout << "accepted no." << accept_num << std::endl;
        
        // 字符流
        char buf[1024];
        while(true){
            ssize_t n = read(conn_fd, buf, sizeof(buf)); // read(accepted_fd, rd_to, buf_size);
            if(n == 0) break; // 对端关闭连接
            if(n < 0){
                perror("read error");
                break;
            } // error
            std::cout << "recive " << n << " bytes" << std::endl;
            write(conn_fd, buf, n); // write(accepted_fd, wr_from, wr_len);
        }
        close(conn_fd); // 关闭连接
    }
}