/* 计数信号量的实现 */
#include "sem.h"
#include "proc.h"
#include "x86.h"

void sem_init(struct Semaphore *sem, int value, const char *name) // 信号量初始化，const：不可修改
{
    sem->value = value;
    list_init(&sem->wait_queue); // 初始化等待队列
    int i;
    for (i = 0; i < sizeof(sem->name) - 1 && name[i] != '\0'; i++) // i<sizeof(sem->name)-1:i<16-1:防止溢出,最多copy 15个字符
        sem->name[i] = name[i];
    sem->name[i] = '\0'; // 手动添加 \0 终止符
}

void sem_wait(struct Semaphore *sem) // P 操作
{
    cli(); // 关中断
    sem->value--;
    if (sem->value < 0) // 资源不足，阻塞当前进程 block(s->list);
    {
        list_del(&current->linklist); // 从就绪队列移除
        current->state = BLOCKED;     // 设置为阻塞状态
        list_add_before(&sem->wait_queue, &current->linklist);
        sti();                     // 开中断
        asm volatile("int $0x80"); // 触发软中断 → schedule() 切换进程
    }
    else
    {
        sti(); // 资源足够，继续执行，直接开中断
    }
}

void sem_signal(struct Semaphore *sem) // V 操作
{
    cli(); // 关中断
    sem->value++;
    if (sem->value <= 0) // 有进程在等待，唤醒队列头部的一个 wakeup(s->list);
    {
        if (!list_empty(&sem->wait_queue))
        {
            struct task_struct *woken = list_entry(
                sem->wait_queue.next, struct task_struct, linklist);
            list_del(&woken->linklist); // 唤醒一个进程
            woken->state = RUNNABLE;    // 设置为就绪状态
            ready_queue_enqueue(woken); // 放入就绪队列
        }
    }
    sti(); // 开中断
}
