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
		// exception
		cli();
		printk("Unexpected exception #%d\n", irq);
		printk(" errorcode %x\n", tf->err);
		printk(" location  %d:%x, esp %x\n", tf->cs, tf->eip, tf);
		panic("unexpected exception");
	}
	else if (irq >= 1000)
	{
		// external interrupt
		if (irq == 1000)
		{						  // IRQ0 时钟中断
			out_byte(0x20, 0x20); // 发送 EOI 给主 PIC
			if (current != NULL)
			{
				current->tf = tf; // 保存当前进程的 TrapFrame (保存当前进程的栈快照)
			}
			schedule();		  // 调度：选择下一个进程运行
			tf = current->tf; // 切换到下一个进程的 TrapFrame(获取新进程的栈快照)
			printk("[schedule] -> %s pid=%d run=%d\n",
			       current->name, current->pid, current->run_count);
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
