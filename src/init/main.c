#include "kernel.h"
#include "x86.h"
#include "vm.h"
#include "irq.h"
#include "proc.h"
#include "sem.h"

/* ── 演示模式：1=进程创建  2=进程切换(RR)  3=进程调度(Counter)  4=进程同步 ── */
#define DEMO 4

/* ── 模块四用到的常量和变量 ── */
#define BUFFER_SIZE 5
struct Semaphore mutex, empty, full;
int buffer[BUFFER_SIZE];
int in = 0, out = 0;
int produced = 0, consumed = 0;

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
    for (i = 0; i < 4; i++)
    {
        sem_wait(&empty);
        sem_wait(&mutex);
        buffer[in] = produced;
        printk("[P%d] produce #%d -> buf[%d]\n",
               current->pid, produced, in);
        in = (in + 1) % BUFFER_SIZE;
        produced++;
        sem_signal(&mutex);
        sem_signal(&full);
    }
    printk("[P%d] producer finished\n", current->pid);
    current->state = STOPED;
    asm volatile("int $0x80");
}

void consumer_entry(void)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        sem_wait(&full);
        sem_wait(&mutex);
        int item = buffer[out];
        printk("[C%d] consume #%d <- buf[%d]\n",
               current->pid, item, out);
        out = (out + 1) % BUFFER_SIZE;
        consumed++;
        sem_signal(&mutex);
        sem_signal(&empty);
    }
    printk("[C%d] consumer finished\n", current->pid);
    current->state = STOPED;
    asm volatile("int $0x80");
}

void os_init(void)
{
    init_seg();
    init_debug();
    init_idt();
    init_i8259();
    printk("%d+%d=%d", 10, 12, 22);
    printk("%s", "The OS is now working!\n");

    ready_queue_init();

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
    sem_init(&mutex, 1, "mutex");
    sem_init(&empty, BUFFER_SIZE, "empty");
    sem_init(&full, 0, "full");

    kthread_create(producer_entry, "Prod1", 1);
    kthread_create(producer_entry, "Prod2", 1);
    kthread_create(consumer_entry, "Cons1", 1);
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
