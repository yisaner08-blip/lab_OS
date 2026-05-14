/* 进程管理相关函数的实现 */

#include "proc.h"

struct task_struct task[MAX_TASKS]; // 进程表，包含所有进程的PCB(数组形式)
struct task_struct *current = NULL; // 当前正在运行的进程的指针
ListHead ready_queue;               // 就绪队列，包含所有处于就绪状态的进程
static int next_pid = 0;            // 自增PID，用于分配新的进程ID

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

struct task_struct *kthread_create(void (*entry)(void), const char *name)
{
    struct task_struct *p = NULL;
    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        if (task[i].state == UNUSED) {
            p = &task[i];
            break;
        }
    }
    if (p == NULL) return NULL;

    p->pid = next_pid++;
    int j;
    for (j = 0; j < sizeof(p->name) - 1 && name[j] != '\0'; j++)
        p->name[j] = name[j];
    p->name[j] = '\0';
    p->state = RUNNABLE;
    p->priority = MAX_PRIORITY;
    p->counter = DEFAULT_COUNTER;
    p->run_count = 0;

    // 在内核栈上手工构造假的 TrapFrame
    uint32_t *top = (uint32_t *)(p->kstack + KSTACKSIZE);

    *(--top) = 0x200;              // eflags (IF=1)
    *(--top) = KSEL(SEG_KCODE);    // cs
    *(--top) = (uint32_t)entry;    // eip
    *(--top) = 0;                  // err
    *(--top) = -1;                 // irq = -1 表示非真实中断
    *(--top) = KSEL(SEG_KDATA);    // ds
    *(--top) = KSEL(SEG_KDATA);    // es
    *(--top) = KSEL(SEG_KDATA);    // fs
    *(--top) = KSEL(SEG_KDATA);    // gs
    // pushal 保存的 8 个通用寄存器，全部初始化为 0
    *(--top) = 0;  // eax
    *(--top) = 0;  // ecx
    *(--top) = 0;  // edx
    *(--top) = 0;  // ebx
    *(--top) = 0;  // esp_
    *(--top) = 0;  // ebp
    *(--top) = 0;  // esi
    *(--top) = 0;  // edi

    // dummy_tf_ptr: 其值 = edi 字段的地址，供 do_irq.S 恢复用
    uint32_t dummy_tf_ptr = (uint32_t)top + 4;
    *(--top) = dummy_tf_ptr;

    p->tf = (TrapFrame *)top;

    ready_queue_enqueue(p);
    return p;
}