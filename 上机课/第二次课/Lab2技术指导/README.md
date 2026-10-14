# Lab2 技术指导与参考代码

本学习包对应第二次上机课，包含 Trace、RISC-V 问答、Backtrace 和 Alarm 的指导与参考代码。**Attack 按你的要求暂时跳过。**

交付内容是供阅读、对照和后续实现的材料，不要求现在实际运行。文中的安装、编译、QEMU 和测试命令均供将来在 **VMware 中的 Ubuntu 24** 使用；不在 Windows 或 WSL 中实施实验。参考代码未经编译或运行验证，文中的预期结果不代表实测结果。

## 阅读入口

建议先读通用说明，再按模块顺序阅读。

1. [通用说明与补丁使用](00_通用说明与补丁使用.md)：仓库版本、分支、环境边界、补丁顺序。
2. [Trace 技术指导](01_Trace/技术指导.md)：按进程保存跟踪 mask，接通系统调用，继承和打印。
3. [RISC-V 技术指导与中文参考答案](03_RISC-V/技术指导与参考答案.md)：六道题的逐题分析，包括内联、返回地址、大小端和可变参数。
4. [Backtrace 技术指导](04_Backtrace/技术指导.md)：读取帧指针、沿内核栈回溯和解析返回地址。
5. [Alarm 技术指导](05_Alarm/技术指导.md)：定时触发用户处理函数、保存完整现场、防止重入及恢复 a0。
6. [报告组织与可选验收](06_报告组织与可选验收.md)：把两个实验分开整理，区分理论分析和实测证据。

各模块目录中另有参考代码说明。RISC-V 问答附带可编辑的 `answers-traps-参考.txt` 和一个大小端演示程序；Backtrace、Alarm 附带补丁及修改后的完整文件。Trace 原有的实践记录模板保留为可选材料。

## 参考代码如何组合

`syscall` 和 `traps` 是两条不同的实验分支，不应把它们的修改混在同一个分支里。

- `syscall`：使用 Trace 补丁即可。本包没有 Attack 实现，因此不能据此宣称整个 System calls 实验已全部完成。
- `traps`：先使用 Backtrace 补丁，再使用 Alarm 补丁；或者在干净基线上只使用一次 `参考补丁/traps-all.patch`。
- RISC-V 问答不需要修改内核。第 3、4 题的具体地址依赖最终生成的 `user/call.asm`，提供查找方法和填写位置，不虚构固定地址。

逐模块补丁与合并补丁是两种替代方式，不能重复应用。完整文件用于对照，不建议整目录覆盖已有代码。

## 版本基线

官方仓库为 `git://g.csail.mit.edu/xv6-labs-2024`。

- `syscall`：`d5b09ff4571e07f6d996fdf7ce1533afc19a6978`。
- `traps`：`814d126edf03b5498dbc7e0796190b938837a851`。

第一次实验使用的新版 `mit-pdos/xv6-riscv` 不是这个实验仓库，接口也可能不同，例如 `pause()` 与 `sleep()`。不要把不同版本的指导和代码混用。

## 交付状态

四个未跳过模块的指导与参考材料已提供。已检查源码接口、补丁适用性、逐模块组合的一致性和文档链接；没有执行编译器、QEMU 或自动评分。

报告和运行截图是否必须提交，仍以老师要求为准。“本次不要求运行”是材料交付范围，不代表取消课程的实验要求。

## 资料依据

- 本课程第二次上机课课件，任务和报告要求以其为准。
- 本地保存的官方 `xv6-labs-2024` 源码，包括 `user/call.c`、`user/printf.c`、`user/alarmtest.c`、`kernel/proc.h` 和陷阱处理代码。
- [MIT System calls 实验说明](https://pdos.csail.mit.edu/6.1810/2024/labs/syscall.html)。
- [MIT Traps 实验说明](https://pdos.csail.mit.edu/6.1810/2024/labs/traps.html)。
- [RISC-V ABI 规范](https://riscv-non-isa.github.io/riscv-elf-psabi-doc/)。
