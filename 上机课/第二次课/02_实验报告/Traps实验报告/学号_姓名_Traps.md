# Traps 实验报告

- 学号：23300200015
- 姓名：张祺昀
- 实验环境：VMware 虚拟机中的 Ubuntu 24
- 实验源码：MIT `xv6-labs-2024`
- 实验分支：`traps`
- 实验目录：`/home/mimiqi/os/lab2-traps`
- 交叉编译工具前缀：`riscv64-linux-gnu-`

## 一、实验目的与完成情况

Traps 实验包含 RISC-V 问答、Backtrace 和 Alarm 三部分。本次已完成六题问答、大小端演示、Backtrace 与 Alarm 的实现和专项验收，以及 usertests 回归检查，最终完整自动评分为 95/95。

- RISC-V 问答：六题答案已整理。前四题依据本机生成的 `user/call.asm`，第五题有实际运行结果，第六题采用理论分析。
- Backtrace：**已完成。bttest 输出三条内核地址，addr2line 解析与预期调用链一致，专项评分 backtrace test: OK（1.8s）。**
- Alarm：**已完成。手动 alarmtest 与专项自动评分的 test0～test3 全部通过，usertests -q 显示 ALL TESTS PASSED。**
- 完整自动评分：**95/95，问答文件、Backtrace、Alarm、usertests 和耗时文件检查全部通过。**


## 二、RISC-V 问答的准备过程

在 Ubuntu 中，使用独立目录保存 `traps` 分支：

```bash
mkdir -p ~/os
cd ~/os
git clone --branch traps --single-branch git://g.csail.mit.edu/xv6-labs-2024 lab2-traps
cd lab2-traps
git branch --show-current
```

确认分支为 `traps` 后，查看示例源码并编译：

```bash
cat user/call.c
make fs.img
```

`user/call.c` 的核心代码如下：

```c
int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  printf("%d %d\n", f(8)+1, 13);
  exit(0);
}
```

随后读取本次编译生成的汇编文件：

```bash
sed -n '1,120p' user/call.asm
grep -n '<printf>:' user/call.asm
```

反汇编文件显示目标程序格式为 `elf64-littleriscv`。以下问题中的地址均取自本次实际生成的汇编，不使用课件示例中的固定地址。

## 三、RISC-V 六道问答

### 3.1 第一题：参数使用哪些寄存器？13 保存在哪里？

RISC-V 使用 `a0` 至 `a7` 传递整数或指针参数。对于本例的 `printf()`：

- `a0` 保存格式字符串的地址。
- `a1` 保存第一个待打印的整数，即 `f(8)+1`，其值为 12。
- `a2` 保存第二个待打印的整数，即 13。

实际汇编为：

```asm
24: 4635                 li    a2,13
26: 45b1                 li    a1,12
28: 00001517             auipc a0,0x1
2c: 84850513             addi  a0,a0,-1976 # 870 <malloc+0x100>
30: 68c000ef             jal   6bc <printf>
```

因此，题目中的 13 保存在 **`a2`**。格式字符串本身也占用一个参数位置，所以 13 虽然是第二个待打印的数，却使用第三个参数寄存器 `a2`。

### 3.2 第二题：main 中对 f、g 的调用在哪里？

本次生成的 `main` 中没有对 `f`、`g` 的实际调用。编译器已经将 `f(8)+1` 优化为常量 12，直接使用：

```asm
26: 45b1                 li a1,12
```

`f` 中也没有调用 `g`。其实际代码如下：

```asm
000000000000000e <f>:
   e: 1141                 addi  sp,sp,-16
  10: e422                 sd    s0,8(sp)
  12: 0800                 addi  s0,sp,16
  14: 250d                 addiw a0,a0,3
  16: 6422                 ld    s0,8(sp)
  18: 0141                 addi  sp,sp,16
  1a: 8082                 ret
```

其中 `addiw a0,a0,3` 直接完成了 `g(x)` 的运算，说明 `g` 的运算已被内联进 `f`。在 `main` 中，相关计算进一步被折叠为常量。

汇编文件仍保留独立的 `g` 和 `f` 函数，它们的入口分别为 `0x0` 和 `0xe`。这与 `main` 不调用它们并不矛盾：函数符号存在，不代表本次调用路径一定经过这些函数。

### 3.3 第三题：printf 的地址是什么？

查找结果为：

```text
1205:00000000000006bc <printf>:
```

