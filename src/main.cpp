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

    if(listen_fd == -1) {perror("socket"); return 1;}

    // 设置socket
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 监听本机所有网卡
    addr.sin_port = htons(9999);                // 端口9999
    int bind_check = bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    if(bind_check == -1) {perror("bind"); return 1;}
    // htonl()小端->大端 htons()大端->小端 网络字节序是大端，x86是小端

    // 监听
    int listen_check = listen(listen_fd, 128);     // 128是握手后未accept的排队上限
    if(listen_check == -1) {perror("listen"); return 1;}

    // 轮流处理已握手数据
    while(true){
        int conn_fd = accept(listen_fd, nullptr, nullptr);      // 建立连接
        // 字符流
        char buf[1024];
        while(true){
            ssize_t n = read(conn_fd, buf, sizeof(buf)); // read(accepted_fd, rd_to, buf_size);
            if(n == 0) break; // 对端关闭连接
            if(n < 0){
                perror("read");
                break;
            } // error
            write(conn_fd, buf, n); // write(accepted_fd, wr_from, wr_len);
        }
        close(conn_fd); // 关闭连接
    }
}