#include "kernel.h"
#include "x86.h"
#include "vm.h"
#include "irq.h"
#include "proc.h"

void proc_a_entry(void) {
    while (TRUE) {
        printk("[ProcA] pid=%d run=%d prio=%d ctr=%d\n",
               current->pid, current->run_count,
               current->priority, current->counter);
        wait_intr();
    }
}
void proc_b_entry(void) {
    while (TRUE) {
        printk("[ProcB] pid=%d run=%d prio=%d ctr=%d\n",
               current->pid, current->run_count,
               current->priority, current->counter);
        wait_intr();
    }
}
void proc_c_entry(void) {
    while (TRUE) {
        printk("[ProcC] pid=%d run=%d prio=%d ctr=%d\n",
               current->pid, current->run_count,
               current->priority, current->counter);
        wait_intr();
    }
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

    // 创建不同优先级的进程（priority 越小优先级越高）
    kthread_create(proc_a_entry, "ProcA", 1);  // 最高优先级（counter=1，短时间片）
    kthread_create(proc_b_entry, "ProcB", 3);  // 中等优先级（counter=3）
    kthread_create(proc_c_entry, "ProcC", 5);  // 最低优先级（counter=5，长时间片）

    printk("=== Counter Priority Scheduling (sched_algo=%d) ===\n", sched_algo);

    // 打印进程状态表
    int i;
    printk("%s", " PID  Name      State     Priority  Counter\n");
    for (i = 0; i < MAX_TASKS; i++) {
        struct task_struct *t = &task[i];
        if (t->state != UNUSED) {
            printk("  %d   %s       %d         %d         %d\n",
                   t->pid, t->name, t->state, t->priority, t->counter);
        }
    }

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