因此，本次用户程序中的 `printf` 符号地址为 **`0x6bc`**。

前面的 `1205:` 是 `grep -n` 输出的文本行号，不是函数地址。`main` 中的调用指令也指向同一地址：

```asm
30: 68c000ef             jal 6bc <printf>
```

### 3.4 第四题：调用 printf 后，ra 的值是多少？

本机实际生成的调用及其下一条指令为：

```asm
30: 68c000ef             jal 6bc <printf>
34: 4501                 li  a0,0
```

位于 `0x30` 的 `jal` 指令编码为 4 字节，建立的返回地址为下一条指令的地址：

```text
ra = 0x30 + 4 = 0x34
```

所以答案是 **`0x34`**，即执行该调用指令后建立的返回地址。

题干使用 `jalr` 表述，但本次最终生成的指令为 `jal`。本题应依据实际汇编回答，不能直接套用其他编译结果的指令地址或返回地址。

### 3.5 第五题：大小端代码的输出是什么？改为大端时如何调整？

题目中的核心代码为：

```c
unsigned int i = 0x00646c72;
printf("H%x Wo%s", 57616, (char *)&i);
```

本次在 xv6 中实际观察到的核心输出是：

```text
HE110 World
```

原因分为整数格式化与字符串读取两部分。

首先，57616 的十六进制数值为 `E110`。本实验版本的 xv6 `printf` 使用大写十六进制字符表，所以 `%x` 输出 `E110`。因此，格式字符串开头的 `H%x` 得到 `HE110`。

其次，本次环境采用小端字节序。32 位整数 `0x00646c72` 在内存中从低地址到高地址的字节顺序为：

```text
72 6c 64 00
 r  l  d \0
```

`(char *)&i` 指向这些字节，`%s` 将它们解释为以零字节结尾的字符串，输出 `rld`。前面的 `Wo` 与 `rld` 拼接后得到 `World`。

演示程序额外打印了各字节的实际值：

```text
bytes low to high: 72 6C 64 0
```

这与上述小端排列一致。`%x` 不补前导零，最后显示的 `0` 表示值为 `0x00` 的字符串结束字节。

如果假设机器改用大端字节序，为保持从低地址开始仍然是 `72 6c 64 00`，应将整数改为：

```c
unsigned int i = 0x726c6400;
```

**57616 不需要改变。** `%x` 根据整数数值生成文本，`%s` 则按地址读取内存字节，大小端变化影响的是后者对整数内存的解释。

上述大端情形为理论推导，本次没有将实际运行环境切换为大端。

### 3.6 第六题：缺少可变参数时，y 会输出什么？

题目代码为：

```c
printf("x=%d y=%d", 3);
```

**不能确定 `y=` 后的数值，这个调用属于 C 语言的未定义行为。**

格式字符串要求两个整数参数，但调用者只提供了一个。按照本例的 RISC-V 参数传递规则，格式字符串在 `a0`，整数 3 在 `a1`，第二个整数本应通过 `a2` 传递，但该参数并不存在。

在常见的机器执行过程中，`printf` 可能读到对应参数位置的残留内容；具体表现与编译器和执行现场有关。但这只是一种底层机制解释，C 语言不保证该调用的结果。



## 四、大小端演示程序与实测记录

### 4.1 演示程序

为了验证第五题，单独增加 `user/endian.c`，保留原来的 `user/call.c` 用于汇编分析。演示代码如下：

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  unsigned int i = 0x00646c72;
  unsigned char *bytes = (unsigned char *)&i;

  printf("H%x Wo%s\n", 57616U, (char *)&i);
  printf("bytes low to high: %x %x %x %x\n",
         (unsigned int)bytes[0], (unsigned int)bytes[1],
         (unsigned int)bytes[2], (unsigned int)bytes[3]);

  exit(0);
}
```

第一行输出在题目格式字符串基础上增加了换行，第二行用于观察内存字节。`57616U` 使用无符号整数常量，与 `%x` 的参数类型对应，数值仍为 57616。

### 4.2 编译与运行

在 `Makefile` 的 `traps` 用户程序列表中加入：

```makefile
	$U/_endian\
