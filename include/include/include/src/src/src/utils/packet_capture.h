#ifndef PACKET_CAPTURE_H
#define PACKET_CAPTURE_H

#include "../../include/router_types.h"
#include "../utils/circular_buffer.h"
#include "../utils/thread_pool.h"
#include <pthread.h>

#define MAX_INTERFACES 8

typedef struct {
    time_t timestamp;
    int interface_index;
    int packet_len;
    unsigned char data[MAX_PACKET_SIZE];
} captured_packet_t;

typedef enum {
    INTERFACE_LAN = 0,
    INTERFACE_WAN = 1,
    INTERFACE_LOOPBACK = 2
} interface_type_t;

int init_packet_capture();
int add_interface_for_capture(const char* interface_name);
int start_packet_capture();
int stop_packet_capture();
void* packet_capture_thread(void* arg);
void* packet_processing_thread(void* arg);
captured_packet_t* get_next_packet(int timeout_ms);
void cleanup_packet_capture();

#endif // PACKET_CAPTURE_H
