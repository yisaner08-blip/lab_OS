#ifndef __SEM_H__
#define __SEM_H__

#include "common.h"
#include "adt/list.h"

#define MAX_SEMAPHORES 8

struct Semaphore
{
    int value;           // 当前资源计数
    ListHead wait_queue; // 阻塞等待队列
    char name[16];       // 调试用名称
};

void sem_init(struct Semaphore *sem, int value, const char *name); // 信号量初始化
void sem_wait(struct Semaphore *sem);                              // P 操作
void sem_signal(struct Semaphore *sem);                            // V 操作

#endif
