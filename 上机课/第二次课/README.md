# 第二次上机课：资料入口

整理日期：2026-10-19。实验在 VMware Ubuntu 24 中完成，Windows 用于保存代码副本、报告与截图。

## 一、目录怎么使用

- [01_提交材料](D:/Text/21_操作系统/上机课/第二次课/01_提交材料/README.md)：你已经提取的代码文件夹和现有两份 RAR。保留原代码和压缩包，不把它们直接标为可发送的最终包。
- [02_实验报告](D:/Text/21_操作系统/上机课/第二次课/02_实验报告)：可编辑报告、最终问答和真实测试截图。
- [03_学习参考](D:/Text/21_操作系统/上机课/第二次课/03_学习参考/Lab2技术指导/README.md)：准备阶段的指导、参考代码及补丁，不代替已通过测试的实际代码。
- [04_历史与原始资料](D:/Text/21_操作系统/上机课/第二次课/04_历史与原始资料/README.md)：旧学习包与最初收到的报告素材，原样保留以便查阅。
- 原课件 [Lab_systemcalls_traps.pptx](D:/Text/21_操作系统/上机课/第二次课/Lab_systemcalls_traps.pptx) 留在本目录，没有移动或修改。

课件旁的 `~$Lab_systemcalls_traps.pptx` 是 Office 临时锁文件，本次未删除；关闭 PowerPoint 后通常由程序自行处理。

## 二、当前完成状态与报告入口

System calls 完整自动评分为 **50/50**，Traps 完整自动评分为 **95/95**，均以实际截图为依据。

System calls 材料目前分开维护：

- [Trace 实验报告](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Trace实验报告/学号_姓名_system_calls.md)。
- [Attack 实验记录](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Attack实验报告/Attack实验记录.md)。
- [GDB 六题分析、实测与最终评分](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/System_calls_GDB问答.md)。
- [可直接复制的 answers-syscall.txt](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/System_calls_GDB问答/answers-syscall.txt)。

Traps 已有一份涵盖 RISC-V 问答、Backtrace、Alarm 与最终评分的 [Traps 实验报告](D:/Text/21_操作系统/上机课/第二次课/02_实验报告/Traps实验报告/学号_姓名_Traps.md)。

## 三、提交前还要处理什么

已检查本地代码文件夹及两份 RAR 的文件列表：

- System calls 文件夹有 11 个文件，包含 9 个源码/构建文件、问答文件及耗时文件；未包含实验报告。
- Traps 文件夹有 17 个文件，包含 14 个源码/构建文件、问答与耗时文件，以及一份 Markdown 报告；未包含 PDF 和报告截图资源。
- 两份现有 RAR 都未包含 PDF 报告。本次只移动归类，没有重新压缩或覆盖它们。
- System calls 的三部分材料应整合成一份提交报告；Traps 报告需检查后转换为 PDF。
- 现有代码按 Trace、Attack、Backtrace、Alarm、Endian 分组，不是原来的 `kernel/`、`user/` 工程目录结构，不能直接解压作为完整工程编译。详见 [提交检查说明](D:/Text/21_操作系统/上机课/第二次课/01_提交材料/README.md)。

最终仍分成两个包，分别包含同名报告和对应实验代码：

```text
23300200015_张祺昀_system_calls
23300200015_张祺昀_Traps
```

课程仓库的 GitHub 同步不等于向老师提交实验；本次版本同步不发送实验邮件，正式提交仍按老师要求单独进行。

## 四、虚拟机中的原始工程

```text
/home/mimiqi/os/xv6-labs-2024      syscall 分支，正常实现及完整评分 50/50
/home/mimiqi/os/lab2-traps        traps 分支，完整评分 95/95
/home/mimiqi/os/xv6-syscall-gdb.sBWx5y   GDB 故障实验副本，不用于提交
```

本次 Windows 目录整理没有修改虚拟机中的文件，也没有在 Windows 或 WSL 中编译运行 xv6。

## 五、虚拟机连接检测

本次已在获准的本机网络环境中确认：

- 地址：`192.168.48.130`，网络可达。
- SSH 端口：`22`，TCP 连接成功，并收到 Ubuntu OpenSSH 服务响应。
- 已知账户名：`mimiqi`。
- 尚未建立 SSH 登录会话；严格主机检查提示本机还没有该地址的已信任 ED25519 主机密钥。
- 未自动接受指纹、写入 `known_hosts`、尝试密码、安装服务或修改虚拟机配置。

远端 SSH 握手提供的指纹如下，尚需在 VMware Ubuntu 控制台独立核对，不能仅凭远端握手就视为已验证：

```text
SHA256:c7bOhPRMZNL5VyMBOpZdb/nmWBrSItyPBTXJGmSW9kI
```

在 VMware Ubuntu 控制台可用下列只读命令查看主机公钥指纹：

```bash
ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
```

指纹核对后，再决定如何完成 SSH 登录认证。不要在聊天中发送密码或私钥。

## 六、本次整理范围

移动归类了 12 个顶层项目，移动后逐一核对了 105 个文件的 SHA256，内容均保持不变。随后仅修复 Markdown 路径并更新说明；源码、图片和现有压缩包未改写，没有删除文件。

四个报告目录整体移入 `02_实验报告`；`Lab2技术指导` 整体移入 `03_学习参考`；两份提交文件夹和两份 RAR 移入 `01_提交材料`；三份旧压缩资料移入 `04_历史与原始资料`。原文件夹和文件名均保留，需要恢复位置时可以按此映射移回，不依赖回收站。
