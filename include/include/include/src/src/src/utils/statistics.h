#ifndef STATISTICS_H
#define STATISTICS_H

#include "../../include/router_types.h"
#include <stdint.h>
#include <pthread.h>

#define DIRECTION_INBOUND 0
#define DIRECTION_OUTBOUND 1

#define NAT_OPERATION_CREATE 0
#define NAT_OPERATION_DELETE 1
#define NAT_OPERATION_HIT 2

#define DHCP_OPERATION_DISCOVER 0
#define DHCP_OPERATION_OFFER 1
#define DHCP_OPERATION_REQUEST 2
#define DHCP_OPERATION_ACK 3

#define DNS_OPERATION_QUERY 0
#define DNS_OPERATION_RESPONSE 1
#define DNS_OPERATION_CACHE_HIT 2
#define DNS_OPERATION_CACHE_MISS 3

typedef struct {
    // Packet counters
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t inbound_packets;
    uint64_t outbound_packets;
    uint64_t inbound_bytes;
    uint64_t outbound_bytes;
    
    // Protocol breakdown
    uint64_t tcp_packets;
    uint64_t udp_packets;
    uint64_t icmp_packets;
    uint64_t other_packets;
    
    // NAT statistics
    uint64_t nat_entries_created;
    uint64_t nat_entries_deleted;
    uint64_t nat_hits;
    
    // DHCP statistics
    uint64_t dhcp_discover;
    uint64_t dhcp_offer;
    uint64_t dhcp_request;
    uint64_t dhcp_ack;
    
    // DNS statistics
    uint64_t dns_queries;
    uint64_t dns_responses;
    uint64_t dns_cache_hits;
    uint64_t dns_cache_misses;
} router_stats_t;

int init_statistics();
void update_packet_stats(int direction, int protocol, int bytes);
void update_nat_stats(int operation);
void update_dhcp_stats(int operation);
void update_dns_stats(int operation);
router_stats_t get_current_statistics();
void print_statistics();
void reset_statistics();
void cleanup_statistics();

#endif // STATISTICS_H
