#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include "../../include/router_types.h"
#include "../network/packet_capture.h"
#include <pthread.h>

#define MAX_ROUTES 256

typedef struct {
    uint32_t destination;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t network;
    interface_type_t interface;
    int metric;
    int valid;
} routing_entry_t;

int init_routing_table();
int add_route(const char* destination, const char* netmask, 
              const char* gateway, interface_type_t interface, int metric);
interface_type_t find_route(uint32_t destination_ip);
int delete_route(const char* destination, const char* netmask);
void print_routing_table();
void cleanup_routing_table();

#endif // ROUTING_TABLE_H
