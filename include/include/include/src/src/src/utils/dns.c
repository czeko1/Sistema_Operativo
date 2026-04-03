#include "dns.h"
#include "../../include/router_types.h"
#include "../logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

static int dns_socket = -1;
static uint32_t upstream_dns_server = 0;
static dns_cache_entry_t dns_cache[DNS_CACHE_SIZE];
static pthread_mutex_t dns_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

int init_dns_server() {
    struct sockaddr_in server_addr;
    
    // Crea socket UDP
    dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dns_socket < 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to create DNS socket");
        return ROUTER_ERROR_NETWORK;
    }
    
    // Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DNS_PORT);
    
    if (bind(dns_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to bind DNS socket");
        close(dns_socket);
        return ROUTER_ERROR_NETWORK;
    }
    
    // Imposta DNS upstream
    upstream_dns_server = inet_addr("8.8.8.8");
    
    // Inizializza cache
    memset(dns_cache, 0, sizeof(dns_cache));
    
    log_message(LOG_LEVEL_INFO, "DNS server initialized on port %d", DNS_PORT);
    return ROUTER_SUCCESS;
}

int handle_dns_request(unsigned char* packet, int packet_len, 
                      struct sockaddr_in* client_addr) {
    
    if (packet_len < sizeof(dns_header_t)) {
        return ROUTER_ERROR_GENERAL;
    }
    
    dns_header_t* header = (dns_header_t*)packet;
    
    // Verifica se è una query
    if ((ntohs(header->flags) & 0x8000) == 0) {
        return process_dns_query(packet, packet_len, client_addr);
    }
    
    return ROUTER_SUCCESS;
}

int process_dns_query(unsigned char* packet, int packet_len, 
                     struct sockaddr_in* client_addr) {
    
    dns_header_t* header = (dns_header_t*)packet;
    unsigned char* query_ptr = packet + sizeof(dns_header_t);
    
    // Estrai nome dominio dalla query
    char domain_name[256];
    int name_len = parse_domain_name(query_ptr, packet, packet_len, domain_name);
    if (name_len <= 0) {
        return ROUTER_ERROR_GENERAL;
    }
    
    // Cerca nella cache
    dns_cache_entry_t* cached_entry = lookup_dns_cache(domain_name);
    if (cached_entry && time(NULL) < cached_entry->expiry_time) {
        // Restituisci risposta dalla cache
        return send_dns_response_from_cache(header, domain_name, cached_entry, client_addr);
    }
    
    // Inoltra al server DNS upstream
    return forward_dns_query(packet, packet_len, client_addr, domain_name);
}

int parse_domain_name(unsigned char* query_ptr, unsigned char* packet, 
                     int packet_len, char* output) {
    int len = 0;
    int pos = 0;
    
    while (*query_ptr != 0 && pos < packet_len - (query_ptr - packet)) {
        int label_len = *query_ptr++;
        if (label_len == 0) break;
        
        if (pos + label_len >= 255) return -1;
        
        memcpy(output + pos, query_ptr, label_len);
        pos += label_len;
        output[pos++] = '.';
        query_ptr += label_len;
    }
    
    if (pos > 0) {
        output[pos - 1] = '\0'; // Rimuovi ultimo punto
    } else {
        output[0] = '\0';
    }
    
    return pos;
}

dns_cache_entry_t* lookup_dns_cache(const char* domain_name) {
    pthread_mutex_lock(&dns_cache_mutex);
    
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].domain[0] != '\0' && 
            strcmp(dns_cache[i].domain, domain_name) == 0) {
            pthread_mutex_unlock(&dns_cache_mutex);
            return &dns_cache[i];
        }
    }
    
    pthread_mutex_unlock(&dns_cache_mutex);
    return NULL;
}