```

在 Ubuntu 终端中执行：

```bash
cd ~/os/lab2-traps
make qemu
```

进入 xv6 shell 后执行 `endian`，实际结果如下：

```text
$ endian
HE110 World
bytes low to high: 72 6C 64 0
$
```

![图 1 xv6 中的大小端演示结果](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/endian-result.png)

图 1 为实际运行截图，证明格式化输出与内存字节观察符合第五题的分析。截图底部的 QEMU 终止提示不属于 `endian` 程序的输出。

**本项演示通过。**

### 4.3 本阶段材料

本阶段使用或产生的主要材料包括：

- `user/call.c`：实验提供的调用约定示例。
- `user/call.asm`：本次编译生成的反汇编，是前四题的分析依据。
- `user/endian.c`：新增的大小端演示程序。
- `Makefile`：将演示程序加入用户程序列表。
- 本报告第三节：六道题的中文答案，已整理为 `answers-traps.txt`，并通过最终自动文件检查。
- `assets/endian-result.png`：本次大小端演示的运行截图。

> 待补充截图：建议保留 `call.asm` 中 g、f、main 的关键汇编段，以及 printf 符号地址的终端截图。

## 五、Backtrace 实验

**状态：已完成实现、地址解析和 Backtrace 专项评分。**

### 5.1 初次运行结果

在 `kernel/riscv.h`、`kernel/defs.h`、`kernel/printf.c` 和 `kernel/sysproc.c` 中加入回溯实现及调用后，使用单 CPU 启动 xv6，执行 `bttest`，得到：

```text
$ bttest
backtrace:
0x0000000080001dac
0x0000000080001cbc
0x0000000080001a3e
0x000000000000000e
$
```

![图 2 Backtrace 初次运行的四条地址](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/backtrace-initial.png)

前三条位于内核地址范围，第四条 `0x000000000000000e` 是额外的低地址。当时尚未完成地址解析与专项评分，不能仅凭该截图确认验收通过。后续先修正多余输出，再解析与验证内核调用链。

### 5.2 栈顶边界问题与修正方案

初版循环使用 `fp <= high`，允许继续读取位于栈页上界的最外层帧。根据陷阱入口代码，内核栈指针设置为栈页顶，随后通过不改写 `ra` 的跳转进入 `usertrap`。因此，最外层帧保存的 `ra` 不保证属于内核调用链，可能保留用户态值。

当前修正方案是把循环上界改为严格小于栈页顶，并使下一帧检查同时排除页顶：

```c
while(fp >= low + 16 && fp < high && (fp & 15) == 0){
  uint64 ra = *(uint64 *)(fp - 8);
  uint64 previous = *(uint64 *)(fp - 16);
  printf("%p\n", (void *)ra);
  if(previous <= fp || previous >= high)
    break;
  fp = previous;
}
```

上述边界修正已在 Ubuntu 中复测，结果见下一节；随后完成了 `addr2line` 解析与专项评分，记录见第 5.5、5.6 节。

### 5.3 修正后复测结果

修改边界判断、重新编译并启动 xv6 后，再次运行 `bttest`，得到：

```text
$ bttest
backtrace:
0x0000000080001dac
0x0000000080001cbc
0x0000000080001a3e
```

![图 3 Backtrace 修正后输出三条内核地址](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/backtrace-retest.png)

本次输出只包含三条内核地址，初版中额外的 `0x000000000000000e` 已消失，说明本次运行中的栈顶边界问题已得到修正。该截图本身只能确认回溯输出的条数与地址范围；结合后续的地址解析和自动评分，才完成本模块的验收。

### 5.4 实现原理与修改内容

RISC-V 中的 `s0` 同时用作帧指针 `fp`。本实验保留帧指针，依据当前内核构建的栈帧布局：

- `fp - 8` 保存返回地址 `ra`，表示当前函数返回后继续执行的位置。
- `fp - 16` 保存调用者的帧指针，可据此找到上一层栈帧。
- 栈向低地址增长，沿调用者方向回溯时，帧指针应严格增大。

首先在 `kernel/riscv.h` 增加读取 `s0` 的内联函数：

```c
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x));
  return x;
}
```

在 `kernel/printf.c` 中实现 `backtrace()`，先取得当前帧指针，确定其所在内核栈页的上下界，再使用第 5.2 节的循环逐层读取并打印：

```c
uint64 fp = r_fp();
uint64 low = PGROUNDDOWN(fp - 1);
uint64 high = low + PGSIZE;
printf("backtrace:\n");
```

使用 `fp - 1` 计算栈页，是为了在初始帧指针恰好落在页顶时仍定位到其下方的栈页。循环读取之前要求 `fp >= low + 16`，保证两个槽位均未越过页底；要求 `fp < high`，排除最外层陷阱入口的特殊帧；同时检查 16 字节对齐。下一帧不严格增大或到达页顶时立即停止，避免错误链或循环回溯。

其余修改如下：

- `kernel/defs.h`：声明 `void backtrace(void);`，供其他内核文件调用。
- `kernel/sysproc.c`：在 `sys_sleep()` 中取得参数并处理负值后、获取 `tickslock` 前调用 `backtrace()`。`bttest` 通过调用 `sleep()` 进入该路径。
- `kernel/printf.c`：在 `panic()` 打印错误信息后、设置 `panicked = 1` 前调用 `backtrace()`，用于故障诊断。本次未主动触发 panic，该入口没有单独的运行验收记录。

上述回溯依赖编译时保留帧指针，即 `-fno-omit-frame-pointer`。它沿内核调用链打印返回地址，不是读取用户栈，也不应把最外层保存的用户态返回值混入内核回溯。

### 5.5 返回地址解析

退出 QEMU 后，在 VMware Ubuntu 的 `/home/mimiqi/os/lab2-traps` 中，使用产生这些地址的同一份 `kernel/kernel` 执行：

```bash
riscv64-linux-gnu-addr2line -f -e kernel/kernel \
  0x0000000080001dac \
  0x0000000080001cbc \
  0x0000000080001a3e
