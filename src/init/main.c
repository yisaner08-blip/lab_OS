#include "kernel.h"
#include "x86.h"
#include "vm.h"
#include "irq.h"
#include "proc.h"
#include "sem.h"

#define BUFFER_SIZE 5

struct Semaphore mutex, empty, full;
int buffer[BUFFER_SIZE];
int in = 0, out = 0;
int produced = 0, consumed = 0;

void producer_entry(void) {
    int i;
    for (i = 0; i < 4; i++) {
        sem_wait(&empty);        // 等空位
        sem_wait(&mutex);        // 进临界区

        buffer[in] = produced;
        printk("[P%d] produce #%d -> buf[%d]\n",
               current->pid, produced, in);
        in = (in + 1) % BUFFER_SIZE;
        produced++;

        sem_signal(&mutex);      // 出临界区
        sem_signal(&full);       // 通知"有货了"
    }
    printk("[P%d] producer finished\n", current->pid);
    current->state = STOPED;
    asm volatile("int $0x80");   // 退出，触发调度
}

void consumer_entry(void) {
    int i;
    for (i = 0; i < 4; i++) {
        sem_wait(&full);         // 等产品
        sem_wait(&mutex);        // 进临界区

        int item = buffer[out];
        printk("[C%d] consume #%d <- buf[%d]\n",
               current->pid, item, out);
        out = (out + 1) % BUFFER_SIZE;
        consumed++;

        sem_signal(&mutex);      // 出临界区
        sem_signal(&empty);      // 通知"有空位了"
    }
    printk("[C%d] consumer finished\n", current->pid);
    current->state = STOPED;
    asm volatile("int $0x80");   // 退出，触发调度
}

void os_init(void)
{
    init_seg();
    init_debug();
    init_idt();
    init_i8259();
    printk("%d+%d=%d", 10, 12, 22);
    printk("%s", "The OS is now working!\n");
    printk("%s", "hello world\n");

    ready_queue_init();

    // 初始化信号量
    sem_init(&mutex, 1, "mutex");
    sem_init(&empty, BUFFER_SIZE, "empty");
    sem_init(&full, 0, "full");

    printk("=== Producer-Consumer (buf=%d) ===\n", BUFFER_SIZE);

    // 2 生产者 + 2 消费者
    kthread_create(producer_entry, "Prod1", 1);
    kthread_create(producer_entry, "Prod2", 1);
    kthread_create(consumer_entry, "Cons1", 1);
    kthread_create(consumer_entry, "Cons2", 1);

    sti();
    while (TRUE) {
        wait_intr();
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
