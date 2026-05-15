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

void ready_queue_enqueue(struct task_struct *p)
{
    list_add_before(&ready_queue, &p->linklist); // 将进程加入就绪队列尾部
}

struct task_struct *ready_queue_dequeue()
{
    if (list_empty(&ready_queue))
        return NULL; // 就绪队列为空，返回NULL

    struct task_struct *p = list_entry(ready_queue.next, struct task_struct, linklist); // 获取就绪队列头部的进程
    list_del(&p->linklist);                                                             // 从就绪队列中移除该进程
    return p;                                                                           // 返回被取出的进程
}

struct task_struct *kthread_create(void (*entry)(void), const char *name, int priority)
{
    struct task_struct *p = NULL;
    int i;
    for (i = 0; i < MAX_TASKS; i++)
    {
        if (task[i].state == UNUSED)
        {
            p = &task[i];
            break;
        }
    }
    if (p == NULL)
        return NULL;

    p->pid = next_pid++;
    int j;
    for (j = 0; j < sizeof(p->name) - 1 && name[j] != '\0'; j++)
        p->name[j] = name[j];
    p->name[j] = '\0';
    p->state = RUNNABLE;
    p->priority = priority;
    p->counter = priority;   // counter 初始值 = priority
    p->run_count = 0;

    // 在内核栈上手工构造假的 TrapFrame
    uint32_t *top = (uint32_t *)(p->kstack + KSTACKSIZE);

    *(--top) = 0x200;           // eflags (IF=1)
    *(--top) = KSEL(SEG_KCODE); // cs
    *(--top) = (uint32_t)entry; // eip
    *(--top) = 0;               // err
    *(--top) = -1;              // irq = -1 表示非真实中断
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
    *(--top) = 0; // edi

    p->tf = (TrapFrame *)top;

    ready_queue_enqueue(p);
    return p;
}

void schedule(void)
{
    if (current != NULL && current->state == RUNNING)
    {
        current->state = RUNNABLE;
        ready_queue_enqueue(current);
    }

    struct task_struct *next = NULL;

    if (sched_algo == 0)
    {
        // RR 轮转：直接取就绪队列队首
        next = ready_queue_dequeue();
    }
    else
    {
        // Counter 优先级调度：遍历就绪队列找 counter 最大的进程
        ListHead *p;
        list_foreach(p, &ready_queue)
        {
            struct task_struct *t = list_entry(p, struct task_struct, linklist);
            if (next == NULL || t->counter > next->counter)
            {
                next = t;
            }
        }

        if (next == NULL || next->counter <= 0)
        {
            // 所有进程 counter 都耗尽了，重新分配时间片
            list_foreach(p, &ready_queue)
            {
                struct task_struct *t = list_entry(p, struct task_struct, linklist);
                t->counter = (t->counter >> 1) + t->priority;
            }
            next = NULL;
            // 重新找 counter 最大的进程
            list_foreach(p, &ready_queue)
            {
                struct task_struct *t = list_entry(p, struct task_struct, linklist);
                if (next == NULL || t->counter > next->counter)
                {
                    next = t;
                }
            }
        }

        list_del(&next->linklist); // 从就绪队列中移除选中的进程
    }

    if (next == NULL)
    {
        panic("schedule: no runnable process!");
    }

    next->state = RUNNING;
    next->run_count++;
    current = next;
}
