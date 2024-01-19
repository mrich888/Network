#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MIN_THREADS     5
#define DEFAULT_MAX_THREADS     10
#define DEFAULT_QUEUE_CAPACITY  100
#define TIME_INTERVAL 6
/* 扩容和缩容时默认变化的线程数 */
#define DEFAULT_VARY_THREADS    3

enum STATUS_CODE
{
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    ACCESS_INVAILD,
    UNKONWN_ERROR,
};

static void * threadHander(void *arg);
static void * managerHander(void *arg);
static void * threadExitClrResources(threadpool_t *pool);

/* 本质是一个消费者--从任务队列中拿任务 */
static void * threadHander(void * arg)
{
    /* 首先拿到线程池 */
    threadpool_t * pool = (threadpool_t *)arg;
    /* 任务是一直在的，时刻准备抢资源的 */
    while (1)
    {
        /* 进入循环先上锁。会有有资源和没有资源的两种情况：一等二直接拿 */
        pthread_mutex_lock(&(pool->mutexpool));

        while (pool->queueSize == 0 && pool->shutDown == 0)
        {
            /* 等待一个条件变量（生产者发给消费者）并解锁 */
            pthread_cond_wait(&(pool->notEmpty), &(pool->mutexpool));

            if (pool->exitThreads > 0)
            {
                pool->exitThreads--;
                break;
            }
            pthread_mutex_unlock(&(pool->mutexpool));
            threadExitClrResources(pool);
        }
        if (pool->shutDown == 1)
        {
            pthread_mutex_unlock(&(pool->mutexpool));
            threadExitClrResources(pool);
        }
        
        /* 意味着任务队列有任务，开始从线程池中的任务队列中取对应的任务工作 */
        task_t tmpTask = pool->taskQueue[pool->queueFront];//拿到任务
        /* 更新队头，对容量取余达到循环队列的效果 */
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        /* 拿到任务之后任务数减少 */
        (pool->queueSize)--;
        /* 工作完成后解锁 */
        pthread_mutex_unlock(&(pool->mutexpool));
        /* 并发送一个不满的信号给生产者 */
        pthread_cond_signal(&(pool->notFull));

        /* 为了提升性能，加一把专门为维护busyThreadNums属性的锁 */
        pthread_mutex_lock(&(pool->mutexBusy));
        pool->busyThreadNums++;// 有现成抢到资源了就意味着忙碌的线程增加
        pthread_mutex_unlock(&(pool->mutexBusy));

        /* 执行钩子 */
        tmpTask.worker_hander(tmpTask.arg);
        pthread_mutex_lock(&(pool->mutexBusy));
        pool->busyThreadNums--;// 有现成抢到资源了就意味着忙碌的线程增加
        pthread_mutex_unlock(&(pool->mutexBusy));

    }
    
    pthread_exit(NULL);
}
/* 管理者线程 */
static void * managerHander(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    while (1)
    {
        sleep(TIME_INTERVAL);
        /* 上锁 */
        pthread_mutex_lock(&(pool->mutexpool));
        /* 任务队列中任务数 */
        int taskNums = pool->queueSize;
        /* 存活的线程数 */
        int liveThreadNums = pool->liveThreadNums;
        pthread_mutex_unlock(&(pool->mutexpool));

        /* 上锁 */
        pthread_mutex_lock(&(pool->mutexBusy));
        int busyThreadNums = pool->busyThreadNums;
        pthread_mutex_unlock(&(pool->mutexBusy));

        /* 扩容：扩大线程池里面的线程数（上限不要超过maxThreads） */
        /* 自定义：任务队列中任务数 > 存活的线程数 && 存活的线程数 < maxThreads */
        if (taskNums > liveThreadNums && liveThreadNums < pool->maxThreads)
        {
            /* 计数 */
            int count = 0;
            /* 一次扩三个 */
            int condition = pool->maxThreads && count < DEFAULT_VARY_THREADS && liveThreadNums <= pool->maxThreads;
            for (int idx = 0; idx < condition; idx++)
            {
                /* code */
                if (pool->threadIds[idx] == 0)
                {
                    int ret = pthread_create(&(pool->threadIds[idx]), NULL, threadHander, pool);
                    if (ret != 0)
                    {
                        perror("thread create error");
                        /* todo.... */
                    }
                    count++;
                    pool->liveThreadNums++;
                }
            }
            pthread_mutex_unlock(&(pool->mutexpool));
        }

        /* 缩容：缩小线程池里面的线程数（下限不要低于minThreads） */
        /* 忙的线程数 * 2 < 存活的线程数 && 存活的线程数 > minThreas */
        pthread_mutex_lock(&(pool->mutexpool));
        pool->exitThreads = DEFAULT_VARY_THREADS;
        pthread_mutex_unlock(&(pool->mutexpool));

        if ((busyThreadNums << 1) < liveThreadNums && liveThreadNums > pool->minThreads)
        {
            /* 缩容： */
            for (int  idx = 0; idx < pool->exitThreads ; idx++)
            {
                threadExitClrResources(pool);
            }
        }
    }
    pthread_exit(NULL);
}

/* 自动离职并清理工位 */
static void * threadExitClrResources(threadpool_t *pool)
{
    for (int idx = 0; idx < pool->maxThreads; idx++)
    {
        if (pool->threadIds[idx] == pthread_self())
        {
            pool->threadIds[idx];
            break;
        }
        
    }
}


