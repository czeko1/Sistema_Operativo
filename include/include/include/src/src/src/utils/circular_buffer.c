#include "circular_buffer.h"
#include <stdlib.h>
#include <string.h>

circular_buffer_t* create_circular_buffer(size_t capacity, size_t item_size) {
    circular_buffer_t* cb = malloc(sizeof(circular_buffer_t));
    if (!cb) return NULL;
    
    cb->buffer = malloc(capacity * item_size);
    if (!cb->buffer) {
        free(cb);
        return NULL;
    }
    
    cb->capacity = capacity;
    cb->item_size = item_size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    pthread_mutex_init(&cb->mutex, NULL);
    pthread_cond_init(&cb->not_full, NULL);
    pthread_cond_init(&cb->not_empty, NULL);
    
    return cb;
}

int circular_buffer_push(circular_buffer_t* cb, void* item) {
    pthread_mutex_lock(&cb->mutex);
    
    while (cb->count == cb->capacity) {
        pthread_cond_wait(&cb->not_full, &cb->mutex);
    }
    
    memcpy((char*)cb->buffer + (cb->head * cb->item_size), item, cb->item_size);
    cb->head = (cb->head + 1) % cb->capacity;
    cb->count++;
    
    pthread_cond_signal(&cb->not_empty);
    pthread_mutex_unlock(&cb->mutex);
    
    return 0;
}

int circular_buffer_pop(circular_buffer_t* cb, void* item) {
    pthread_mutex_lock(&cb->mutex);
    
    while (cb->count == 0) {
        pthread_cond_wait(&cb->not_empty, &cb->mutex);
    }
    
    memcpy(item, (char*)cb->buffer + (cb->tail * cb->item_size), cb->item_size);
    cb->tail = (cb->tail + 1) % cb->capacity;
    cb->count--;
    
    pthread_cond_signal(&cb->not_full);
    pthread_mutex_unlock(&cb->mutex);
    
    return 0;
}

size_t circular_buffer_count(circular_buffer_t* cb) {
    pthread_mutex_lock(&cb->mutex);
    size_t count = cb->count;
    pthread_mutex_unlock(&cb->mutex);
    return count;
}

void destroy_circular_buffer(circular_buffer_t* cb) {
    if (cb) {
        pthread_mutex_destroy(&cb->mutex);
        pthread_cond_destroy(&cb->not_full);
        pthread_cond_destroy(&cb->not_empty);
        free(cb->buffer);
        free(cb);
    }
}
