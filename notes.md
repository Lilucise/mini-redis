## 2026-11-12 · CMake 骨架

- 为什么用 CMake：多文件项目的构建自动化，手敲 g++ 命令不可维护
- 关键三行：CXX_STANDARD 17 / REQUIRED ON / add_executable
- 踩的坑：cmake -B -build 打错，-B 后面跟的是目录名参数不是选项
- 为什么 build/ 要 gitignore：它随时可以删掉重新生成，不属于源码
- 下一步：epoll echo server，阻塞版 → epoll LT → ET

## 2026-11-12 · Echo Server 阻塞版

- read 返回值三种情况：>0 字节 / =0 对端关闭 / <0 出错
- 踩的坑：setsocket 拼错、没重新编译跑的旧程序、telnet 退出是 quit 不是 exit
- 看到的现象：第二个连接卡住，断开第一个它才活——阻塞的本质是 read 把线程焊死在一个连接上
- 调试手段：关键路径加打印 + ss -tlnp 确认监听
- 终端基础操作：编译代码：cmake --build build，运行程序：./build/echo_server，启动telnet并访问：telnet 127.0.0.1 9999，退出telnet：quit

## 2026-11-14 · LT Epoll Echo Server

- 注册进入epoll里的文件必须设置非阻塞，使用按位或的方式设置标志位
- 在listen后创建epoll
- 给epoll注册/删除文件，使用epoll_ctl(epoll, op, fd, event); // epoll文件、操作、需要注册的文件、注册的事件。其中EPOLL_CTL_ADD是注册，EPOLL_CLT_DEL是删除
- epoll_event events[MAX_EVENTS] 是一个event数组，用作收件箱。当epoll_wait时，传入了events的首地址，内核向events里写入就绪的fd
- 获取epoll里就绪fd，使用epoll_wait(epoll_fd, events, max_event, timeout)->ready_events_num // to get ready fd; epoll文件、event数组首地址、event数组的容量、timeout ->就绪fd数
- 就绪的fd包括：1个listen_fd和n-1个accepted_fd
- 当epoll里的fd不需要使用后，先把fd从epoll注销，再关闭fd

## 2026-11-16 · ET Epoll Echo Server

- ET版本和LT版本的最大区别在于，ET版本的数据只在0->1时发出通知，产生事件，并被epoll_wait接收。所以程序每次都必须把一个event的数据读干净。
- 需要注册进epoll的event都加上EPOLLET参数(包括listen_fd和conn)：ev.events = EPOLLIN | EPOLLET;
- 调整listen_fd读取accept队列、conn读取字节流、conn写入字节流的方式，从单次读取改为循环读取直到读完

## 2026-11-16 · 客户端与EchoServerET交互全流程

### O0 Server启动

```
   Server 进程                    内核
─────────────────────────────────────────────
socket() ──────────────────────→ 分配 listen_fd
bind(:9999) ───────────────────→ 端口表登记 9999
listen(fd,128) ────────────────→ 标记为监听态，开accept队列
epoll_create1() ───────────────→ 建红黑树(注册表)+就绪链表
epoll_ctl(ADD listen_fd, EPOLLIN|EPOLLET)
        [登记卡: listen_fd, 关心可读] ─→ 放进红黑树
epoll_wait(...) ───────────────→ 就绪链表是空的 → 进程睡眠 💤
```

### O1 三次握手建立连接

```
   Client                          内核（协议栈）
─────────────────────────────────────────────
connect(127.0.0.1:9999) ──→ ① SYN ──────→ 回 SYN+ACK，把它排进
                                          listen_fd 的半/全连接队列
        ←────────── ② SYN+ACK
        ──→ ③ ACK ──────→  握手完成！连接进入 accept 队列
                           ⚡事件诞生：listen_fd 可读
                           把 listen_fd 挂上【就绪链表】
```

### O2 Server accept

```
   Server                          内核
─────────────────────────────────────────────
epoll_wait 返回，n=1
events[0] = { EPOLLIN, fd=listen_fd }
   ↓ data.fd == listen_fd → 是新连接事件！

accept 循环（ET 铁律）：
  accept() ─────────────────────→ 从 accept 队列取出连接
                                 ← 返回 conn_fd（socket 就在
                                    你和内核之间的第三个角色：conn_fd
                                    两端各一份，内核里是同一连接的两张脸）
  set_nonblocking(conn_fd) ─────→ 标志位拨上 O_NONBLOCK
  epoll_ctl(ADD conn_fd, EPOLLIN|EPOLLET)
        [登记卡: conn_fd, 关心可读, 边沿触发] ─→ 放进红黑树
  accept() 再调一次 ────────────→ 队列空了 → -1, errno=EAGAIN
  break，回到 epoll_wait ────────→ 无事 → 继续睡 💤
```

### O3 客户端发送数据

```
   Client                          内核                          Server
──────────────────────────────────────────────────────────────────
send("hello\n")
  ──→ 分段经网络到达
      网卡 → 协议栈
      数据写入 conn_fd
      的接收缓冲区 📦
                    ⚡ 数据从无到有 = 一个"边缘"！
                    ⚡ET 事件诞生：conn_fd 可读
                    挂上就绪链表
                    唤醒 sleeping 进程 ──→ epoll_wait 返回 n=1
                                           events[0]={EPOLLIN, conn_fd}

                                           ↓ 不是 listen_fd → 读事件

                                           read 循环（ET 铁律）：
                                           read(conn_fd, buf, 1024)
                                           ← 内核从缓冲区搬出 6 字节
                                           write(conn_fd, buf, 6)
                                           ──→ 写入发送缓冲区
                                               ──→ 协议栈 → 网卡
      ←────────────────────── "hello\n" 飞回客户端
      recv → 屏幕显示 hello ✅

                                           read() 再调一次
                                           ← 缓冲区已空 → -1, EAGAIN
                                           （"榨干了"的正常出口）
                                           break → epoll_wait
                                           ──→ 无事 → 继续睡 💤
```

### O4 客户端断开

```
   Client                          内核                          Server
──────────────────────────────────────────────────────────────────
Ctrl+] → quit
  ──→ 发 FIN
      内核：conn_fd 收到 FIN
      ⚡读事件（读到的将是 EOF）
      挂就绪链表，唤醒 server ──→ epoll_wait 返回
                                           read(conn_fd,...)
                                           ← 返回 0 ← 对端关闭的铁证
                                           epoll_ctl(DEL conn_fd)
                                           ──→ 从红黑树抽掉登记卡
                                           close(conn_fd) ──→ 释放
                                           回 epoll_wait 💤
```