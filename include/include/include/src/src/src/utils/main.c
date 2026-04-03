#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>

#include "logger.h"
#include "config.h"
#include "core/packet_handler.h"
#include "core/routing_table.h"
#include "core/statistics.h"
#include "network/nat.h"
#include "network/dhcp.h"
#include "network/dns.h"
#include "network/arp.h"
#include "network/firewall.h"
#include "network/packet_capture.h"
#include "utils/thread_pool.h"

volatile sig_atomic_t running = 1;
static pthread_t stats_thread = 0;
static pthread_t cleanup_thread = 0;

void signal_handler(int sig) {
    log_message(LOG_LEVEL_INFO, "Received signal %d, shutting down...", sig);
    running = 0;
}

void* statistics_thread(void* arg) {
    while (running) {
        sleep(60); // Stampa statistiche ogni minuto
        if (running) {
            print_statistics();
        }
    }
    return NULL;
}

void* cleanup_thread_func(void* arg) {
    while (running) {
        sleep(300); // Cleanup ogni 5 minuti
        if (running) {
            remove_expired_nat_entries();
            cleanup_expired_arp_entries();
            cleanup_expired_leases();
            cleanup_dns_cache();
            log_message(LOG_LEVEL_INFO, "Periodic cleanup completed");
        }
    }
    return NULL;
}

int initialize_system() {
    log_message(LOG_LEVEL_INFO, "Initializing Router OS...");
    
    // Inizializza componenti principali
    if (init_logger("/var/log/router.log") != ROUTER_SUCCESS) {
        fprintf(stderr, "Failed to initialize logger\n");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_statistics() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize statistics");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_routing_table() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize routing table");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_firewall() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize firewall");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_nat_table() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize NAT");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_arp_table() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize ARP");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_dhcp_server() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize DHCP server");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_dns_server() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize DNS server");
        return ROUTER_ERROR_GENERAL;
    }
    
    if (init_packet_capture() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to initialize packet capture");
        return ROUTER_ERROR_GENERAL;
    }
    
    // Aggiungi interfacce per il packet capture
    add_interface_for_capture(LAN_INTERFACE);
    add_interface_for_capture(WAN_INTERFACE);
    
    log_message(LOG_LEVEL_INFO, "All systems initialized successfully");
    return ROUTER_SUCCESS;
}

int start_services() {
    log_message(LOG_LEVEL_INFO, "Starting services...");
    
    // Avvia packet capture
    if (start_packet_capture() != ROUTER_SUCCESS) {
        log_message(LOG_LEVEL_ERROR, "Failed to start packet capture");
        return ROUTER_ERROR_GENERAL;
    }
    
    // Avvia thread per statistiche
    pthread_create(&stats_thread, NULL, statistics_thread, NULL);
    
    // Avvia thread per cleanup periodico
    pthread_create(&cleanup_thread, NULL, cleanup_thread_func, NULL);
    
    log_message(LOG_LEVEL_INFO, "Services started successfully");
    return ROUTER_SUCCESS;
}

void cleanup_system() {
    log_message(LOG_LEVEL_INFO, "Cleaning up system...");
    
    // Ferma servizi
    stop_packet_capture();
    
    // Attendi terminazione thread
    if (stats_thread) {
        pthread_join(stats_thread, NULL);
    }
    if (cleanup_thread) {
        pthread_join(cleanup_thread, NULL);
    }
    
    // Chiudi componenti
    close_dns_server();
    cleanup_packet_capture();
    cleanup_routing_table();
    cleanup_statistics();
    close_logger();
    
    log_message(LOG_LEVEL_INFO, "System cleanup completed");
}

void print_usage(const char* prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -d, --daemon     Run as daemon\n");
    printf("  -v, --verbose    Enable verbose logging\n");
    printf("  -h, --help       Show this help message\n");
    printf("  -c, --config     Configuration file path\n");
}

int main(int argc, char* argv[]) {
    int daemon_mode = 0;
    int verbose = 0;
    char* config_file = NULL;
    
    // Parsing opzioni
    static struct option long_options[] = {
        {"daemon", no_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
