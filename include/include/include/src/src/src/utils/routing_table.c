#include "routing_table.h"
#include "../logger.h"
#include <string.h>
#include <arpa/inet.h>

static routing_entry_t routing_table[MAX_ROUTES];
static int route_count = 0;
static pthread_rwlock_t routing_lock = PTHREAD_RWLOCK_INITIALIZER;

int init_routing_table() {
    memset(routing_table, 0, sizeof(routing_table));
    route_count = 0;
    
    // Aggiungi rotte di default
    add_route("0.0.0.0", "0.0.0.0", "192.168.1.1", INTERFACE_LAN, 1);
    add_route("192.168.1.0", "255.255.255.0", "0.0.0.0", INTERFACE_LAN, 0);
    
    log_message(LOG_LEVEL_INFO, "Routing table initialized with %d default routes", route_count);
    return ROUTER_SUCCESS;
}

int add_route(const char* destination, const char* netmask, 
              const char* gateway, interface_type_t interface, int metric) {
    
    if (route_count >= MAX_ROUTES) {
        log_message(LOG_LEVEL_ERROR, "Maximum routes reached");
        return ROUTER_ERROR_GENERAL;
    }
    
    pthread_rwlock_wrlock(&routing_lock);
    
    routing_table[route_count].destination = inet_addr(destination);
    routing_table[route_count].netmask = inet_addr(netmask);
    routing_table[route_count].gateway = inet_addr(gateway);
    routing_table[route_count].interface = interface;
    routing_table[route_count].metric = metric;
    routing_table[route_count].valid = 1;
    
    // Calcola network address
    routing_table[route_count].network = 
        routing_table[route_count].destination & routing_table[route_count].netmask;
    
    route_count++;
    pthread_rwlock_unlock(&routing_lock);
    
    log_message(LOG_LEVEL_INFO, "Added route: %s/%s via %s", 
                destination, netmask, gateway);
    return ROUTER_SUCCESS;
}

interface_type_t find_route(uint32_t destination_ip) {
    pthread_rwlock_rdlock(&routing_lock);
    
    interface_type_t best_interface = INTERFACE_WAN; // Default
    uint32_t longest_match = 0;
    int best_metric = 999999;
    
    for (int i = 0; i < route_count; i++) {
        if (!routing_table[i].valid) continue;
        
        // Controlla se l'IP matcha questa route
        if ((destination_ip & routing_table[i].netmask) == routing_table[i].network) {
            // Controlla lunghezza del match (più specifico)
            uint32_t mask_bits = __builtin_popcount(routing_table[i].netmask);
            
            if (mask_bits > longest_match || 
                (mask_bits == longest_match && routing_table[i].metric < best_metric)) {
                longest_match = mask_bits;
                best_metric = routing_table[i].metric;
                best_interface = routing_table[i].interface;
            }
        }
    }
    
    pthread_rwlock_unlock(&routing_lock);
    return best_interface;
}

int delete_route(const char* destination, const char* netmask) {
    uint32_t dest_ip = inet_addr(destination);
    uint32_t netmask_ip = inet_addr(netmask);
    uint32_t network = dest_ip & netmask_ip;
    
    pthread_rwlock_wrlock(&routing_lock);
    
    for (int i = 0; i < route_count; i++) {
        if (routing_table[i].valid && 
            routing_table[i].network == network &&
            routing_table[i].netmask == netmask_ip) {
            
            routing_table[i].valid = 0;
            pthread_rwlock_unlock(&routing_lock);
            
            log_message(LOG_LEVEL_INFO, "Deleted route: %s/%s", destination, netmask);
            return ROUTER_SUCCESS;
        }
    }
    
    pthread_rwlock_unlock(&routing_lock);
    log_message(LOG_LEVEL_WARNING, "Route not found: %s/%s", destination, netmask);
    return ROUTER_ERROR_GENERAL;
}

void print_routing_table() {
    pthread_rwlock_rdlock(&routing_lock);
    
    log_message(LOG_LEVEL_INFO, "=== Routing Table ===");
    log_message(LOG_LEVEL_INFO, "%-18s %-18s %-18s %-10s %-6s", 
                "Destination", "Netmask", "Gateway", "Interface", "Metric");
    
    for (int i = 0; i < route_count; i++) {
        if (routing_table[i].valid) {
            char dest_str[16], mask_str[16], gw_str[16];
            inet_ntop(AF_INET, &routing_table[i].destination, dest_str, 16);
            inet_ntop(AF_INET, &routing_table[i].netmask, mask_str, 16);
            inet_ntop(AF_INET, &routing_table[i].gateway, gw_str, 16);
            
            const char* iface_name = "unknown";
            switch (routing_table[i].interface) {
                case INTERFACE_LAN: iface_name = "LAN"; break;
                case INTERFACE_WAN: iface_name = "WAN"; break;
                case INTERFACE_LOOPBACK: iface_name = "LOOP"; break;
            }
            
            log_message(LOG_LEVEL_INFO, "%-18s %-18s %-18s %-10s %-6d",
                        dest_str, mask_str, gw_str, iface_name, routing_table[i].metric);
        }
    }
    
    pthread_rwlock_unlock(&routing_lock);
}

void cleanup_routing_table() {
    pthread_rwlock_destroy(&routing_lock);
    log_message(LOG_LEVEL_INFO, "Routing table cleaned up");
}