int cache_dns_record(const char* domain_name, uint32_t ip_address, time_t ttl) {
    pthread_mutex_lock(&dns_cache_mutex);
    
    // Trova slot libero o sovrascrivi entry più vecchia
    int oldest_index = 0;
    time_t oldest_time = time(NULL);
    
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].domain[0] == '\0') {
            // Slot libero
            strncpy(dns_cache[i].domain, domain_name, sizeof(dns_cache[i].domain) - 1);
            dns_cache[i].ip_address = ip_address;
            dns_cache[i].expiry_time = time(NULL) + ttl;
            pthread_mutex_unlock(&dns_cache_mutex);
            return 0;
        }
        
        if (dns_cache[i].expiry_time < oldest_time) {
            oldest_time = dns_cache[i].expiry_time;
            oldest_index = i;
        }
    }
    
    // Sovrascrivi entry più vecchia
    strncpy(dns_cache[oldest_index].domain, domain_name, sizeof(dns_cache[oldest_index].domain) - 1);
    dns_cache[oldest_index].ip_address = ip_address;
    dns_cache[oldest_index].expiry_time = time(NULL) + ttl;
    
    pthread_mutex_unlock(&dns_cache_mutex);
    return 0;
}

int forward_dns_query(unsigned char* packet, int packet_len, 
                     struct sockaddr_in* client_addr, const char* domain_name) {
    
    struct sockaddr_in upstream_addr;
    unsigned char response_buffer[512];
    socklen_t addr_len = sizeof(upstream_addr);
    
    // Configura server upstream
    memset(&upstream_addr, 0, sizeof(upstream_addr));
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_addr.s_addr = upstream_dns_server;
    upstream_addr.sin_port = htons(DNS_PORT);
    
    // Invia query upstream
    int sent = sendto(dns_socket, packet, packet_len, 0,
                     (struct sockaddr*)&upstream_addr, sizeof(upstream_addr));
    
    if (sent < 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to forward DNS query for %s", domain_name);
        return ROUTER_ERROR_NETWORK;
    }
    
    // Ricevi risposta
    int received = recvfrom(dns_socket, response_buffer, sizeof(response_buffer), 0,
                           (struct sockaddr*)&upstream_addr, &addr_len);
    
    if (received > 0) {
        // Invia risposta al client
        sendto(dns_socket, response_buffer, received, 0,
               (struct sockaddr*)client_addr, sizeof(*client_addr));
        
        // Cache result (se è una risposta valida)
        cache_dns_response(domain_name, response_buffer, received);
    }
    
    return ROUTER_SUCCESS;
}

int cache_dns_response(const char* domain_name, unsigned char* response, int response_len) {
    // Parse DNS response per estrarre IP address
    if (response_len < sizeof(dns_header_t)) return -1;
    
    dns_header_t* header = (dns_header_t*)response;
    if (ntohs(header->ancount) == 0) return -1;
    
    // Questa parte richiede parsing completo dei record DNS
    // Per semplicità, assumiamo che il primo record A sia quello che ci interessa
    time_t ttl = 3600; // TTL di default
    
    // Estrai IP dal record A (implementazione semplificata)
    uint32_t ip_address = inet_addr("127.0.0.1"); // Placeholder
    
    cache_dns_record(domain_name, ip_address, ttl);
    return 0;
}

int send_dns_response_from_cache(dns_header_t* header, const char* domain_name,
                                dns_cache_entry_t* cache_entry,
                                struct sockaddr_in* client_addr) {
    
    unsigned char response_buffer[512];
    memset(response_buffer, 0, sizeof(response_buffer));
    
    // Copia header e modifica flags per indicare risposta
    dns_header_t* resp_header = (dns_header_t*)response_buffer;
    resp_header->id = header->id;
    resp_header->flags = htons(0x8180); // Response, recursion desired, no error
    resp_header->qdcount = header->qdcount;
    resp_header->ancount = htons(1);
    resp_header->nscount = 0;
    resp_header->arcount = 0;
    
    // Costruisci risposta (implementazione semplificata)
    // In produzione, questo richiederebbe parsing completo del dominio
    
    return sendto(dns_socket, response_buffer, sizeof(dns_header_t) + 32, 0,
                 (struct sockaddr*)client_addr, sizeof(*client_addr));
}

void cleanup_dns_cache() {
    pthread_mutex_lock(&dns_cache_mutex);
    
    time_t now = time(NULL);
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].domain[0] != '\0' && now > dns_cache[i].expiry_time) {
            memset(&dns_cache[i], 0, sizeof(dns_cache_entry_t));
        }
    }
    
    pthread_mutex_unlock(&dns_cache_mutex);
}

void close_dns_server() {
    if (dns_socket >= 0) {
        close(dns_socket);
        dns_socket = -1;
    }
    pthread_mutex_destroy(&dns_cache_mutex);
    log_message(LOG_LEVEL_INFO, "DNS server closed");
}
