#include <iostream>

#include <sys/socket.h>    // socket/bind/listen/accept
#include <sys/epoll.h>
#include <netinet/in.h>    // sockaddr_in
#include <arpa/inet.h>     // htons/htonl
#include <unistd.h>        // read/write/close
#include <cstdio>          // perror
#include <cstring>         // str
#include <fcntl.h>         // fcntl

// 最大事件数
constexpr int MAX_EVENTS = 1024; 

// 将fd设为非阻塞
int set_nonblocking(int fd){
    // 标志位读取
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) {perror("fcntl F_GETFL error"); return -1;}

    // 仅将O_NONBLOCK标志位置1
    flags |= O_NONBLOCK;

    // 回写标志位
    if(fcntl(fd, F_SETFL, flags) == -1) {perror("fcntl F_SETFL error"); return -1;}
    return 0;
}

int main(){
    // 申请通信端点
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1) {perror("socket error"); return 1;}

    // 设置socket
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if(set_nonblocking(listen_fd) == -1){perror("listen_socket set_nonblocking error"); return 1};

    std::cout << "socket OK!" << std::endl;

    // sockaddr
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

    std::cout << "listen OK! epfd=" << epfd << std::endl;
    std::cout << "server ready on :9999" << std::endl;

    // 创建epoll
    int epfd = epoll_create1(0);
    if(epfd == -1) {perror("epoll error"); return 1;}

    // epoll注册监听fd
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {perror("epoll_ctl ADD listen_fd error"); return -1;}
    // epoll_ctl(epoll, op, fd, event);

    // events收件箱，读取内核里epoll的fd
    epoll_event events[MAX_EVENTS];

    // 轮流处理已握手数据
    while(true){
        // 本轮就绪的事件个数
        int ready_events_num = epoll_wait(epfd, events, MAX_EVENTS, -1); // epoll_wait(epoll_fd, event, max_event, timeout)->ready_events_num // to get ready fd;
        if(ready_events_num == -1) {perror("epoll_wait error"); return 1;}

        // 遍历本轮就绪的事件
        for(int i = 0; i < ready_events_num; i++){
            if(events[i].data.fd == listen_fd){
                int conn_fd = accept(listen_fd, nullptr, nullptr);
                if(conn_fd == -1){ perror("accept error"); continue;}

                // 将建立的连接设非阻塞
                if(set_nonblocking(conn_fd) == -1){
                    perror("accepted connfd set_nonblocking error");
                    close(conn_fd);
                    continue;
                }

                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_fd;

                // 给epoll内核注册已经accept的fd
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
                    perror("epoll_ctl ADD conn_fd error");
                    close(conn_fd);
                    continue;
                }
            }else{
                char buf[1024];
                int fd = events[i].data.fd;
                ssize_t received_bytes = read(fd, buf, sizeof(buf)); // read(accepted_fd, rd_to, buf_size);
                if(received_bytes == 0){
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                }else if(received_bytes < 0){
                    // 暂定
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                }else{
                    std::cout << "received " << n << " bytes" << std::endl;
                    write(fd, buf, n); // write(accepted_fd, wr_from, wr_len);
                }
            }
        }
    }
}