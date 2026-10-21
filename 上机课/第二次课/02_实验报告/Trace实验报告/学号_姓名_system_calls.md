# System calls 实验报告：Trace

- 学号：23300200015
- 姓名：张祺昀
- 实验环境：VMware 虚拟机中的 Ubuntu 24
- 实验源码：MIT `xv6-labs-2024`
- 实验分支：`syscall`
- 实验目录：`/home/mimiqi/os/xv6-labs-2024`
- 交叉编译工具前缀：`riscv64-linux-gnu-`

本报告专门记录 System calls 实验中的 Trace 模块，包括实现过程、手动测试和 Trace 专项自动评分。后续已完成 Attack、GDB 问答及耗时文件，完整 System calls 自动评分实测为 **50/50**，见第 4.6 节。Attack 的实现另见[Attack 实验记录](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Attack实验报告/Attack实验记录.md)，GDB 问答及实测证据另见[GDB 问答记录](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/System_calls_GDB问答.md)。

## 一、实验目的

本实验在 xv6 中增加 `trace(mask)` 系统调用，使进程能够选择需要跟踪的系统调用。对于选中的调用，内核在调用返回后打印进程号、系统调用名称和返回值。

通过本实验，理解以下内容：

1. 用户程序如何通过系统调用入口进入内核，以及内核如何根据调用编号选择处理函数。
2. 如何在进程控制结构中保存进程独立的状态。
3. 如何使用位掩码选择需要跟踪的系统调用。
4. 如何在 `fork()` 中继承跟踪设置，并使设置在 `exec()` 后继续生效。
5. 如何通过手动测试和自动评分验证功能及进程间隔离。

## 二、实验原理与设计

### 2.1 系统调用的执行过程

用户程序调用 `trace(mask)` 时，首先进入由 `user/usys.pl` 生成的汇编入口。入口将系统调用编号 `SYS_trace` 放入 `a7`，执行 `ecall` 进入内核。函数参数按照 RISC-V 调用约定传递，第一个整数参数位于 `a0`。

进入内核时，用户寄存器保存在当前进程的 `trapframe` 中。`kernel/syscall.c` 的 `syscall()` 读取 `p->trapframe->a7`，以该编号索引 `syscalls[]`，调用对应的处理函数。处理完成后，返回值写入 `p->trapframe->a0`，并在返回用户态后交给用户程序。

本实验为 Trace 分配系统调用编号 22，并将它映射到 `sys_trace()`。

### 2.2 位掩码的含义

`mask` 的第 n 位表示是否跟踪编号为 n 的系统调用。判断编号 `num` 是否被选中的表达式是：

```c
(p->trace_mask & (1U << num)) != 0
```

本实验使用的典型掩码包括：

- `32`：等于 `1 << 5`，而 `SYS_read` 为 5，因此只跟踪 `read`。
- `2`：等于 `1 << 1`，而 `SYS_fork` 为 1，因此只跟踪 `fork`。
- `2147483647`：十六进制为 `0x7fffffff`，低 31 位均为 1，覆盖本实验中已有的系统调用编号。

打印条件额外检查 `num < 32`，避免对 32 位掩码执行超出范围的移位。使用 `1U` 表示无符号整数，使移位运算与 `uint` 类型的掩码一致。

### 2.3 进程独立状态与继承

跟踪掩码保存在 `struct proc` 的 `trace_mask` 字段中，没有使用全局掩码。这样，某个进程设置跟踪后，不会改变其他进程的跟踪状态。

新进程分配时将掩码初始化为 0；释放进程槽位时再次清零；`fork()` 创建子进程时，将父进程的掩码复制给子进程。子进程后续再调用 `fork()`，其后代也会继续继承。

`trace` 用户程序先设置掩码，再通过 `exec()` 执行目标程序。`exec()` 更换用户程序的地址空间，但继续使用当前进程结构，且本实现没有在 `exec()` 中清除掩码，因此目标程序仍处于跟踪状态。

### 2.4 输出时机

