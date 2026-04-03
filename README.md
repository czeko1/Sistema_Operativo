# Router OS - Sistema Operativo per Router Embedded

Un sistema operativo completo per router embedded scritto in C, ottimizzato per architetture ARM con basso consumo energetico.

## Caratteristiche

### Funzionalità di Rete
- **NAT Completo**: Supporto TCP, UDP, ICMP con timeout configurabili
- **Port Forwarding**: Mapping porte esterne verso interne
- **DHCP Server**: Assegnazione IP dinamica con gestione lease
- **DNS Server**: Risoluzione nomi con caching
- **ARP Table**: Gestione cache ARP con timeout automatico
- **Firewall**: Sistema di filtraggio pacchetti configurabile

### Performance e Sicurezza
- **Multi-threading**: Processing parallelo con thread pool
- **Packet Capture**: Cattura efficiente pacchetti raw
- **Buffer Circolari**: Code lock-free per alta performance
- **Statistiche**: Monitoraggio in tempo reale del traffico
- **Logging**: Sistema di log completo con livelli

### Architettura
- **Cross-platform**: Compilazione per ARM (toolchain arm-linux-gnueabihf)
- **Modulare**: Design a componenti facilmente estensibile
- **Efficiente**: Ottimizzato per sistemi embedded con risorse limitate

## Requisiti di Sistema

### Hardware
- Processore ARM (ARMv7 o superiore raccomandato)
- Minimo 256MB RAM
- Due interfacce Ethernet (LAN/WAN)
- Storage sufficiente per il sistema operativo

### Software
- Toolchain ARM cross-compilation: `arm-linux-gnueabihf-gcc`
- Sistema operativo host: Linux/Unix
- Librerie: pthread, libc

## Compilazione

### Setup Toolchain (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install gcc-arm-linux-gnueabihf build-essential
