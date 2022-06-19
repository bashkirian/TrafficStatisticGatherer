#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

// check if socket is created successfully
check_socket(int socket) {
  if (socket < 0) {
    perror("Socket creation error");
    printf("Error code: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

// check if packet meets required parameters
bool is_valid(unsigned char *buffer, ssize_t size, struct Parameters *params) {
  size_t ethernet_header_length = sizeof(struct ethhdr);
  struct iphdr *ip_header = (struct iphdr *)(buffer + ethernet_header_length);
  unsigned short ip_header_length = ip_header->ihl * 4;
  struct udphdr *udp_header = (struct udphdr *)(buffer + ip_header_length + ethernet_header_length);

  memset(source, 0, sizeof(*source));
  source->sin_addr.s_addr = ip_header->saddr;

  memset(destination, 0, sizeof(*destination));
  destination->sin_addr.s_addr = ip_header->daddr;

  return ((strcmp(params->ip_sender, inet_ntoa(source->sin_addr)) == 0) &&
          (strcmp(params->ip_receiver, inet_ntoa(destination->sin_addr)) == 0) &&
          params->port_sender == ntohs(udp_header->source) &&
          params->port_receiver == ntohs(udp_header->dest));
}
