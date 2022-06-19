#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "packet.h"

// queue parameters
#define SERVER_QUEUE_NAME   "/server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 1
#define MAX_MSG_SIZE 20
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 1
#define BUFFER_SIZE 65536

int raw_socket;
extern struct sockaddr_in *source;
extern struct sockaddr_in *destination;

pthread_mutex_t thread_mutex;

struct PacketInfo packet_info;

void handle_signal(int sig);

// receives packet statistics from below function, then sends to another program
void receive_and_send_packets_info() {
    while (true) {
        mqd_t qd_server, qd_client;

        struct mq_attr attr;

        attr.mq_flags = 0;
        attr.mq_maxmsg = MAX_MESSAGES;
        attr.mq_msgsize = MAX_MSG_SIZE;
        attr.mq_curmsgs = 0;

        if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
            perror ("Server: mq_open (server)");
            exit (1);
        }

        char in_buffer [MSG_BUFFER_SIZE];
        if (mq_receive (qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror ("Server: mq_receive");
            exit (1);
        }

        if ((qd_client = mq_open (in_buffer, O_WRONLY)) == 1) {
            perror ("Server: Not able to open client queue");
        }

        if (mq_send (qd_client, (char *) &packet_info, sizeof(packet_info), 1) == -1) {
            perror ("Server: Not able to send message to client");
        }
    }
}

// reads statistics from packets that meet parameters
void* process_packets(void *arg) {
    struct Parameters params = *(struct Parameters *)arg;
    ssize_t bytes_read;
    unsigned char buffer[BUFFER_SIZE];
    struct sockaddr addr;
    socklen_t addr_size = (socklen_t)sizeof(addr);

    source = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    destination = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    check_socket(raw_socket);

    signal(SIGINT, handle_signal);
    packet_info.bytes_amount = 0;
    packet_info.packet_amount = 0;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytes_read = recvfrom(raw_socket, buffer, sizeof(buffer), 0, &addr, &addr_size);
        if (bytes_read < 0) {
            perror("Error receiving data");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        if (ip_header->protocol == 17) {
            if (is_valid(buffer, bytes_read, &params)) {
                pthread_mutex_lock(&thread_mutex);
                packet_info.bytes_amount += bytes_read;
                ++packet_info.packet_amount;
                pthread_mutex_unlock(&thread_mutex);
            }
        }
    }
}

int main(int argc, char *argv[])
{
  struct Parameters params;
  if (argc == 5) {
      params.ip_sender = argv[1];
      params.ip_receiver = argv[2];
      params.port_sender = atoi(argv[3]);
      params.port_receiver = atoi(argv[4]);
  }
  else {
      perror("Error in parameters number, please write 2 ip adresses and 2 ports for sender and receiver");
      printf("Error code: %d\n", errno);
      exit(EXIT_FAILURE);
  }
  pthread_t t1;
  pthread_create(&t1, NULL, &process_packets, &params);
  receive_and_send_packets_info();
  pthread_join(t1, NULL);
  return 0;
}

void handle_signal(int sig)
{
  free(source);
  free(destination);
  // закрытие сокета
  if (close(raw_socket) == 0) {
      exit(EXIT_SUCCESS);
  }
  else {
      perror("An error occurred while closing the socket: ");
      printf("Error code: %d\n", errno);
      exit(EXIT_FAILURE);
  }
}
