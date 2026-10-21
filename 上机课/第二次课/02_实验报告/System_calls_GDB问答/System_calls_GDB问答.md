# System calls：GDB 问答与实测步骤

- 整理日期：2026-10-19。
- 运行环境：VMware 中的 Ubuntu 24，使用 QEMU 运行 xv6；不在 Windows 或 WSL 中编译运行。
- 实验仓库与分支：`xv6-labs-2024`，`syscall`。
- Ubuntu 原实验目录：`/home/mimiqi/os/xv6-labs-2024`。
- 本次 Ubuntu 故障实验副本：`/home/mimiqi/os/xv6-syscall-gdb.sBWx5y`，用户已确认建立；后续故障修改与调试只在此目录进行。
- 状态：六题所需实测信息已核对完成，答案已保存到 Ubuntu 原实验目录；完整 System calls 自动评分实测为 **50/50**，全部评分项通过。故障地址为 `0x80001c94`，指令为 `lw s2,0(zero)`，`num` 在该位置对应 `s2`；故障断点处进程为 `initcode`，PID 为 1。固定答案正文保持不变。
- 唯一维护的提交目标文件：同目录 `answers-syscall.txt`。该文件只保留六题问答正文；本 Markdown 保留分析、操作步骤与完成状态，不是另一份提交版。
- 更新方式：固定维护同一份 `answers-syscall.txt`，不再新建不同版本。后续若重做实验、修改源码或改变编译环境，应重新核对具体地址和寄存器，并原位更新答案与证据。

