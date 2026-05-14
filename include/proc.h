/*
 * proc.h - 进程管理相关的定义和函数声明
 *
 * 包含进程控制块（PCB）的定义、进程状态枚举、全局变量以及与进程调度相关的函数声明。
 *
 * 进程控制块（PCB）包含了每个进程的基本信息，如寄存器状态、内核栈、进程ID、名称、状态以及调度相关的信息。
 * 就绪队列用于管理所有处于就绪状态的进程，调度器会从中选择一个进程进行运行。
 *
 * 该文件还提供了创建内核线程的函数声明，允许用户创建新的内核线程并将其加入到调度系统中。
 */
#ifndef _PROC_H
#define _PROC_H

#include "common.h"
#include "adt/list.h"
#include "x86.h"

/*  ----常量----  */
#define MAX_TASKS 16      // 最大进程数
#define KSTACKSIZE 1024   // 每个进程的内核栈大小
#define MAX_PRIORITY 5    // 最大优先级(数字越小优先级越高)
#define DEFAULT_COUNTER 4 // 时间片大小

/*  ----进程状态----  */
enum proc_state
{
    UNUSED = 0, // 未使用（槽位空闲）
    RUNNABLE,   // 就绪态
    RUNNING,    // 运行态
    BLOCKED,    // 阻塞态
    STOPED      // 终止态
};

/*  ----PCB进程控制块----  */
struct task_struct
{
    TrapFrame *tf;           // 进程的TrapFrame指针，保存进程的寄存器状态,指向内核栈上的寄存器快照位置
    char kstack[KSTACKSIZE]; // 内核栈(1024字节)
    pid_t pid;               // 进程ID
    char name[20];           // 进程名称
    enum proc_state state;   // 进程当前状态
    ListHead linklist;       // 链表节点，用于将进程加入到就绪队列或其他队列中(就绪和阻塞)

    /* 调度相关 */
    int priority;  // 进程优先级，数值越小优先级越高（静态）
    int counter;   // 进程剩余时间片
    int run_count; // 进程被调度总次数
};

/* ----全局变量---- */
extern struct task_struct task[MAX_TASKS]; // 所有进程的 PCB 数组
extern struct task_struct *current;        // 当前正在运行的进程
extern ListHead ready_queue;               // 就绪队列头

/* ----函数声明---- */
void ready_queue_init();                                                   // 初始化就绪队列
void ready_queue_enqueue(struct task_struct *p);                           // 将进程加入就绪队列
struct task_struct *ready_queue_dequeue();                                 // 从就绪队列中取出一个进程
struct task_struct *kthread_create(void (*entry)(void), const char *name); // 创建内核线程

#endif /* _PROC_H */