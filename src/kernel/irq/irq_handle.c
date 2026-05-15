/*
 * Interrupt handler for IRQs
 */
#include "x86.h"
#include "kernel.h"
#include "proc.h"

TrapFrame *irq_handle(TrapFrame *tf) // 处理 IRQ 中断的函数，参数 tf 是中断发生时的 CPU 状态快照
{
	int irq = tf->irq;
	assert(irq >= 0);

	if (irq < 1000)
	{
		if (irq == 0x80)
		{
			// 系统调用：进程主动让出 CPU（信号量阻塞/退出）
			if (current != NULL)
			{
				current->tf = tf; // 保存当前进程的 TrapFrame
			}
			schedule();
			tf = current->tf;
		}
		else
		{
			// 真正的异常
			cli();
			printk("Unexpected exception #%d\n", irq);
			printk(" errorcode %x\n", tf->err);
			printk(" location  %d:%x, esp %x\n", tf->cs, tf->eip, tf);
			panic("unexpected exception");
		}
	}
	else if (irq >= 1000)
	{
		// external interrupt
		if (irq == 1000)
		{						  // IRQ0 时钟中断
			out_byte(0x20, 0x20); // 发送 EOI 给主 PIC
			if (current != NULL)
			{
				current->tf = tf; // 保存当前进程的 TrapFrame

				if (sched_algo == 1)
				{
					current->counter--; // Counter 模式：每 tick 消耗一个时间片
				}
			}

			if (sched_algo == 0)
			{
				// RR 模式：每个时钟 tick 都切换
				schedule();
				tf = current->tf;
				printk("[schedule] -> %s pid=%d run=%d\n",
				       current->name, current->pid, current->run_count);
			}
			else if (current == NULL || current->counter <= 0)
			{
				// Counter 模式：时间片耗尽（或初始调度）才切换
				schedule();
				tf = current->tf;
				printk("[schedule] -> %s pid=%d run=%d prio=%d ctr=%d\n",
				       current->name, current->pid, current->run_count,
				       current->priority, current->counter);
			}
			// Counter 模式下 counter > 0：不切换，继续当前进程
		}
		else if (irq == 1001)
		{						  // IRQ1 键盘中断
			out_byte(0x20, 0x20); // 发送 EOI 给主 PIC
								  // 处理键盘输入（略）
		}
		// 其他外部中断处理（略）
	}
	return tf; // 返回新的 TrapFrame，供中断返回时使用
}
