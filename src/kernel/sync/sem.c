/* 计数信号量的实现 */
#include "sem.h"
#include "proc.h"
#include "x86.h"

void sem_init(struct Semaphore *sem, int value, const char *name)
{
    sem->value = value;
    list_init(&sem->wait_queue);
    int i;
    for (i = 0; i < sizeof(sem->name) - 1 && name[i] != '\0'; i++)
        sem->name[i] = name[i];
    sem->name[i] = '\0';
}

void sem_wait(struct Semaphore *sem) // P 操作
{
    cli();
    sem->value--;
    if (sem->value < 0)
    {
        // 资源不足，阻塞当前进程
        list_del(&current->linklist); // 从就绪队列移除
        current->state = BLOCKED;
        list_add_before(&sem->wait_queue, &current->linklist);
        sti();
        asm volatile("int $0x80"); // 触发软中断 → schedule() 切换进程
    }
    else
    {
        sti();
    }
}

void sem_signal(struct Semaphore *sem) // V 操作
{
    cli();
    sem->value++;
    if (sem->value <= 0)
    {
        // 有进程在等待，唤醒队列头部的一个
        if (!list_empty(&sem->wait_queue))
        {
            struct task_struct *woken = list_entry(
                sem->wait_queue.next, struct task_struct, linklist);
            list_del(&woken->linklist);
            woken->state = RUNNABLE;
            ready_queue_enqueue(woken);
        }
    }
    sti();
}
