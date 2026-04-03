#include "statistics.h"
#include "../logger.h"
#include <string.h>
#include <time.h>

static router_stats_t global_stats;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t start_time = 0;

int init_statistics() {
    memset(&global_stats, 0, sizeof(global_stats));
    start_time = time(NULL);
    
    pthread_mutex_init(&stats_mutex, NULL);
    
    log_message(LOG_LEVEL_INFO, "Statistics system initialized");
    return ROUTER_SUCCESS;
}

void update_packet_stats(int direction, int protocol, int bytes) {
    pthread_mutex_lock(&stats_mutex);
    
    global_stats.total_packets++;
    global_stats.total_bytes += bytes;
    
    if (direction == DIRECTION_INBOUND) {
        global_stats.inbound_packets++;
        global_stats.inbound_bytes += bytes;
    } else {
        global_stats.outbound_packets++;
        global_stats.outbound_bytes += bytes;
    }
    
    switch (protocol) {
        case IPPROTO_TCP:
            global_stats.tcp_packets++;
            break;
        case IPPROTO_UDP:
            global_stats.udp_packets++;
            break;
        case IPPROTO_ICMP:
            global_stats.icmp_packets++;
            break;
        default:
            global_stats.other_packets++;
            break;
    }
    
    pthread_mutex_unlock(&stats_mutex);
}

void update_nat_stats(int operation) {
    pthread_mutex_lock(&stats_mutex);
    
    switch (operation) {
        case NAT_OPERATION_CREATE:
            global_stats.nat_entries_created++;
            break;
        case NAT_OPERATION_DELETE:
            global_stats.nat_entries_deleted++;
            break;
        case NAT_OPERATION_HIT:
            global_stats.nat_hits++;
            break;
    }
    
    pthread_mutex_unlock(&stats_mutex);
}

void update_dhcp_stats(int operation) {
    pthread_mutex_lock(&stats_mutex);
    
    switch (operation) {
        case DHCP_OPERATION_DISCOVER:
            global_stats.dhcp_discover++;
            break;
        case DHCP_OPERATION_OFFER:
            global_stats.dhcp_offer++;
            break;
        case DHCP_OPERATION_REQUEST:
            global_stats.dhcp_request++;
            break;
        case DHCP_OPERATION_ACK:
            global_stats.dhcp_ack++;
            break;
    }
    
    pthread_mutex_unlock(&stats_mutex);
}

void update_dns_stats(int operation) {
    pthread_mutex_lock(&stats_mutex);
    
    switch (operation) {
        case DNS_OPERATION_QUERY:
            global_stats.dns_queries++;
            break;
        case DNS_OPERATION_RESPONSE:
            global_stats.dns_responses++;
            break;
        case DNS_OPERATION_CACHE_HIT:
            global_stats.dns_cache_hits++;
            break;
        case DNS_OPERATION_CACHE_MISS:
            global_stats.dns_cache_misses++;
            break;
    }
    
    pthread_mutex_unlock(&stats_mutex);
}

router_stats_t get_current_statistics() {
    pthread_mutex_lock(&stats_mutex);
    router_stats_t stats = global_stats;
    pthread_mutex_unlock(&stats_mutex);
    return stats;
}

void print_statistics() {
    router_stats_t stats = get_current_statistics();
    time_t uptime = time(NULL) - start_time;
    
    log_message(LOG_LEVEL_INFO, "=== Router Statistics ===");
    log_message(LOG_LEVEL_INFO, "Uptime: %ld seconds", uptime);
    log_message(LOG_LEVEL_INFO, "Total Packets: %llu", stats.total_packets);
    log_message(LOG_LEVEL_INFO, "Total Bytes: %llu", stats.total_bytes);
    log_message(LOG_LEVEL_INFO, "Inbound: %llu packets (%llu bytes)", 
                stats.inbound_packets, stats.inbound_bytes);
    log_message(LOG_LEVEL_INFO, "Outbound: %llu packets (%llu bytes)", 
                stats.outbound_packets, stats.outbound_bytes);
    log_message(LOG_LEVEL_INFO, "TCP: %llu | UDP: %llu | ICMP: %llu | Other: %llu",
                stats.tcp_packets, stats.udp_packets, stats.icmp_packets, stats.other_packets);
    log_message(LOG_LEVEL_INFO, "NAT Entries Created: %llu | Deleted: %llu | Hits: %llu",
                stats.nat_entries_created, stats.nat_entries_deleted, stats.nat_hits);
    log_message(LOG_LEVEL_INFO, "DHCP: DISCOVER=%llu, OFFER=%llu, REQUEST=%llu, ACK=%llu",
                stats.dhcp_discover, stats.dhcp_offer, stats.dhcp_request, stats.dhcp_ack);
    log_message(LOG_LEVEL_INFO, "DNS: Queries=%llu, Responses=%llu, Cache Hits=%llu, Misses=%llu",
                stats.dns_queries, stats.dns_responses, stats.dns_cache_hits, stats.dns_cache_misses);
}

void reset_statistics() {
    pthread_mutex_lock(&stats_mutex);
    memset(&global_stats, 0, sizeof(global_stats));
    start_time = time(NULL);
    pthread_mutex_unlock(&stats_mutex);
    
    log_message(LOG_LEVEL_INFO, "Statistics reset");
}

void cleanup_statistics() {
    pthread_mutex_destroy(&stats_mutex);
    log_message(LOG_LEVEL_INFO, "Statistics system cleaned up");
}
