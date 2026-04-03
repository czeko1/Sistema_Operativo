#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "circular_buffer.h"

typedef struct {
    pthread_t* threads;
    int num_threads;
    circular_buffer_t* task_queue;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t notify;
} thread_pool_t;

thread_pool_t* create_thread_pool(int num_threads);
void* thread_worker(void* arg);
int thread_pool_add_task(thread_pool_t* pool, void (*function)(void*), void* arg);
void destroy_thread_pool(thread_pool_t* pool);
int submit_task(void (*function)(void*), void* arg);

#endif // THREAD_POOL_H
