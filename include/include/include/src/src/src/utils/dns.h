#ifndef DNS_H
#define DNS_H

#include "../../include/router_types.h"
#include <netinet/in.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;
#pragma pack(pop)

typedef struct {
    char domain[256];
    uint32_t ip_address;
    time_t expiry_time;
} dns_cache_entry_t;

int init_dns_server();
int handle_dns_request(unsigned char* packet, int packet_len, 
                      struct sockaddr_in* client_addr);
int process_dns_query(unsigned char* packet, int packet_len, 
                     struct sockaddr_in* client_addr);
int parse_domain_name(unsigned char* query_ptr, unsigned char* packet, 
                     int packet_len, char* output);
dns_cache_entry_t* lookup_dns_cache(const char* domain_name);
int cache_dns_record(const char* domain_name, uint32_t ip_address, time_t ttl);
int forward_dns_query(unsigned char* packet, int packet_len, 
                     struct sockaddr_in* client_addr, const char* domain_name);
int cache_dns_response(const char* domain_name, unsigned char* response, int response_len);
int send_dns_response_from_cache(dns_header_t* header, const char* domain_name,
                                dns_cache_entry_t* cache_entry,
                                struct sockaddr_in* client_addr);
void cleanup_dns_cache();
void close_dns_server();

#endif // DNS_H