/* 线程池初始化 *//* 正确的以及遇到问题时的解决办法 */
int threadPoolInit(threadpool_t *pool, int minThreads, int maxThreads, int queueCapacity)
{
    /* 判断是否为空 */
    if (pool == NULL)
    {
        return NULL_PTR;
    }
    do
    {
        /* 判断是否合法 */
        if (minThreads < 0 || maxThreads > 0 || minThreads > maxThreads)
        {
            /* code */
            minThreads = DEFAULT_MIN_THREADS;
            maxThreads = DEFAULT_MAX_THREADS;
        }

        /* 初始化,更新线程池属性 */
        {
            pool->minThreads = minThreads;
            pool->maxThreads = maxThreads;
            pool->busyThreadNums = 0;
            pool->exitThreads = 0;
            /* 退出信号 */
            pool->shutDown = 0;
        }
        

        /* 判断容量是否合法 */
        if (queueCapacity < 0)
        {
            queueCapacity = DEFAULT_QUEUE_CAPACITY;
        }

        /* 初始化,更新线程池属性 */
        {
            pool->queueCapacity = queueCapacity;
            pool->taskQueue = (task_t *)malloc(sizeof(task_t) * pool->queueCapacity);
            if (pool->taskQueue)
            {
                perror("malloc error");
                break;
            }
            memset(pool->taskQueue, 0, sizeof(task_t) * pool->queueCapacity);
            pool->queueFront = 0;
            pool->queueRear = 0;
            pool->queueSize = 0;
        }

        /* ⭐ 为线程ID分配堆空间 */
        pool->threadIds = (threadpool_t *)malloc(sizeof(threadpool_t ) * maxThreads);// pool->maxThreads = maxThreads 两个都行
        if (pool->threadIds == NULL)
        {
            perror("malloc error");
            exit(-1);
        }

        /* 清楚脏数据 */
        memset(pool->threadIds, 0, sizeof(threadpool_t ) * maxThreads);
        /* ⭐ 创建线程 空间分配最大的，线程创建最小的 */
        int ret = 0;
        /* 创建管理者线程 */
        ret = thread_create(&(pool->managerThread), NULL, managerHander, pool);
        if (ret != 0)
        {
            perror("create error");
            break;
        }
        
        for (int idx = 0; idx < pool->minThreads; idx++)
        {
            if (pool->threadIds[idx] == 0)
            {
                ret = pthread_create(&(pool->threadIds[idx]), NULL, threadHander, pool);
                if (ret != 0)
                {
                    perror("thread create error");
                    break;
                }
            }    
        }

        /* 此ret是创建线程的返回值 未创建成功 */
        if (ret != 0)
        {
            break;
        }
        /* 存活的线程数等于开辟的线程数 */
        pool->liveThreadNums = minThreads;

        /* 初始化锁资源 */
        pthread_mutex_init(&(pool->mutexpool), NULL);
        pthread_mutex_init(&(pool->mutexBusy), NULL);

        /* 初始化条件变量,成功返回0不成功返回错误码 */
        if (pthread_cond_init(&(pool->notEmpty), NULL) != 0 || pthread_cond_init(&(pool->notFull), NULL) != 0)
        {
            perror("thread cond error");
            break;
        }
  
        return ON_SUCCESS;

    } while (0);

    /*  程序执行到这里一定遇到了问题，回收堆空间 */
    if (pool->taskQueue != NULL) 
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }
    pthread_join(pool->managerThread, NULL);
    
    /* 程序执行到这里一定遇到了问题,回收县城资源 */
    for (int idx = 0; idx < pool->minThreads; idx++)
    {
        if (pool->threadIds[idx] != 0)
        {
            pthread_join(pool->threadIds[idx], NULL);
        }
        
    }
    /* 释放资源 */
    if (pool->threadIds != NULL)
    {
        free(pool->threadIds);
    }

    /* 释放锁 */
    pthread_mutex_destroy(&(pool->mutexpool));
    pthread_mutex_destroy(&(pool->mutexBusy));

    /* 释放条件变量 */
    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));

    return UNKONWN_ERROR;     
    
}


/* 在线程池中添加任务--生产者 */
int threadPoolAddTask(threadpool_t *pool, void * (* worker_hander)(void *), void *arg)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }
    /* 加锁 */
    pthread_mutex_lock(&(pool->mutexpool));
    /* 如果任务队列的size 等于 队列容量时，需要等待一个不满的信号 */
    while (pool->queueSize == pool->queueCapacity && pool->shutDown == 0)
    {
        pthread_cond_wait(&(pool->notFull), &(pool->mutexpool));
    }
    if (pool->shutDown == 1)
    {
        /* code */
    }
    
    /* 程序到这个地方任务队列中一定有任务可以放任务，并且放到队尾 */
    pool->taskQueue[pool->queueRear].worker_hander = worker_hander;
    pool->taskQueue[pool->queueRear].arg = arg;
    /* 队尾后移，用取余的方式达到循环队列的效果 */
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    /* 任务数加一 */
    pool->queueSize++;
    pthread_mutex_unlock(&(pool->mutexpool));

    /* 发信号 */
    pthread_cond_signal(&(pool->notEmpty));   //⭐这个时候消费者接到信号就要抢资源去拿任务队列中的任务

    return ON_SUCCESS;
}

/* 线程池的销毁 */
int threadOPoolDestroy(threadpool_t *pool)
{
    int ret = 0;
    pool->shutDown = 1;
    pthread_cond_broadcast(&(pool->notEmpty));
    return ret;
}