跟踪输出放在下面这条语句之后：

```c
p->trapframe->a0 = syscalls[num]();
```

此时系统调用已经执行完毕，返回值也已写入 `a0`。这还保证了 `trace()` 自身修改掩码后，如果新掩码选中了 `SYS_trace`，该次 `trace()` 也能够被记录。

输出格式为：

```text
进程号: syscall 系统调用名称 -> 返回值
```

返回值按 64 位有符号整数打印，能够显示 `-1` 等错误返回值。对于正常执行后不再返回到该打印位置的调用，例如 `exit()`，不会产生返回后的跟踪行。

## 三、修改文件与实现过程

本次提交的源码素材包含 8 个修改文件：

1. `Makefile`：将 `trace` 用户程序加入文件系统镜像。
2. `user/user.h`：声明用户可调用的 `trace()` 函数。
3. `user/usys.pl`：增加系统调用汇编入口的生成项。
4. `kernel/syscall.h`：定义 `SYS_trace` 编号。
5. `kernel/proc.h`：增加进程独立的跟踪掩码。
6. `kernel/proc.c`：初始化、清理和继承掩码。
7. `kernel/sysproc.c`：实现 `sys_trace()`。
8. `kernel/syscall.c`：登记处理函数、维护名称表并输出跟踪信息。

以下展示关键修改，完整代码见随报告提交的源码文件。

### 3.1 接通用户态入口

在 `Makefile` 的 `syscall` 实验编译列表中增加：

```makefile
	$U/_trace\
```

这使已有的 `user/trace.c` 被编译为用户程序，并打包进入 `fs.img`。

在 `user/user.h` 中增加函数声明：

```c
int trace(int);
```

在 `user/usys.pl` 末尾增加：

```perl
entry("trace");
```

由构建过程自动生成对应的汇编入口，无需手动修改生成文件 `user/usys.S`。

在 `kernel/syscall.h` 中增加：

```c
#define SYS_trace 22
```

完成这些修改后，执行 `make fs.img` 编译检查。构建输出中的镜像文件列表包含 `user/_trace`，并正常完成 `mkfs`，说明用户程序已编译并打包成功。

### 3.2 增加进程掩码字段

在 `kernel/proc.h` 的 `struct proc` 中增加：

```c
uint trace_mask;
```

在 `kernel/proc.c` 的 `allocproc()` 中初始化：

```c
p->pid = allocpid();
p->state = USED;
p->trace_mask = 0;
```

在 `freeproc()` 释放进程槽位时清理：

```c
p->trace_mask = 0;
p->state = UNUSED;
```

在 `fork()` 复制用户寄存器后，复制父进程的掩码：

```c
*(np->trapframe) = *(p->trapframe);
np->trace_mask = p->trace_mask;
```

这里 `p` 指向父进程，`np` 指向新创建的子进程。复制操作发生在子进程被设置为可运行之前。

### 3.3 实现系统调用处理函数

在 `kernel/sysproc.c` 中增加：

```c
uint64
sys_trace(void)
{
  int mask;

  argint(0, &mask);
  myproc()->trace_mask = (uint)mask;
  return 0;
}
```

`argint(0, &mask)` 读取用户传入的第一个整数参数；`myproc()` 获取当前进程；随后将参数写入当前进程的掩码字段，并返回 0 表示设置成功。

本实验版本的 `argint()` 返回类型为 `void`，因此直接调用，不对它进行错误返回值判断。

完成进程字段和处理函数后，执行 `make kernel/kernel`，内核编译通过，没有出现 `error`。

### 3.4 登记处理函数与调用名称

在 `kernel/syscall.c` 的处理函数声明区域增加：

```c
extern uint64 sys_trace(void);
```

在 `syscalls[]` 中增加编号与处理函数的映射：

```c
[SYS_trace] = sys_trace,
```

另外增加 `syscall_names[]` 名称数组，使用与系统调用表相同的编号索引。例如，实际数组中的部分条目如下：

