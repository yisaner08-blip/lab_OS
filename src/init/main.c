#include "kernel.h"
#include "x86.h"
#include "vm.h"
#include "irq.h"
#include "proc.h"

void proc_a_entry(void) {
    while (TRUE) wait_intr();
}
void proc_b_entry(void) {
    while (TRUE) wait_intr();
}
void proc_c_entry(void) {
    while (TRUE) wait_intr();
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
    kthread_create(proc_a_entry, "ProcA");
    kthread_create(proc_b_entry, "ProcB");
    kthread_create(proc_c_entry, "ProcC");

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