```

实际输出为：

```text
sys_sleep
/home/mimiqi/os/lab2-traps/kernel/sysproc.c:62
syscall
/home/mimiqi/os/lab2-traps/kernel/syscall.c:141 (discriminator 1)
usertrap
/home/mimiqi/os/lab2-traps/kernel/trap.c:76
```

![图 4 Backtrace 返回地址的 addr2line 解析结果](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/backtrace-addr2line.png)

三条地址按输入顺序分别对应：

1. `0x0000000080001dac`：`sys_sleep`，`kernel/sysproc.c:62`。
2. `0x0000000080001cbc`：`syscall`，`kernel/syscall.c:141`。
3. `0x0000000080001a3e`：`usertrap`，`kernel/trap.c:76`。

这与进入 `backtrace()` 前的内核调用链 `usertrap → syscall → sys_sleep → backtrace` 一致；回溯按相反方向打印各层保存的返回地址。地址是调用后的继续执行位置，不是函数入口，源码行号也可能映射到调用之后的语句。`discriminator 1` 是编译器调试信息的区分标记，不表示错误。

本次先解析地址，再运行专项评分，避免评分清理并重新构建后造成旧地址与新内核不匹配。上述地址与源码行号仅对应本次构建。

### 5.6 自动评分与结论

在 Ubuntu 终端中执行 Backtrace 专项评分：

```bash
make GRADEFLAGS=backtrace grade
```

实际结果为：

```text
== Test backtrace test == backtrace test: OK (1.8s)
```

![图 5 Backtrace 专项评分通过](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/backtrace-grade.png)

结合手动 `bttest`、三条返回地址的源码解析以及专项评分结果，可以确认 **Backtrace 专项验收通过**。本次修正的关键是区分“地址仍在栈页范围内”与“该槽位确实属于应打印的内核调用链”，不能仅以未越出页边界作为停止条件。

本节记录 Backtrace 专项结果；Alarm 及完整 Traps 评分见第六节。`panic()` 入口未单独触发测试，不将专项通过扩展为所有内核故障路径均已验证。

## 六、Alarm 实验

**状态：实现、手动测试、Alarm 专项自动评分与 usertests 回归检查均已通过，最终完整 Traps 自动评分为 95/95。**

### 6.1 实验目标与修改文件

实现 `sigalarm(interval, handler)` 与 `sigreturn()`：进程累计指定数量的用户态时钟 tick 后，转去执行用户处理函数；处理函数调用 `sigreturn()` 后，恢复被打断的寄存器和指令位置。`sigalarm(0, 0)` 关闭后续通知。

实现过程分为接口接通、进程状态保存、定时触发与现场恢复三个阶段，前两个阶段先完成编译检查，再替换临时占位函数。最终方案涉及以下文件：

- `Makefile`：保留 `call`、`bttest`、`endian`，将 `alarmtest` 加入镜像。
- `user/user.h`、`user/usys.pl`：声明用户接口并生成系统调用入口。
- `kernel/syscall.h`、`kernel/syscall.c`：在本实验的 `traps` 分支使用编号 22、23，注册两个内核处理函数。
- `kernel/proc.h`、`kernel/proc.c`、`kernel/defs.h`：保存每进程状态，提供初始化和清理函数。
- `kernel/exec.c`：成功替换程序映像后关闭旧 Alarm。
- `kernel/sysproc.c`：登记处理函数，并实现现场恢复。
- `kernel/trap.c`：在用户态时钟陷阱中计数、备份现场并转向处理函数。

### 6.2 每进程状态与生命周期

在 `struct proc` 中加入：

```c
int alarm_interval;
int alarm_ticks;
uint64 alarm_handler;
int alarm_active;
struct trapframe alarm_saved;
```

`alarm_interval` 是触发间隔，0 表示关闭；`alarm_ticks` 是当前累计值；`alarm_handler` 是用户处理函数的地址；`alarm_active` 标记处理函数是否正在运行；`alarm_saved` 是被打断现场的独立副本。

备份直接嵌入进程结构体，不需要额外分配页面。不能只保存指向当前 `trapframe` 的指针，因为处理函数后续发起系统调用时，当前现场会被再次改写。

`alarmreset(p)` 将上述状态清零，并分别在新进程分配、进程回收以及成功 `exec()` 后调用。成功 `exec()` 后旧 handler 地址已经失效，应关闭 Alarm；若 `exec()` 失败，则不提前清除原程序的状态。本方案中，新建子进程默认关闭 Alarm，不继承父进程的配置。

### 6.3 登记与定时触发

`sys_sigalarm()` 读取间隔与用户函数地址，拒绝负间隔，更新配置并将计数清零：

```c
uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;
  struct proc *p = myproc();

  argint(0, &interval);
  argaddr(1, &handler);
  if(interval < 0)
    return -1;

  p->alarm_interval = interval;
  p->alarm_handler = handler;
  p->alarm_ticks = 0;
  return 0;
}
```

这里不能清除 `alarm_active` 或备份现场：测试中的处理函数会先调用 `sigalarm(0, 0)` 禁止后续触发，再调用 `sigreturn()` 恢复当前这一次被打断的执行。

在 `usertrap()` 的时钟分支中，只有间隔大于 0 且当前未在执行 handler 时才累计 tick：

```c
if(which_dev == 2){
  if(p->alarm_interval > 0 && !p->alarm_active){
    p->alarm_ticks++;
    if(p->alarm_ticks >= p->alarm_interval){
      p->alarm_saved = *p->trapframe;
      p->alarm_active = 1;
      p->alarm_ticks = 0;
      p->trapframe->epc = p->alarm_handler;
    }
  }
  yield();
}
```

先完整备份现场，再改变 `epc`。原有的返回用户态路径会使用新的 `epc`，因此无需在内核中直接调用用户处理函数。`alarm_active` 防止处理函数被自身的定时通知重复打断；处理期间暂停下一轮计数。原来的 `yield()` 保留，以免破坏时钟驱动的调度。

本方案只在中断用户态执行的时钟陷阱中计数，不在 `kerneltrap()` 中增加相同逻辑，也不使用全局墙钟经过的时间代替进程的累计值。

### 6.4 sigreturn 与 a0 恢复

处理函数不是通过普通 C 函数调用进入的，不能仅靠 `return` 回到原程序；必须由 `sigreturn()` 恢复现场：

```c
uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  if(!p->alarm_active)
    return -1;

  uint64 saved_a0 = p->alarm_saved.a0;
  *p->trapframe = p->alarm_saved;
  p->alarm_active = 0;
  p->alarm_ticks = 0;
  return saved_a0;
}
```

恢复内容不只有 `epc`，还包括栈指针、返回地址、参数及其他用户寄存器，确保继续原程序时计算状态不被 handler 破坏。

系统调用分发器最后会执行 `p->trapframe->a0 = syscalls[num]();`。因此 `sys_sigreturn()` 返回原来保存的 `a0`，使分发器写回的值仍与被打断时相同；若返回 0，就会把已经恢复的 `a0` 再次覆盖。返回用户态前，`usertrapret()` 会重新设置所需的内核辅助字段。

### 6.5 手动测试结果

在 VMware Ubuntu 中编译并启动 xv6：

```bash
cd ~/os/lab2-traps
make kernel/kernel fs.img
make CPUS=1 qemu
```

在 xv6 shell 中运行 `alarmtest`，实际截图显示以下四项全部通过。下面只摘录通过标记，省略中间的点号和 `alarm!`：

```text
test0 passed
test1 passed
test2 passed
test3 passed
```

![图 6 Alarm 手动测试 test0 至 test3 全部通过](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/alarmtest-manual.png)

原版测试各项对应的检查目标为：

- `test0`：处理函数能够被触发。
- `test1`：能够重复触发，恢复后继续正确执行原循环。
- `test2`：长时间运行的处理函数不会重入，关闭后续通知后仍可恢复现场。
- `test3`：恢复后保留原来的 `a0` 值。

本次结果证明上述原版手动测试均已通过，没有据此推断其他尚未运行的检查也已通过。

### 6.6 usertests 回归检查

在同一 VMware Ubuntu 实验环境的 xv6 shell 中运行：

```text
usertests -q
```

用户提供的实际截图显示，测试末尾为：

```text
test sbrklast: OK
test sbrk8000: OK
test badarg: OK
ALL TESTS PASSED
```

![图 7 usertests 回归检查显示 ALL TESTS PASSED](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/usertests-regression.png)

因此，本次 `usertests -q` 回归检查通过。图 7 只保留了该次手动回归的末尾结果，不填写未记录的运行时长；最终自动回归结果及完整评分见第 6.8 节。

### 6.7 Alarm 专项自动评分

退出 QEMU 后，在 VMware Ubuntu 的实验目录中执行：

```bash
cd ~/os/lab2-traps
make GRADEFLAGS=alarmtest grade
```

实际评分截图显示：

```text
== Test running alarmtest == (5.2s)
== Test   alarmtest: test0 ==
  alarmtest: test0: OK
