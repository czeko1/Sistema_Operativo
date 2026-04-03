#ifndef FIREWALL_H
#define FIREWALL_H

#include "../../include/router_types.h"
#include <pthread.h>

typedef enum {
    FIREWALL_ACCEPT = 0,
    FIREWALL_DROP = 1
} firewall_action_t;

typedef struct {
    firewall_action_t action;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    int enabled;
} firewall_rule_t;

typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    uint8_t direction; // INBOUND or OUTBOUND
} packet_info_t;

int init_firewall();
int add_firewall_rule(firewall_action_t action, 
                     const char* src_ip, const char* dst_ip,
                     uint16_t src_port, uint16_t dst_port,
                     uint8_t protocol);
firewall_action_t check_firewall_policy(packet_info_t* packet);
int remove_firewall_rule(int rule_id);
void list_firewall_rules();

#endif // FIREWALL_H
