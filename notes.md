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

## 2026-11-14 · Epoll Echo Server

- 注册进入epoll里的文件必须设置非阻塞，使用按位或的方式设置标志位
- 在listen后创建epoll
- 给epoll注册/删除文件，使用epoll_ctl(epoll, op, fd, event); // epoll文件、操作、需要注册的文件、注册的事件。其中EPOLL_CTL_ADD是注册，EPOLL_CLT_DEL是删除
- epoll_event events[MAX_EVENTS] 是一个event数组，用作收件箱。当epoll_wait时，传入了events的首地址，内核向events里写入就绪的fd
- 获取epoll里就绪fd，使用epoll_wait(epoll_fd, events, max_event, timeout)->ready_events_num // to get ready fd; epoll文件、event数组首地址、event数组的容量、timeout ->就绪fd数
- 就绪的fd包括：1个listen_fd和n-1个accepted_fd
- 当epoll里的fd不需要使用后，先把fd从epoll注销，再关闭fd