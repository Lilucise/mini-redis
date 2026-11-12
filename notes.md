## 2026-11-12 · CMake 骨架
- 为什么用 CMake：多文件项目的构建自动化，手敲 g++ 命令不可维护
- 关键三行：CXX_STANDARD 17 / REQUIRED ON / add_executable
- 踩的坑：cmake -B -build 打错，-B 后面跟的是目录名参数不是选项
- 为什么 build/ 要 gitignore：它随时可以删掉重新生成，不属于源码
- 下一步：epoll echo server，阻塞版 → epoll LT → ET