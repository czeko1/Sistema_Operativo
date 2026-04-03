#ifndef CONSTANTS_H
#define CONSTANTS_H

// Dimensioni buffer e tabelle
#define MAX_PACKET_SIZE 1500
#define NAT_TABLE_SIZE 4096
#define ARP_TABLE_SIZE 1024
#define DHCP_LEASE_TABLE_SIZE 512
#define DNS_CACHE_SIZE 1024
#define FIREWALL_RULES_MAX 256

// Timeout configurazioni (secondi)
#define ARP_TIMEOUT 300
#define NAT_TIMEOUT_TCP_ESTABLISHED 86400
#define NAT_TIMEOUT_TCP_SYN_SENT 120
#define NAT_TIMEOUT_UDP 300
#define NAT_TIMEOUT_ICMP 30
#define DHCP_LEASE_TIME_DEFAULT 86400
#define DNS_CACHE_TTL 3600

// Porte standard
#define DNS_PORT 53
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define HTTP_PORT 80
#define HTTPS_PORT 443

// Interfacce di rete
#define LAN_INTERFACE "eth0"
#define WAN_INTERFACE "eth1"
#define LOOPBACK_INTERFACE "lo"

// Buffer sizes
#define LOG_BUFFER_SIZE 4096
#define PACKET_BUFFER_SIZE 65536
#define THREAD_POOL_SIZE 8

// Codici errore personalizzati
#define ROUTER_SUCCESS 0
#define ROUTER_ERROR_GENERAL -1
#define ROUTER_ERROR_MEMORY -2
#define ROUTER_ERROR_NETWORK -3
#define ROUTER_ERROR_CONFIG -4

#endif // CONSTANTS_H
