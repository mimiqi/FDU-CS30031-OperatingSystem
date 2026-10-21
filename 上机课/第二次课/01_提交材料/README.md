# 提交材料检查说明

检查日期：2026-10-19。本目录保留用户已提取的代码和已生成的 RAR，未覆盖或重新打包压缩文件。

## 当前内容

- [System calls 代码文件夹](D:/Text/21_操作系统/上机课/第二次课/01_提交材料/23300200015_张祺昀_system_calls)：11 个文件，Trace 8 个修改文件、Attack 1 个修改文件、`answers-syscall.txt` 和 `time.txt`。
- [Traps 代码与 Markdown 文件夹](D:/Text/21_操作系统/上机课/第二次课/01_提交材料/23300200015_张祺昀_Traps)：17 个文件，14 个源码/构建文件、`answers-traps.txt`、`time.txt` 和一份报告 Markdown。
- [现有 System calls RAR](D:/Text/21_操作系统/上机课/第二次课/01_提交材料/23300200015_张祺昀_system_calls.rar)。
- [现有 Traps RAR](D:/Text/21_操作系统/上机课/第二次课/01_提交材料/23300200015_张祺昀_Traps.rar)。

两份 RAR 的文件列表均未出现 PDF 报告。Traps RAR 含 Markdown，但没有报告引用的截图资源，不能仅将该 Markdown 当作可独立查看的完整报告。

整理时为了保持本地阅读可用，已修复文件夹内 Traps Markdown 的图片路径；RAR 内仍是之前的快照，未同步修改。最终提交前应补齐报告，再重新打包并检查包内文件。

## 代码路径如何还原

现有文件按实验模块分组，下面是还原到对应官方分支时的路径映射。两个实验必须使用各自版本的同名文件，不能混在一个工程中。

### System calls：对应 syscall 分支

来源工程：`/home/mimiqi/os/xv6-labs-2024`。

- `Trace/Makefile` → 工程根目录 `Makefile`。
- `Trace/proc.c`、`Trace/proc.h`、`Trace/syscall.c`、`Trace/syscall.h`、`Trace/sysproc.c` → 工程 `kernel/` 下的同名文件。
- `Trace/user.h`、`Trace/usys.pl` → 工程 `user/` 下的同名文件。
- `Attack/attack.c` → 工程 `user/attack.c`。
- 根目录 `answers-syscall.txt`、`time.txt` → 工程根目录。

### Traps：对应 traps 分支

来源工程：`/home/mimiqi/os/lab2-traps`。

- `Alarm/Makefile` → 工程根目录 `Makefile`。
- `Alarm/exec.c`、`Alarm/proc.c`、`Alarm/proc.h`、`Alarm/syscall.c`、`Alarm/syscall.h`、`Alarm/trap.c` → 工程 `kernel/` 下的同名文件。
- `Alarm/user.h`、`Alarm/usys.pl` → 工程 `user/` 下的同名文件。
- `Backtrace/defs.h`、`Backtrace/printf.c`、`Backtrace/riscv.h`、`Backtrace/sysproc.c` → 工程 `kernel/` 下的同名文件。
- `Endian/endian.c` → 工程 `user/endian.c`。
- 根目录 `answers-traps.txt`、`time.txt` → 工程根目录。

其中 `Backtrace/defs.h`、`Backtrace/sysproc.c` 已检查包含 Alarm 相关声明或处理函数；不能因为目录名是 Backtrace，就用早期仅完成 Backtrace 的版本替换。

本次只检查了文件清单、这些共享接口和原有评分证据，没有在 Windows 中重新编译，也没有通过 SSH 比较本地提取文件与虚拟机当前文件的哈希。

## 最终提交检查

1. 将 System calls 的 GDB、Trace、Attack 材料整合成一份同名实验报告，并转换、检查 PDF。
2. 检查 Traps 报告的学号姓名、图片和最终 95/95 记录，转换、检查 PDF。
3. 两份报告和两个提交包分别按 `23300200015_张祺昀_system_calls`、`23300200015_张祺昀_Traps` 命名。
4. 若只交修改文件，保留源码原始目录层级，或附清晰路径说明；需要可直接编译的工程时，应从 Ubuntu 对应原目录取完整工程，在该目录执行 `make clean` 后打包。
5. 不使用 `/home/mimiqi/os/xv6-syscall-gdb.sBWx5y` 的故障版本作为提交代码。
6. 压缩后重新查看文件列表，确认同名报告和代码确实都在包内，再按老师要求提交。

报告与全部截图的工作版本集中在 [02_实验报告](D:/Text/21_操作系统/上机课/第二次课/02_实验报告)。
