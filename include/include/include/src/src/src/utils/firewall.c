#include "firewall.h"
#include "../../include/router_types.h"
#include "../logger.h"
#include <string.h>
#include <arpa/inet.h>

static firewall_rule_t firewall_rules[FIREWALL_RULES_MAX];
static int rule_count = 0;
static pthread_rwlock_t firewall_lock = PTHREAD_RWLOCK_INITIALIZER;

int init_firewall() {
    memset(firewall_rules, 0, sizeof(firewall_rules));
    rule_count = 0;
    log_message(LOG_LEVEL_INFO, "Firewall initialized");
    return 0;
}

int add_firewall_rule(firewall_action_t action, 
                     const char* src_ip, const char* dst_ip,
                     uint16_t src_port, uint16_t dst_port,
                     uint8_t protocol) {
    
    if (rule_count >= FIREWALL_RULES_MAX) {
        log_message(LOG_LEVEL_ERROR, "Maximum firewall rules reached");
        return -1;
    }
    
    pthread_rwlock_wrlock(&firewall_lock);
    
    firewall_rules[rule_count].action = action;
    firewall_rules[rule_count].src_ip = src_ip ? inet_addr(src_ip) : 0;
    firewall_rules[rule_count].dst_ip = dst_ip ? inet_addr(dst_ip) : 0;
    firewall_rules[rule_count].src_port = src_port;
    firewall_rules[rule_count].dst_port = dst_port;
    firewall_rules[rule_count].protocol = protocol;
    firewall_rules[rule_count].enabled = 1;
    
    rule_count++;
    pthread_rwlock_unlock(&firewall_lock);
    
    log_message(LOG_LEVEL_INFO, "Added firewall rule #%d", rule_count);
    return 0;
}

firewall_action_t check_firewall_policy(packet_info_t* packet) {
    pthread_rwlock_rdlock(&firewall_lock);
    
    // Default policy: ACCEPT
    firewall_action_t default_action = FIREWALL_ACCEPT;
    
    for (int i = 0; i < rule_count; i++) {
        if (!firewall_rules[i].enabled) continue;
        
        // Check IP addresses
        if (firewall_rules[i].src_ip != 0 && 
            firewall_rules[i].src_ip != packet->src_ip) continue;
            
        if (firewall_rules[i].dst_ip != 0 && 
            firewall_rules[i].dst_ip != packet->dst_ip) continue;
        
        // Check ports
        if (firewall_rules[i].src_port != 0 && 
            firewall_rules[i].src_port != packet->src_port) continue;
            
        if (firewall_rules[i].dst_port != 0 && 
            firewall_rules[i].dst_port != packet->dst_port) continue;
        
        // Check protocol
        if (firewall_rules[i].protocol != 0 && 
            firewall_rules[i].protocol != packet->protocol) continue;
        
        // Rule matched
        pthread_rwlock_unlock(&firewall_lock);
        return firewall_rules[i].action;
    }
    
    pthread_rwlock_unlock(&firewall_lock);
    return default_action;
}

int remove_firewall_rule(int rule_id) {
    if (rule_id < 0 || rule_id >= rule_count) {
        return -1;
    }
    
    pthread_rwlock_wrlock(&firewall_lock);
    firewall_rules[rule_id].enabled = 0;
    pthread_rwlock_unlock(&firewall_lock);
    
    log_message(LOG_LEVEL_INFO, "Disabled firewall rule #%d", rule_id);
    return 0;
}

void list_firewall_rules() {
    pthread_rwlock_rdlock(&firewall_lock);
    
    log_message(LOG_LEVEL_INFO, "=== Firewall Rules ===");
    for (int i = 0; i < rule_count; i++) {
        if (firewall_rules[i].enabled) {
            char src_ip_str[16] = "any";
            char dst_ip_str[16] = "any";
            
            if (firewall_rules[i].src_ip != 0) {
                inet_ntop(AF_INET, &firewall_rules[i].src_ip, src_ip_str, 16);
            }
            if (firewall_rules[i].dst_ip != 0) {
                inet_ntop(AF_INET, &firewall_rules[i].dst_ip, dst_ip_str, 16);
            }
            
            log_message(LOG_LEVEL_INFO, "Rule %d: %s %s -> %s %d:%d proto %d",
                       i,
                       firewall_rules[i].action == FIREWALL_DROP ? "DROP" : "ACCEPT",
                       src_ip_str, dst_ip_str,
                       firewall_rules[i].src_port, firewall_rules[i].dst_port,
                       firewall_rules[i].protocol);
        }
    }
    
    pthread_rwlock_unlock(&firewall_lock);
}