```c
[SYS_fork]   = "fork",
[SYS_read]   = "read",
[SYS_exec]   = "exec",
[SYS_close]  = "close",
[SYS_trace]  = "trace",
```

完整数组为本实验已有的 22 个系统调用均设置了名称，避免手动按顺序排列字符串造成编号错位。

### 3.5 增加跟踪输出

在 `syscall()` 已确认调用编号有效的分支中，执行处理函数并保存返回值后增加：

```c
p->trapframe->a0 = syscalls[num]();
if(num < 32 && (p->trace_mask & (1U << num)) != 0){
  printf("%d: syscall %s -> %ld\n",
         p->pid, syscall_names[num],
         (long)p->trapframe->a0);
}
```

这段代码只在当前进程的掩码选中该编号时打印。未知系统调用仍由原有的错误分支处理，不会用非法编号索引名称数组。

## 四、测试结果与分析

### 4.1 仅跟踪 read

在 xv6 shell 中执行：

```text
$ trace 32 grep hello README
3: syscall read -> 1023
3: syscall read -> 971
3: syscall read -> 298
3: syscall read -> 0
```

输出只包含 `read`，与掩码 32 选择 `SYS_read` 的设计一致。前三次返回值表示各次实际读取的字节数，最后返回 0 表示到达文件末尾。具体字节数与 README 内容和读取过程有关，不要求与课件示例完全一致。

**测试结果：通过。**

> 待补充截图 1：包含上述命令及四行 read 输出的真实终端截图。

### 4.2 跟踪多个系统调用

执行：

```text
$ trace 2147483647 grep hello README
4: syscall trace -> 0
4: syscall exec -> 3
4: syscall open -> 3
4: syscall read -> 1023
4: syscall read -> 971
4: syscall read -> 298
4: syscall read -> 0
4: syscall close -> 0
```

输出包含 `trace`、`exec`、`open`、`read` 和 `close`，说明编号分发、名称映射和返回值打印均能工作。

第一行 `trace -> 0` 证明输出条件在处理函数执行后检查：`sys_trace()` 已设置新掩码，且新掩码包含 `SYS_trace` 对应位。`exec()` 之后继续出现 `grep` 的读取操作，说明跟踪设置在程序替换后仍保留。

**测试结果：通过。**

> 待补充截图 2：包含上述命令及多个系统调用输出的真实终端截图。

### 4.3 普通程序不受影响

随后直接运行：

```text
$ grep hello README
$
```

命令结束后返回提示符，没有出现系统调用跟踪行。这说明前一个 Trace 进程设置的掩码没有改变 shell 或后来普通进程的跟踪状态，符合每个进程独立保存掩码的设计。

**测试结果：通过。**

> 待补充截图 3：同时包含命令和下一行提示符，以证明没有跟踪输出。

### 4.4 子进程继承掩码

执行：

```text
$ trace 2 usertests forkforkfork
```

以下摘录实际输出的开头和结尾，中间大量 fork 跟踪行省略：

```text
usertests starting
3: syscall fork -> 4
test forkforkfork: 3: syscall fork -> 5
5: syscall fork -> 6
6: syscall fork -> 7
6: syscall fork -> 8
6: syscall fork -> 9
7: syscall fork -> 10
...
7: syscall fork -> -1
6: syscall fork -> -1
8: syscall fork -> -1
OK
3: syscall fork -> 66
ALL TESTS PASSED
```

掩码 2 只选中 `fork`。日志中不仅有初始进程 3 的输出，还出现了进程 5、6、7、8 等后代进程的输出，说明掩码能够沿着多代 `fork()` 继承。

压力测试中的部分 `fork()` 返回 `-1`，表示当时无法继续创建进程，符合进程资源耗尽时的预期。结合最终的 `OK` 和 `ALL TESTS PASSED`，可以判断本项测试通过。

**测试结果：通过。**

> 待补充截图 4：建议分别截取不同进程号的跟踪输出，以及末尾的 OK、ALL TESTS PASSED。

### 4.5 Trace 专项自动评分

