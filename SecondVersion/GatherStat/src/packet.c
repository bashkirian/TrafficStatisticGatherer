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

int isNumber(char *str) {
   while (*str) {
      if(!isdigit(*str))
         return 0;
      str++;
   }
   return 1;
}

int isIpAdress(char *ip) {
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ip, &(sa.sin_addr));
  return result != 0;
}

// compare 2 ip adresses for equality
bool compareIps(char *ip_lhs, char *ip_rhs) {
   char *ptr1, *ptr2;
   ptr1 = strtok(ip_lhs, ".");
   ptr2 = strtok(ip_rhs, ".");
   while(ptr1 != NULL) {
     if (atoi(ptr1) != atoi(ptr2))
       return 0;
     ptr1 = strtok(ip_lhs, ".");
     ptr2 = strtok(ip_rhs, ".");
   }
   return 1;
}

// check if socket is created successfully
void checkSocket(int socket) {
  if (socket < 0) {
    perror("Socket creation error");
    printf("Error code: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

// check if packet meets required parameters
bool isValidPacket(unsigned char *buffer, ssize_t size, struct Parameters *params) {
  size_t ethernet_header_length = sizeof(struct ethhdr);
  struct iphdr *ip_header = (struct iphdr *)(buffer + ethernet_header_length);
  unsigned short ip_header_length = ip_header->ihl * 4;
  struct udphdr *udp_header = (struct udphdr *)(buffer + ip_header_length + ethernet_header_length);

  memset(source, 0, sizeof(*source));
  source->sin_addr.s_addr = ip_header->saddr;
  memset(destination, 0, sizeof(*destination));
  destination->sin_addr.s_addr = ip_header->daddr;

  printf("Packet acquired \n");

  return (compareIps(params->ip_sender, inet_ntoa(source->sin_addr)) &&
          compareIps(params->ip_receiver, inet_ntoa(destination->sin_addr)) &&
          params->port_sender == ntohs(udp_header->source) &&
          params->port_receiver == ntohs(udp_header->dest));
}
