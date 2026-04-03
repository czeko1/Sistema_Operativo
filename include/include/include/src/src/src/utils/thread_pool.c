#include "thread_pool.h"
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    void (*function)(void*);
    void* argument;
} thread_task_t;

static thread_pool_t* global_thread_pool = NULL;

thread_pool_t* create_thread_pool(int num_threads) {
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    if (!pool) return NULL;
    
    pool->num_threads = num_threads;
    pool->threads = malloc(num_threads * sizeof(pthread_t));
    if (!pool->threads) {
        free(pool);
        return NULL;
    }
    
    pool->task_queue = create_circular_buffer(1024, sizeof(thread_task_t));
    if (!pool->task_queue) {
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    pool->shutdown = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->notify, NULL);
    
    // Crea i thread worker
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, thread_worker, pool);
    }
    
    global_thread_pool = pool;
    return pool;
}

void* thread_worker(void* arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;
    thread_task_t task;
    
    while (1) {
        pthread_mutex_lock(&pool->mutex);
        
        while (circular_buffer_count(pool->task_queue) == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->notify, &pool->mutex);
        }
        
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }
        
        circular_buffer_pop(pool->task_queue, &task);
        pthread_mutex_unlock(&pool->mutex);
        
        // Esegui il task
        if (task.function) {
            task.function(task.argument);
        }
    }
    
    return NULL;
}

int thread_pool_add_task(thread_pool_t* pool, void (*function)(void*), void* arg) {
    if (!pool || !function) return -1;
    
    thread_task_t task;
    task.function = function;
    task.argument = arg;
    
    return circular_buffer_push(pool->task_queue, &task);
}

void destroy_thread_pool(thread_pool_t* pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);
    
    pthread_cond_broadcast(&pool->notify);
    
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    destroy_circular_buffer(pool->task_queue);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->notify);
    free(pool->threads);
    free(pool);
}

// Funzione helper per uso globale
int submit_task(void (*function)(void*), void* arg) {
    if (!global_thread_pool) return -1;
    return thread_pool_add_task(global_thread_pool, function, arg);
}
