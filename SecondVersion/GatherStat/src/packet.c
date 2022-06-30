#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "packet.h"

struct sockaddr_in *source;
struct sockaddr_in *destination;

int isNumber(char *str) 
{
    while (*str) {
        if(!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

uint32_t ipAdressFromString(char *ip) 
{
    struct sockaddr_in sa;
    if(inet_pton(AF_INET, ip, &(sa.sin_addr))) {
        return sa.sin_addr.s_addr;
    }
    else {
        return 0;
    }
}

// check if socket is created successfully
void checkSocket(int socket) 
{
    if (socket < 0) {
        perror("Socket creation error");
        printf("Error code: %d\n", errno);
        _exit(EXIT_FAILURE);
    }
}

// check if packet meets required parameters
bool isValidPacket(unsigned char *buffer, ssize_t size, struct Parameters *params) 
{
    size_t ethernet_header_length = sizeof(struct ethhdr);
    struct iphdr *ip_header = (struct iphdr *)(buffer + ethernet_header_length);
    unsigned short ip_header_length = ip_header->ihl * 4;
    struct udphdr *udp_header = (struct udphdr *)(buffer + ip_header_length + ethernet_header_length);

    memset(source, 0, sizeof(*source));
    source->sin_addr.s_addr = ip_header->saddr;
    memset(destination, 0, sizeof(*destination));
    destination->sin_addr.s_addr = ip_header->daddr;

    return (params->ip_sender == source->sin_addr.s_addr &&
            params->ip_receiver == destination->sin_addr.s_addr &&
            params->port_sender == ntohs(udp_header->source) &&
            params->port_receiver == ntohs(udp_header->dest));
}