问题依据为 [MIT 2024 System calls 实验的 Using gdb 部分](https://pdos.csail.mit.edu/6.1810/2024/labs/syscall.html#using-gdb)。这是 System calls 的问答，不是 Traps 的 RISC-V 六题。以下按六组整理。

## 一、六组问答

### 1. 哪个函数调用了 syscall()？

答案：`usertrap()`。

源码依据：`kernel/trap.c` 中，用户态的 `ecall` 进入陷阱处理；`usertrap()` 在识别到用户态系统调用后调用 `syscall()`，后者负责根据系统调用号分发处理函数。

实测结果：`backtrace` 中第 0 帧为 `syscall()`，第 1 帧为 `usertrap()`，对应 `kernel/trap.c:67`。这不是询问最初发起调用的用户程序，也不是询问具体的 `sys_exec()` 处理函数。

### 2. p->trapframe->a7 的值是多少，代表什么？

答案：本次在全新启动后第一次进入 `syscall()` 时，实测值为 `7`，即十六进制 `0x7`，代表 `SYS_exec`。

源码依据：`user/initcode.S` 将 `SYS_exec` 放入 `a7`，然后执行 `ecall`，请求加载 `/init`；`kernel/syscall.h` 定义 `SYS_exec` 为 7。用户寄存器在进入内核时保存到当前进程的 `trapframe`。

`a7` 表示系统调用编号，不是参数个数。后续任意一次进入 `syscall()` 时，该值可能不同；本题应观察启动后的第一次调用。

### 3. CPU 此前处于什么特权级？

答案：此前处于用户态（U-mode）；调试停在内核时，CPU 当前处于监督态（S-mode）。

判断依据：`sstatus` 的 SPP 位是第 8 位。`kernel/riscv.h` 中定义 `SSTATUS_SPP = 1L << 8`，其含义是上一特权级：0 表示用户态，1 表示监督态。

本次实测 `sstatus = 0x200000022`，`sstatus & 0x100 = 0x0`，因此上一特权级为用户态。不能因为当前正在执行内核代码，就把“上一特权级”写成监督态；整个 `sstatus` 数值只记录本次观测，不应当作所有构建、所有运行时刻的固定值。

### 4. 故障对应哪条汇编指令，num 对应哪个寄存器？

答案：本次故障指令位于 `0x80001c94`，为 `lw s2,0(zero)`；在该位置，变量 `num` 对应寄存器 `s2`。

官方练习要求临时将 `syscall()` 中读取系统调用号的语句改为读取空指针，然后根据 panic 的 `sepc` 定位本次构建的故障指令。

实测信息：

- 实测 `sepc`：`0x80001c94`。
- 同一构建中该地址的完整汇编指令：`lw s2,0(zero)`，机器码为 `00002903`。
- 结合反汇编数据流，`num` 在该位置对应的寄存器为 `s2`；后续 `addiw a4,s2,-1` 等指令使用它检查系统调用号。
- 实测 `scause`：`0xd`；实测 `stval`：`0x0`。

地址、指令和寄存器会受编译器、优化和已有代码修改影响。官方网页中的地址只是示例，不能直接抄成自己的结果。空指针读取属于 C 的未定义行为，应以实际产物为准。

`zero` 寄存器固定为 0，偏移量也是 0，因此该指令尝试从地址 0 加载一个字到 `s2`。GDB 的 `info locals` 显示 `num = <optimized out>`，但通过目标寄存器及后续数据流仍可确认寄存器分配。

断点停在该指令执行前，`info registers` 显示 `pc = 0x80001c94`、`s2 = 0x0`。此时加载尚未执行，且随后会因页故障失败，因此不能写成“从地址 0 成功读到 num = 0”。

### 5. 为什么会崩溃？内核是否映射了地址 0？

答案：内核页表没有映射虚拟地址 0。内核尝试通过空指针读取该地址时，触发页故障；这个实验中的内核异常处理不能恢复该访问，最终执行 `panic("kerneltrap")`。

源码依据：`kernel/vm.c` 的 `kvmmake()` 映射设备、内核代码、数据、物理内存、trampoline 和内核栈等区域，没有为虚拟地址 0 建立映射。

本次实测 `scause = 13`（`0xd`，load page fault），`stval = 0x0`，与空指针加载页故障一致；`sepc = 0x80001c94` 是故障指令所在地址，不是被读取的地址。

必须区分用户页表和内核页表：初始用户程序可在自己的页表中从虚拟地址 0 开始执行，不代表内核页表也映射了地址 0。本次 panic 已为上述异常解释提供实测依据。

### 6. 故障时运行的进程名称和 PID 是什么？

答案：在故障指令执行前的断点处，实测进程名称为 `initcode`，PID 为 `1`。

源码依据：`kernel/proc.c` 的 `userinit()` 创建第一个用户进程，并将名称设置为 `initcode`；初始 PID 分配从 1 开始。第一次 `exec` 尚未成功完成，进程名称还没有变成 `init`。

本次在 `0x80001c94` 的断点处查询 `p p->name` 和 `p p->pid`，分别得到有效名称 `initcode` 和 PID `1`。如果以后断点停在其他系统调用，不能套用这里的名称和 PID。

正常模式与故障断点处均已分别实测进程名称和 PID，两轮结果一致；不是用正常模式记录替代故障版本的查询。

## 二、先测正常系统调用：不修改实验代码

以下所有命令均在 VMware Ubuntu 中操作。打开两个 Ubuntu 终端，不是在 xv6 的 `$` 提示符下执行。

### 终端 A：启动等待调试的 QEMU

```bash
cd /home/mimiqi/os/xv6-labs-2024
git branch --show-current
make CPUS=1 qemu-gdb
```

分支应为 `syscall`。若不是，先停止并核对，不要带着未提交修改直接切分支。单 CPU 便于观察启动顺序；QEMU 等待调试器时没有出现 shell 是正常的。

### 终端 B：连接 GDB

```bash
cd /home/mimiqi/os/xv6-labs-2024
gdb-multiarch -q -nx -x .gdbinit
```

`.gdbinit` 由本项目的 `make qemu-gdb` 生成，包含对应连接端口及内核符号设置。若提示找不到 `gdb-multiarch` 或连接失败，先保留报错，不需要修改内核源码。

连接后，在 `(gdb)` 提示符下依次执行：

```gdb
set pagination off
set logging file gdb-normal.log
set logging overwrite on
set logging enabled on
break syscall
continue
backtrace
list
```

随后逐次执行 `next`，直到已经执行完 `struct proc *p = myproc();`，但尚未执行整个系统调用。不要一次连续输入很多个 `next`。

```gdb
next
```

确认 `p` 已初始化后，执行：

```gdb
p/x p->trapframe->a7
p/x $sstatus
p/x ($sstatus & 0x100)
p p->name
p p->pid
set logging enabled off
```

请保留 `backtrace` 和这些查询的真实输出。本轮只核对第 1、2、3 题，并记录正常启动时的进程信息；这还不等于完成了故障实验。

结束时，在 GDB 中执行 `detach`、`quit`，再到终端 A 按 `Ctrl+A`，松开后按 `X` 退出 QEMU。避免旧 QEMU 实例占用下一次实验的调试端口。

### 本次正常模式的实测记录

用户提供的图一回溯输出为：

```text
(gdb) backtrace
#0  syscall () at kernel/syscall.c:160
#1  0x0000000080001a3e in usertrap () at kernel/trap.c:67
#2  0x0000000000000000 in ?? ()
Backtrace stopped: frame did not save the PC
```

该输出已清楚显示 `syscall()` 的直接调用者为 `usertrap()`。末尾提示表示 GDB 无法继续展开更早的栈帧，不影响本题对直接调用者的判断。

![正常模式 GDB 调用栈](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-normal-backtrace.png)

用户提供的图二中，成功执行的查询结果为：

```text
(gdb) p/x p->trapframe->a7
$1 = 0x7
(gdb) p/x $sstatus
$2 = 0x200000022
(gdb) p/x ($sstatus & 0x100)
$3 = 0x0
(gdb) p p->pid
$4 = 1
```

据此确认系统调用号为 `SYS_exec`、上一特权级为用户态、正常模式下当前进程 PID 为 1。图二未包含 `p p->name` 的输出，进程名称由随后补充的图三确认。

![正常模式 GDB 寄存器与 PID](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-normal-registers.png)

随后，用户补充执行 `p p->name`。图三中 GDB 显示进程名为 `initcode`，后面跟随若干 `\000`，表示固定长度字符数组中的空字节，不属于进程名称。

![正常模式 GDB 进程名](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-normal-process-name.png)

正常模式的调用栈、系统调用号、上一特权级、进程名和 PID 均已完成核对。本次故障实验在下面的独立副本中进行，原目录保持正常实现。

## 三、故障实验：只在 Ubuntu 独立副本中修改

原目录已经有通过测试的 Trace、Attack 实现。不要为了问答覆盖它的 `syscall()`，也不要使用 `git reset` 或 `git checkout --` 恢复文件。

### 1. 复制当前完整实验目录

退出原 QEMU 和 GDB 后，在 Ubuntu 终端执行：

```bash
syscall_gdb_dir=$(mktemp -d /home/mimiqi/os/xv6-syscall-gdb.XXXXXX)
cp -a /home/mimiqi/os/xv6-labs-2024/. "$syscall_gdb_dir/"
printf '%s\n' "$syscall_gdb_dir"
cd "$syscall_gdb_dir"
pwd
```

记下打印出的实际副本路径。确认当前路径为新建的 `xv6-syscall-gdb.*` 目录后，才继续；任何一步报错都先停止。复制包含未提交的实验修改，不会只取旧 Git 提交。

本次已经完成复制，实际副本路径为 `/home/mimiqi/os/xv6-syscall-gdb.sBWx5y`。继续本次实验时直接进入该目录，无需再次创建副本。

### 2. 在副本中临时制造空指针故障

用编辑器打开副本的 `kernel/syscall.c`，仅将下面这一行：

```c
num = p->trapframe->a7;
```

临时替换为：

```c
num = *(int *)0;
```

这项修改用于观察内核异常，不是最终提交代码。在确认仍位于副本目录后执行：

```bash
make clean
make CPUS=1 qemu
```

在副本中重新构建，避免使用带有原目录调试路径的旧产物。记录实际 panic 的 `scause`、`sepc`、`stval`，截图后退出 QEMU。

本次故障版本已经运行，用户提供的实际输出为：

```text
xv6 kernel is booting

scause=0xd sepc=0x80001c94 stval=0x0
panic: kerneltrap
```

![空指针故障的实际 panic 输出](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-null-panic.png)

故障地址取自本次构建，不使用官方网页中的示例地址。后续保持副本源码不变，已使用 GDB 调试同一构建的 `0x80001c94`，结果见下一节。

### 3. 定位同一构建的故障指令

查看副本内的 `kernel/kernel.asm`，查找实测 `sepc` 对应地址。也可在故障地址断点处用 GDB 检查。

终端 A 仍在副本目录执行：

```bash
make CPUS=1 qemu-gdb
```

终端 B 进入刚才记下的实际副本路径，再执行：

```bash
gdb-multiarch -q -nx -x .gdbinit
```

在 GDB 中设置指令地址断点。对于本次没有再修改源码的故障构建，执行 `break *0x80001c94`；若以后重新修改并构建，应重新采集 `sepc`，不要沿用过期地址。

设置断点后输入 `continue` 继续运行。命中断点后，执行：

```gdb
x/i $pc
disassemble /r syscall
info registers
info locals
p p->name
p p->pid
```

此时断点停在故障指令执行前。本次输出已经用于第 4、6 题，之前 panic 的异常信息用于第 5 题。若以后构建中的 `p` 被优化掉，应保留反汇编和报错，再根据实际代码定位，不要把猜测写成观测值。

本次使用 `CPUS=1`。若局部变量 `p` 被优化掉，可改为只读查询 `p cpus[0].proc->name` 和 `p cpus[0].proc->pid`；`cpus[0].proc` 是该单 CPU 当前运行进程的指针，不需要从 GDB 调用内核函数。

### 本次故障断点的实测记录

用户提供的六张截图补齐了故障指令、完整反汇编、寄存器、局部变量与进程信息。

`x/i $pc` 显示：

```text
=> 0x80001c94 <syscall+18>: lw s2,0(zero) # 0x0
```

![故障指令](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-fault-instruction.png)

完整反汇编中的关键连续指令为：

```text
0x0000000080001c92 <+16>: 84aa      mv    s1,a0
0x0000000080001c94 <+18>: 00002903  lw    s2,0(zero)
0x0000000080001c98 <+22>: fff9071b  addiw a4,s2,-1
0x0000000080001c9c <+26>: 47d5      li    a5,21
0x0000000080001c9e <+28>: 04e7ed63  bltu  a5,a4,0x80001cf8
```

故障指令以 `s2` 为加载目标，后续系统调用号范围检查继续使用 `s2`，因此它对应这一位置的 `num`。这一结论来自同一构建的真实反汇编，不来自对 GDB 变量数值的猜测。

![syscall 完整反汇编](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-fault-disassembly.png)

`info registers` 显示当前 `pc = 0x80001c94`、`s2 = 0x0`。这些值是故障加载执行前的现场，不表示已经成功执行了空指针读取。

![故障断点寄存器](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-fault-registers.png)

`info locals` 显示：

```text
num = <optimized out>
p = 0x8000a810 <proc>
```

`num` 的调试信息无法直接给出其值，不妨碍通过反汇编确定寄存器分配；本次 `p` 仍然可直接查询。

![故障断点局部变量](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-fault-locals.png)

在同一断点处，`p p->name` 的有效字符串为 `initcode`，`p p->pid` 为 `1`，完成第 6 题的故障现场核对。

![故障断点进程名称](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-fault-process-name.png)

![故障断点 PID](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/gdb-fault-pid.png)

### 4. 提交前回到原来的正常实验目录

结束副本的调试后，回到 `/home/mimiqi/os/xv6-labs-2024`，不要在故障副本中打包或运行最终评分。

在 GDB 中执行 `detach`、`quit`，再到 QEMU 窗口按 `Ctrl+A`，松开后按 `X`，结束此次调试。故障副本可以暂时保留作为证据，但不能替代原目录中的正常实验代码。

## 四、整理为提交文件

同目录的 `answers-syscall.txt` 是唯一维护的六题答案正文，六题所需信息已经实测核对完成，所有占位符已经清除。可以直接复制文件全文，保存为 Ubuntu 原实验目录中的：

`/home/mimiqi/os/xv6-labs-2024/answers-syscall.txt`

用户已在 Ubuntu 原目录保存答案，并通过 `grep -n '^[1-6]\.'` 确认六个题目均可读取，题目起始行号依次为 1、8、14、22、35、44，与维护版一致。这是文件保存检查，不是完整评分结果。

答案中的地址、寄存器、异常信息和进程查询均来自本次对应构建的真实输出。若之后改变源码或编译环境，应重新核对，而不是沿用可能已经变化的地址。报告所用截图已保存在同目录的 `assets` 中。

另外创建 `/home/mimiqi/os/xv6-labs-2024/time.txt`，只填实际投入实验的小时数，以一个整数表示；不要复制 Traps 的耗时或随意填数。

用户已在原目录运行完整 `make grade`，实测总分为 **50/50**，结果及截图见下一节。评分脚本对问答文件只作基本检查，不能将自动检查通过等同于教师已经人工审定答案。

## 五、System calls 完整自动评分：50/50

运行位置为 VMware Ubuntu 中的正常实验目录 `/home/mimiqi/os/xv6-labs-2024`，不是空指针故障副本。执行完整评分命令：

```bash
make grade
```

本次截图中的结果摘要如下，耗时保留该轮真实显示值：

```text
answers-syscall.txt: OK
trace 32 grep: OK (1.6s)
trace close grep: OK (0.9s)
trace exec + open grep: OK (0.9s)
trace all grep: OK (1.0s)
trace nothing: OK (1.0s)
trace children: OK (11.4s)
attack: OK (0.2s)
time: OK
Score: 50/50
```

![System calls 完整评分 50/50](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/assets/system-calls-grade-50of50.png)

结论：本次完整评分中，六项 Trace 测试、Attack 测试、问答文件检查和耗时文件检查均通过，System calls 自动评分为 **50/50**。不将该分数解释为未包含在评分中的所有异常路径均已验证，也不据此编造 `time.txt` 中的具体小时数。

提交应使用正常实验目录，故障副本仅用于 GDB 问答实验。System calls 与 Traps 仍按课程要求分开打包。

## 六、依据与范围

- 官方题目：[MIT 2024 System calls — Using gdb](https://pdos.csail.mit.edu/6.1810/2024/labs/syscall.html)。
- 源码核对：本地参考 `syscall` 分支的 `user/initcode.S`、`kernel/syscall.h`、`kernel/trap.c`、`kernel/proc.c`、`kernel/riscv.h`、`kernel/vm.c`、`Makefile` 和评分脚本。
- 本文未使用其他人的故障地址、寄存器或成绩作为自己的实测记录；未在 Windows 或 WSL 中运行 xv6。
