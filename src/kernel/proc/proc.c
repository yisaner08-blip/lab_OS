/* 进程管理相关函数的实现 */

#include "proc.h"
#include "kernel.h"

struct task_struct task[MAX_TASKS]; // 进程表，包含所有进程的PCB(数组形式)
struct task_struct *current = NULL; // 当前正在运行的进程的指针
ListHead ready_queue;               // 就绪队列，包含所有处于就绪状态的进程
static int next_pid = 0;            // 自增PID，用于分配新的进程ID
int sched_algo = 1;                 // 调度算法：0=RR, 1=Counter优先级

void ready_queue_init()
{
    list_init(&ready_queue); // 初始化就绪队列
}

void ready_queue_enqueue(struct task_struct *p) // 将一个进程加入就绪队列
{
    list_add_before(&ready_queue, &p->linklist); // 将进程加入就绪队列尾部
}

struct task_struct *ready_queue_dequeue() // 从就绪队列头部取出一个进程
{
    if (list_empty(&ready_queue))
        return NULL; // 就绪队列为空，返回NULL

    struct task_struct *p = list_entry(ready_queue.next, struct task_struct, linklist); // 获取就绪队列头部的进程
    list_del(&p->linklist);                                                             // 从就绪队列中移除该进程
    return p;                                                                           // 返回被取出的进程
}

struct task_struct *kthread_create(void (*entry)(void), const char *name, int priority) // 创建一个新的内核线程，entry是线程的入口函数，name是线程名称，priority是线程优先级
{
    struct task_struct *p = NULL; // p是指向新创建的进程的pcb的指针
    int i;
    for (i = 0; i < MAX_TASKS; i++) // 在进程表中寻找一个 UNUSED 状态的槽位来创建新进程
    {
        if (task[i].state == UNUSED) // 找到一个 UNUSED 的槽位
        {
            p = &task[i];
            break;
        }
    }
    if (p == NULL) // 没有找到 UNUSED 的槽位，无法创建新进程
        return NULL;

    p->pid = next_pid++; // 给新的进程分配一个pid
    int j;
    for (j = 0; j < sizeof(p->name) - 1 && name[j] != '\0'; j++) // 将传入的线程名称复制到进程的 name 字段中，确保不超过 20 字节并以 '\0' 结尾
        p->name[j] = name[j];
    p->name[j] = '\0';
    p->state = RUNNABLE;    // 设置进程状态为RUNNABLE
    p->priority = priority; // 设置进程优先级
    p->counter = priority;  // counter 初始值 = priority
    p->run_count = 0;       // 进程被调度的总次数

    // 在内核栈上手工构造假的 TrapFrame
    uint32_t *top = (uint32_t *)(p->kstack + KSTACKSIZE);
    // p->kstack:进程p的内核栈的起始地址（底）
    // KSTACKSIZE:内核栈的大小（字节）
    // top:指向内核栈的栈顶（高）
    //*top:指向内核栈的栈顶的值（高）
    //(uint32_t *):强制转换为unit32_t*类型，即指向uint32_t类型的指针

    *(--top) = 0x200; // eflags 0x200对应(IF=1),即开中断
    // EFLAGS 是 x86 CPU 的标志寄存器，存储了 CPU 的各种状态标志（如是否允许中断、运算结果是否为零等）
    *(--top) = KSEL(SEG_KCODE); // cs，SEG_KCODE在segment.h中定义，SEG_kCODE=1
    // CS（Code Segment）是 x86 CPU 的代码段寄存器，用来指示当前正在执行哪一段内存中的代码。
    *(--top) = (uint32_t)entry; // eip
    // eip:指令指针寄存器，存储了当前正在执行的指令的地址
    *(--top) = 0; // err
    // err:错误码，用于保存异常或中断处理程序返回的错误码
    *(--top) = -1; // irq = -1 表示非真实中断
    // irq:中断请求号，用于保存触发中断的设备号
    *(--top) = KSEL(SEG_KDATA); // ds
    *(--top) = KSEL(SEG_KDATA); // es
    *(--top) = KSEL(SEG_KDATA); // fs
    *(--top) = KSEL(SEG_KDATA); // gs
    // pushal 保存的 8 个通用寄存器，全部初始化为 0
    *(--top) = 0; // eax
    *(--top) = 0; // ecx
    *(--top) = 0; // edx
    *(--top) = 0; // ebx
    *(--top) = 0; // esp_
    *(--top) = 0; // ebp
    *(--top) = 0; // esi
    *(--top) = 0; // edi，最后栈顶指针指向TrapFrame的eip字段
    // 将构造好的 TrapFrame 的地址保存到进程的 tf 字段中，指向内核栈上的寄存器快照位置

    p->tf = (TrapFrame *)top; // 将构造好的 TrapFrame 的地址保存到进程的 tf 字段中，指向内核栈上的寄存器快照位置

    ready_queue_enqueue(p); // 将新创建的进程加入就绪队列
    return p;
}

void schedule(void) // 调度函数，根据调度算法选择下一个要运行的进程
{
    if (current != NULL && current->state == RUNNING) // 如果当前进程还在运行，先把它放回就绪队列
    {
        current->state = RUNNABLE;
        ready_queue_enqueue(current);
    }

    struct task_struct *next = NULL; // 用于保存下一个要运行的进程

    if (sched_algo == 0) // RR时间片轮转:sched_algo=0
    {
        // RR 轮转：直接取就绪队列队首
        next = ready_queue_dequeue();
    }
    else // Counter优先级调度:sched_algo=1
    {
        // Counter 优先级调度：遍历就绪队列找 counter 最大的进程
        ListHead *p;
        list_foreach(p, &ready_queue) // 遍历就绪队列中的每个进程
        {
            struct task_struct *t = list_entry(p, struct task_struct, linklist); // 获取就绪队列中的每个进程
            if (next == NULL || t->counter > next->counter)                      // 找到 counter 最大的进程
            {
                next = t;
            }
        }

        if (next == NULL || next->counter <= 0) // 如果所有进程的 counter 都耗尽了，重新分配时间片
        {
            // 所有进程 counter 都耗尽了，重新分配时间片
            list_foreach(p, &ready_queue)
            {
                struct task_struct *t = list_entry(p, struct task_struct, linklist);
                t->counter = (t->counter >> 1) + t->priority;
                //>> 1 体现在有进程阻塞后保留了 counter 的场景。模块三中三个进程都是纯 CPU 死循环，counter 同步耗尽，所以 >> 1
                // 暂时看不到效果。如果引入一个会主动阻塞的 I/O 型进程，它就能保留上次未用完的 counter，>> 1
                // 的效果会立即显现。
            }
            next = NULL;
            // 重新找 counter 最大的进程
            list_foreach(p, &ready_queue)
            {
                struct task_struct *t = list_entry(p, struct task_struct, linklist); // 获取就绪队列中的每个进程
                if (next == NULL || t->counter > next->counter)
                {
                    next = t;
                }
            }
        }

        if (next != NULL)
        {
            list_del(&next->linklist); // 从就绪队列中移除选中的进程
        }
    }

    if (next == NULL)
    {
        // 无就绪进程，所有进程已结束或阻塞
        printk("[schedule] all processes finished or blocked, halt\n");
        while (TRUE)
            wait_intr(); // 停机，等待中断，防止CPU空转
    }

    next->state = RUNNING; // 选择下一个进程
    next->run_count++;
    current = next;
}