退出 QEMU，回到 Ubuntu 终端后执行：

```bash
make GRADEFLAGS=trace grade
```

实际结果如下：

```text
== Test trace 32 grep == trace 32 grep: OK (2.7s)
== Test trace close grep == trace close grep: OK (0.9s)
== Test trace exec + open grep == trace exec + open grep: OK (1.0s)
== Test trace all grep == trace all grep: OK (1.1s)
== Test trace nothing == trace nothing: OK (0.9s)
== Test trace children == trace children: OK (17.8s)
```

六项测试分别覆盖单独跟踪 `read`、单独跟踪 `close`、同时选择 `exec` 与 `open`、跟踪多个调用、普通程序不受影响，以及子进程继承。

![图 1 Trace 专项自动评分结果](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Trace实验报告/assets/trace-grade.png)

图 1 为原 Trace 专项评分的实际截图，六项 Trace 测试均为 `OK`。该截图仅用于记录专项测试，后续完整评分及总分另见第 4.6 节。

**测试结果：六项 Trace 专项测试全部通过。** 此命令使用 `trace` 筛选测试，专项结果本身不能代替完整实验评分。

### 4.6 后续完整 System calls 自动评分

完成 Attack、GDB 问答文件 `answers-syscall.txt` 和耗时文件 `time.txt` 后，在 VMware Ubuntu 的正常实验目录 `/home/mimiqi/os/xv6-labs-2024` 中执行 `make grade`，实际总分为 **50/50**。

本轮六项 Trace 测试、Attack 测试、问答文件检查和耗时文件检查全部为 `OK`。这轮 Trace 测试耗时与原专项评分不同，两轮均保留各自真实结果，不混用耗时。

![图 2 System calls 完整自动评分 50/50](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/system-calls-grade-50of50.png)

完整结果摘要见[GDB 问答与最终评分记录](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/System_calls_GDB问答.md)。本节只补充最终验收，不替代前面 Trace 模块的实现分析。

## 五、遇到的问题与处理

### 5.1 一次手动测试出现程序加载失败

最初手动执行子进程测试时，终端出现：

```text
$ trace 2 usertests forkforkfork
exec trace failed
```

该报错由 xv6 shell 在 `exec()` 未能成功加载 `trace` 程序时输出。仅凭这条信息，无法确定当时加载失败的具体原因。

随后 Trace 专项自动评分中的 `trace children` 通过。之后重新启动 QEMU，手动执行同一条命令，成功出现多个后代进程的跟踪输出，以及 `OK`、`ALL TESTS PASSED`。这次复测没有修改 Trace 的实现代码。

因此，本实验记录为“曾出现一次程序加载失败，后续自动测试和手动复测通过，具体原因未进一步确定”，不将其归因于已经确认的某项源码错误。

### 5.2 正确认识压力测试中的失败返回值

`forkforkfork` 测试会持续创建进程，部分 `fork()` 返回 `-1` 是资源不足时的正常返回，并不意味着 Trace 实现错误。本实验的跟踪功能应如实显示该值，而不是隐藏或改写它。最终测试程序仍输出 `ALL TESTS PASSED`。

## 六、实验总结

本实验为 xv6 增加了 `trace(mask)` 系统调用，完成了用户态声明与入口、调用编号分配、内核处理函数登记、进程掩码管理和返回后的跟踪输出。

实现中，将 mask 放入 `struct proc`，使不同进程的设置相互独立；在 `fork()` 时复制掩码，使后代进程继承设置；在系统调用处理完毕后检查掩码，能够输出实际返回值，并记录 `trace()` 自身启用跟踪的结果。

手动测试验证了按掩码选择、多个调用的输出、普通进程不受影响和子进程继承。六项 Trace 专项自动测试全部通过，说明本实现满足这些测试覆盖的 Trace 要求。

本次实验也说明，分析问题时需要区分测试程序本身的失败返回值、shell 的程序加载错误，以及系统调用跟踪实现中的错误，并结合实际输出和复测结果作出判断。