== Test   alarmtest: test1 ==
  alarmtest: test1: OK
== Test   alarmtest: test2 ==
  alarmtest: test2: OK
== Test   alarmtest: test3 ==
  alarmtest: test3: OK
```

![图 8 Alarm 专项自动评分四项全部通过](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/alarmtest-grade.png)

因此，**Alarm 专项自动评分通过**。图 8 中的 5.2 秒对应该次 `running alarmtest` 阶段，不是每个子项的独立耗时。完整 Traps 总分以第 6.8 节的最终评分为准。

### 6.8 最终完整 Traps 自动评分：95/95

在 VMware Ubuntu 的实验目录中运行不带筛选条件的完整评分：

```bash
cd ~/os/lab2-traps
make grade
```

最终实际输出摘要如下：

```text
answers-traps.txt: OK
backtrace test: OK (2.5s)
alarmtest: test0: OK
alarmtest: test1: OK
alarmtest: test2: OK
alarmtest: test3: OK
usertests: OK (68.9s)
time: OK
Score: 95/95
```

![图 9 最终完整 Traps 自动评分 95/95](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/assets/traps-grade-95of95.png)

最终截图中，问答文件、Backtrace、Alarm 四个子项、usertests 和耗时文件检查均为 `OK`，总分为 **95/95**。其中本次 Backtrace 检查用时 2.5 秒，`running alarmtest` 阶段用时 3.5 秒，usertests 检查用时 68.9 秒；这些数值仅对应本次最终评分，不与前面的单独测试用时混用。问答文件的自动检查只验证基本内容，六题答案仍以报告中的分析及教师人工评阅为准。

## 七、实验结论与提交说明

RISC-V 部分已经根据本机汇编回答参数寄存器、编译优化、函数地址和返回地址问题，并在 xv6 中实际验证了大小端演示。可变参数缺失问题采用 C 语言未定义行为与调用约定进行分析，没有虚构固定输出。

本次编译的关键地址是 `printf = 0x6bc`，调用 `printf` 建立的返回地址为 `0x34`。这些数值仅对应本次生成的用户程序；若修改示例程序、编译选项或重新得到不同汇编，应同步核对答案。

Backtrace 已完成栈顶边界修正、三条返回地址解析及专项评分；Alarm 已完成定时登记、时钟触发、完整现场恢复、防重入与 `a0` 保留，手动测试和专项评分的 test0～test3 均通过。手动 `usertests -q` 显示 `ALL TESTS PASSED`，最终完整自动评分中所有检查均为 `OK`，总分为 **95/95**。

通过本实验，可以区分系统调用的同步进入与时钟中断的异步进入，并理解恢复用户执行不仅需要正确的指令地址，还必须恢复完整寄存器现场；Backtrace 则说明了帧指针链、返回地址和栈边界在内核诊断中的作用。
