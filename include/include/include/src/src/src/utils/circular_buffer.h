#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stddef.h>

typedef struct {
    void* buffer;
    size_t capacity;
    size_t item_size;
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} circular_buffer_t;

circular_buffer_t* create_circular_buffer(size_t capacity, size_t item_size);
int circular_buffer_push(circular_buffer_t* cb, void* item);
int circular_buffer_pop(circular_buffer_t* cb, void* item);
size_t circular_buffer_count(circular_buffer_t* cb);
void destroy_circular_buffer(circular_buffer_t* cb);

#endif // CIRCULAR_BUFFER_H
