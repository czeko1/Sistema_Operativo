#include "packet_capture.h"
#include "../logger.h"
#include "../core/packet_handler.h"
#include "../../include/constants.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

static int raw_sockets[MAX_INTERFACES];
static int num_interfaces = 0;
static volatile int capture_running = 0;
static pthread_t capture_threads[MAX_INTERFACES];
static circular_buffer_t* packet_queue = NULL;
static thread_pool_t* processing_pool = NULL;

int init_packet_capture() {
    // Inizializza coda pacchetti
    packet_queue = create_circular_buffer(PACKET_BUFFER_SIZE, sizeof(captured_packet_t));
    if (!packet_queue) {
        log_message(LOG_LEVEL_ERROR, "Failed to create packet queue");
        return ROUTER_ERROR_MEMORY;
    }
    
    // Inizializza thread pool per processing
    processing_pool = create_thread_pool(THREAD_POOL_SIZE);
    if (!processing_pool) {
        log_message(LOG_LEVEL_ERROR, "Failed to create thread pool");
        destroy_circular_buffer(packet_queue);
        return ROUTER_ERROR_GENERAL;
    }
    
    // Inizializza socket array
    memset(raw_sockets, -1, sizeof(raw_sockets));
    num_interfaces = 0;
    
    log_message(LOG_LEVEL_INFO, "Packet capture system initialized");
    return ROUTER_SUCCESS;
}

int add_interface_for_capture(const char* interface_name) {
    if (num_interfaces >= MAX_INTERFACES) {
        log_message(LOG_LEVEL_ERROR, "Maximum interfaces reached");
        return ROUTER_ERROR_GENERAL;
    }
    
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to create raw socket for %s", interface_name);
        return ROUTER_ERROR_NETWORK;
    }
    
    // Bind to specific interface
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex(interface_name);
    sll.sll_protocol = htons(ETH_P_ALL);
    
    if (bind(sock, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
        log_message(LOG_LEVEL_ERROR, "Failed to bind socket to %s", interface_name);
        close(sock);
        return ROUTER_ERROR_NETWORK;
    }
    
    // Setta socket non bloccante
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    raw_sockets[num_interfaces] = sock;
    num_interfaces++;
    
    log_message(LOG_LEVEL_INFO, "Added interface %s for packet capture", interface_name);
    return ROUTER_SUCCESS;
}

void* packet_capture_thread(void* arg) {
    int interface_index = *(int*)arg;
    int sock = raw_sockets[interface_index];
    unsigned char buffer[MAX_PACKET_SIZE];
    captured_packet_t packet;
    
    while (capture_running) {
        int bytes_received = recvfrom(sock, buffer, MAX_PACKET_SIZE, 0, NULL, NULL);
        
        if (bytes_received > 0) {
            // Prepara pacchetto per la coda
            packet.timestamp = time(NULL);
            packet.interface_index = interface_index;
            packet.packet_len = bytes_received;
            
            if (bytes_received <= MAX_PACKET_SIZE) {
                memcpy(packet.data, buffer, bytes_received);
                
                // Aggiungi alla coda
                if (circular_buffer_push(packet_queue, &packet) != 0) {
                    log_message(LOG_LEVEL_WARNING, "Packet queue full, dropping packet");
                }
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log_message(LOG_LEVEL_ERROR, "Error receiving packets on interface %d", interface_index);
        }
        
        usleep(1000); // 1ms delay per ridurre CPU usage
    }
    
    return NULL;
}

void process_packet_task(void* arg) {
    captured_packet_t* packet = (captured_packet_t*)arg;
    
    // Processa il pacchetto attraverso il packet handler
    process_ethernet_packet(packet->data, packet->packet_len);
    
    // Libera memoria se allocata dinamicamente
    free(packet);
}

int start_packet_capture() {
    if (num_interfaces == 0) {
        log_message(LOG_LEVEL_ERROR, "No interfaces configured for capture");
        return ROUTER_ERROR_CONFIG;
    }
    
    capture_running = 1;
    
    // Avvia thread per ogni interfaccia
    for (int i = 0; i < num_interfaces; i++) {
        int* interface_index = malloc(sizeof(int));
        *interface_index = i;
        
        if (pthread_create(&capture_threads[i], NULL, packet_capture_thread, interface_index) != 0) {
            log_message(LOG_LEVEL_ERROR, "Failed to create capture thread for interface %d", i);
            free(interface_index);
            return ROUTER_ERROR_GENERAL;
        }
    }
    
    // Thread per processare pacchetti dalla coda
    pthread_t processing_thread;
    pthread_create(&processing_thread, NULL, packet_processing_thread, NULL);
    
    log_message(LOG_LEVEL_INFO, "Packet capture started on %d interfaces", num_interfaces);
    return ROUTER_SUCCESS;
}

void* packet_processing_thread(void* arg) {
    captured_packet_t* packet;
    
    while (capture_running) {
        packet = malloc(sizeof(captured_packet_t));
        if (!packet) {
            usleep(10000); // 10ms
            continue;
        }
        
        if (circular_buffer_pop(packet_queue, packet) == 0) {
            // Submit al thread pool per processing
            if (submit_task(process_packet_task, packet) != 0) {
                log_message(LOG_LEVEL_WARNING, "Thread pool busy, processing inline");
                process_packet_task(packet);
            }
        } else {
            free(packet);
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}

int stop_packet_capture() {
    capture_running = 0;
    
    // Attendi terminazione thread
    for (int i = 0; i < num_interfaces; i++) {
        if (capture_threads[i] != 0) {
            pthread_join(capture_threads[i], NULL);
        }
    }
    
    // Chiudi sockets
    for (int i = 0; i < num_interfaces; i++) {
        if (raw_sockets[i] >= 0) {
            close(raw_sockets[i]);
            raw_sockets[i] = -1;
        }
    }
    
    num_interfaces = 0;
    
    log_message(LOG_LEVEL_INFO, "Packet capture stopped");
    return ROUTER_SUCCESS;
}

captured_packet_t* get_next_packet(int timeout_ms) {
    captured_packet_t* packet = malloc(sizeof(captured_packet_t));
    if (!packet) return NULL;
    
    // Implementazione con timeout
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
    timeout.tv_sec += timeout_ms / 1000;
    if (timeout.tv_nsec >= 1000000000) {
        timeout.tv_nsec -= 1000000000;
        timeout.tv_sec++;
    }
    
    // Questa parte richiede modifiche alla circular_buffer per supportare timeout
    // Per ora restituiamo direttamente
    if (circular_buffer_pop(packet_queue, packet) == 0) {
        return packet;
    }
    
    free(packet);
    return NULL;
}

void cleanup_packet_capture() {
    stop_packet_capture();
    
    if (processing_pool) {
        destroy_thread_pool(processing_pool);
        processing_pool = NULL;
    }
    
    if (packet_queue) {
        destroy_circular_buffer(packet_queue);
        packet_queue = NULL;
    }
    
    log_message(LOG_LEVEL_INFO, "Packet capture system cleaned up");
}
