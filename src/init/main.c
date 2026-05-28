#include "kernel.h"
#include "x86.h"
#include "vm.h"
#include "irq.h"
#include "proc.h"
#include "sem.h"

/* ── 演示模式：1=进程创建  2=进程切换(RR)  3=进程调度(Counter)  4=进程同步 ── */
#define DEMO 4

/* ── 模块四用到的常量和变量 ── */
#define BUFFER_SIZE 5                // 缓冲区大小
struct Semaphore mutex, empty, full; // 互斥信号量，空缓冲区信号量，满缓冲区信号量
int buffer[BUFFER_SIZE];             // 缓冲区
int in = 0, out = 0;                 // 输入和输出指针
int produced = 0, consumed = 0;      // 已生产数量，已消费数量

/* ── 模块一/二/三用到的简单循环进程 ── */
void proc_a_entry(void) // 进程A的入口函数，循环打印自己的PID和被调度的次数
{
    while (TRUE)
    {
        printk("[ProcA] pid=%d run=%d\n",
               current->pid, current->run_count);
        volatile int i;
        for (i = 0; i < 5000000; i++)
            ;
    }
}

void proc_b_entry(void) // 进程B的入口函数，循环打印自己的PID和被调度的次数
{
    while (TRUE)
    {
        printk("[ProcB] pid=%d run=%d\n",
               current->pid, current->run_count);
        volatile int i;
        for (i = 0; i < 5000000; i++)
            ;
    }
}

void proc_c_entry(void) // 进程C的入口函数，循环打印自己的PID和被调度的次数
{
    while (TRUE)
    {
        printk("[ProcC] pid=%d run=%d\n",
               current->pid, current->run_count);
        volatile int i;
        for (i = 0; i < 5000000; i++)
            ;
    }
}

/* ── 模块四用的生产者和消费者 ── */
void producer_entry(void)
{
    int i;
    for (i = 0; i < 4; i++) // 生产4个产品
    {
        sem_wait(&empty);      // 申请空缓冲区
        sem_wait(&mutex);      // 申请互斥信号量
        buffer[in] = produced; // 生产一个产品
        printk("[P%d] produce #%d -> buf[%d]\n",
               current->pid, produced, in); // 打印生产的信息
        in = (in + 1) % BUFFER_SIZE;        // 更新输入指针
        produced++;                         // 生产数量加1
        sem_signal(&mutex);                 // 释放互斥信号量
        sem_signal(&full);                  // 释放满缓冲区信号量
    }
    printk("[P%d] producer finished\n", current->pid); // 生产者完成
    current->state = STOPED;                           // 设置为停止状态
    asm volatile("int $0x80");                         // 触发软中断，切换到其他进程
}

void consumer_entry(void)
{
    int i;
    for (i = 0; i < 4; i++) // 消费4个产品
    {
        sem_wait(&full);        // 申请满缓冲区
        sem_wait(&mutex);       // 申请互斥信号量
        int item = buffer[out]; // 消费一个产品
        printk("[C%d] consume #%d <- buf[%d]\n",
               current->pid, item, out); // 打印消费的信息
        out = (out + 1) % BUFFER_SIZE;   // 更新输出指针
        consumed++;                      // 消费数量加1
        sem_signal(&mutex);              // 释放互斥信号量
        sem_signal(&empty);              // 释放空缓冲区信号量
    }
    printk("[C%d] consumer finished\n", current->pid); // 消费者完成
    current->state = STOPED;                           // 设置为停止状态
    asm volatile("int $0x80");                         // 触发软中断，切换到其他进程
}

void os_init(void) // 初始化函数
{
    init_seg();                     // 初始化段寄存器
    init_debug();                   // 初始化调试输出
    init_idt();                     // 初始化中断描述符表
    init_i8259();                   // 初始化8259A中断控制器
    printk("%d+%d=%d", 10, 12, 22); // 测试输出
    printk("%s", "The OS is now working!\n");

    ready_queue_init(); // 初始化就绪队列

#if DEMO == 1
    /* ── 模块一：进程创建 ── */
    sched_algo = 0;
    kthread_create(proc_a_entry, "ProcA", 5);
    kthread_create(proc_b_entry, "ProcB", 5);
    kthread_create(proc_c_entry, "ProcC", 5);

    printk("\n=== Module 1: Process Creation ===\n");
    printk(" PID  Name      State     Priority  Counter\n");
    int i;
    for (i = 0; i < MAX_TASKS; i++)
    {
        struct task_struct *t = &task[i];
        if (t->state != UNUSED)
        {
            printk("  %d   %s       %d         %d         %d\n",
                   t->pid, t->name, t->state, t->priority, t->counter);
        }
    }
    printk("\n3 processes created, all RUNNABLE.\n");
    /* 模块一不开中断，仅演示 PCB 创建 */

#elif DEMO == 2
    /* ── 模块二：进程切换（RR 轮转）── */
    sched_algo = 0;
    kthread_create(proc_a_entry, "ProcA", 5);
    kthread_create(proc_b_entry, "ProcB", 5);
    kthread_create(proc_c_entry, "ProcC", 5);

    printk("\n=== Module 2: Process Switch (RR) ===\n");
    printk("A -> B -> C -> A -> B -> C ...\n\n");
    sti();

#elif DEMO == 3
    /* ── 模块三：进程调度（Counter 优先级）── */
    sched_algo = 1;
    kthread_create(proc_a_entry, "ProcA", 1);
    kthread_create(proc_b_entry, "ProcB", 3);
    kthread_create(proc_c_entry, "ProcC", 5);

    printk("\n=== Module 3: Counter Priority Scheduling ===\n");
    printk("Priority: ProcA=1  ProcB=3  ProcC=5 (larger = higher)\n");
    printk("ProcC runs 5 ticks -> ProcB 3 ticks -> ProcA 1 tick\n\n");
    sti();

#elif DEMO == 4
    /* ── 模块四：进程同步（生产者-消费者）── */
    sched_algo = 0;
    sem_init(&mutex, 1, "mutex");           // 初始化互斥信号量，初值为1，表示可用
    sem_init(&empty, BUFFER_SIZE, "empty"); // 初始化信号量
    sem_init(&full, 0, "full");

    kthread_create(producer_entry, "Prod1", 1); // 创建生产者1进程
    kthread_create(producer_entry, "Prod2", 1);
    kthread_create(consumer_entry, "Cons1", 1); // 创建消费者1进程
    kthread_create(consumer_entry, "Cons2", 1);

    printk("\n=== Module 4: Producer-Consumer (buf=%d) ===\n", BUFFER_SIZE);
    printk("2 Producers + 2 Consumers, 4 items each\n\n");
    sti();
#endif

    while (TRUE)
    {
        wait_intr(); // 进程执行完后停在这里，等待中断发生
    }
}

void entry(void)
{
    init_kvm();
    void (*volatile next)(void) = os_init;
    asm volatile("addl %0, %%esp" : : "p"(KOFFSET));
    next();
    panic("init code should never return");
}
