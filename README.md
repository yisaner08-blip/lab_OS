# lab_OS — 操作系统内核实验

基于 **x86 保护模式**的操作系统内核实验框架，运行于 QEMU 模拟器。本项目为操作系统课程设计（进程管理部分）的实现。

## 实验环境

| 项目 | 说明 |
|------|------|
| 架构 | x86 保护模式（32 位） |
| 编译器 | GCC 9.4 (Ubuntu 20.04) |
| 模拟器 | QEMU (qemu-system-i386) |
| 构建工具 | Docker（os-dev 镜像） |

> 注意：必须使用 GCC 9.x 编译，GCC 12（Ubuntu 22.04）会导致内核三重故障（triple fault）。

## 项目结构

```
lab_OS/
├── boot/               # 引导加载程序（实模式 → 保护模式）
│   ├── start.S         # BIOS 入口，加载 bootblock
│   ├── main.c          # 读取内核 ELF 到内存
│   └── asm.h           # 汇编宏定义
├── include/            # 头文件
│   ├── proc.h          # 进程控制块（PCB）定义
│   ├── irq.h           # 中断请求定义
│   ├── vm.h            # 虚拟内存管理
│   ├── x86.h           # x86 特定常量与宏
│   ├── list.h          # 通用双向链表
│   ├── types.h         # 基础类型定义
│   ├── const.h         # 常量定义
│   └── ...
├── src/
│   ├── init/
│   │   └── main.c      # 内核入口 entry() → os_init()
│   ├── kernel/
│   │   ├── proc/       # 进程管理
│   │   │   └── proc.c  # 就绪队列 + kthread_create
│   │   ├── irq/        # 中断处理
│   │   │   ├── do_irq.S     # 中断上下文切换（汇编）
│   │   │   ├── idt.c        # IDT 初始化
│   │   │   ├── i8259.c      # 8259A PIC 初始化
│   │   │   └── irq_handle.c # 中断处理分发
│   │   └── vm/         # 虚拟内存管理
│   │       └── kvm.c   # 内核页表初始化（0x0 → 0xC0000000 映射）
│   └── lib/            # 库函数
│       ├── vfprintf.c  # 格式化输出（printk）
│       ├── string.c    # 字符串操作
│       ├── debug.c     # 调试输出
│       └── abort.c     # panic 处理
├── Makefile            # 编译脚本
└── run_qemu.bat        # QEMU 运行脚本（Windows）
```

## 内核启动流程

```
BIOS → boot/start.S（实模式）
     → boot/main.c（读内核 ELF → 内存 0x100000）
     → src/init/main.c:entry()（开启分页，切换内核栈）
     → os_init()（初始化 GDT/IDT/PIC，创建进程）
```

关键机制：
- **页表映射**：物理地址 0x0 ~ 4MB 映射到虚拟地址 0xC0000000 ~ 0xC0400000
- **GDT**：内核代码段 / 数据段
- **IDT**：中断描述符表，处理时钟中断等
- **8259A PIC**：可编程中断控制器

## 已实现模块

### 模块一：进程创建

- **PCB（进程控制块）**：TrapFrame 指针、内核栈 (1024B)、PID、进程名、状态、调度信息
- **就绪队列**：基于通用双向链表 (`list.h`) 实现
- **kthread_create**：创建内核线程，在内核栈上手工构造假的 TrapFrame，使新进程在首次调度时能正确"恢复"上下文
- **测试**：创建 3 个进程并打印进程状态表

```
PID  Name      State     Priority  Counter
 0   ProcA       1         5         4
 1   ProcB       1         5         4
 2   ProcC       1         5         4
```

### 模块二：进程切换（开发中）

- 时钟中断处理
- do_irq.S 栈切换
- schedule() 调度函数

### 模块三：进程调度（待开发）

- 轮转调度（RR）+ Counter 优先级调度

### 模块四：进程同步（待开发）

- 计数信号量 P/V 操作
- 生产者-消费者问题

## 编译与运行

### 前提条件

- Docker（用于编译）
- QEMU（用于运行，Windows 下可用 `qemu-system-i386.exe`）

### 编译

```bash
# 使用 Docker（推荐）
docker build --network host -t os-dev .
docker run --rm -v "${PWD}:/workspace" os-dev bash -c "cd /workspace/lab_OS && make clean && make"

# 本地编译（需 GCC 9.x 32 位工具链）
make clean && make
```

### 运行

```bash
qemu-system-i386 -serial stdio kernel.img
```

## 许可证

本项目仅用于教育目的 — 操作系统课程设计。
