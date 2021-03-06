#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <net/if.h>
#include "packet.h"

// queue parameters
#define SERVER_QUEUE_NAME "/server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 1
#define MAX_MSG_SIZE 20
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 1
#define BUFFER_SIZE 65536

mqd_t qd_server, qd_client;
struct mq_attr attr;
    
#define MAX_PORT_VALUE 65535

int raw_socket;
extern struct sockaddr_in *source;
extern struct sockaddr_in *destination;

struct Bytes { 
    int bytes_by_packets[1000];
    int packets_read;
};

struct PacketInfo packet_info;
struct Bytes bytes;
pthread_mutex_t thread_mutex;

void handleSignal(int sig);

void sumStatistic() 
{
    pthread_mutex_lock(&thread_mutex);
    for (int i = packet_info.packet_amount; i < bytes.packets_read; ++i) {
        packet_info.packet_amount++;
        packet_info.bytes_amount += bytes.bytes_by_packets[i];
    }
    pthread_mutex_unlock(&thread_mutex);
}

void sendPacketsInfo() 
{
    while (true) {
        if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
            perror ("Server: mq_open (server)");
            _exit(1);
        }

        char in_buffer [MSG_BUFFER_SIZE];

        if (mq_receive(qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror("Server: mq_receive");
            _exit(1);
        }

        if ((qd_client = mq_open (in_buffer, O_WRONLY)) == 1) {
            perror("Server: Not able to open client queue");
            _exit(1);
        }
    
        sumStatistic();
    
        if (mq_send(qd_client, (char *)&packet_info, sizeof(packet_info), 1) == -1) {
            perror ("Server: Not able to send message to client");
        }
    }
}

// receives packet statistics from below function, then sends to another program
void receiveAndSendPacketsInfo() 
{
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    sendPacketsInfo();
}

// reads statistics from packets that meet parameters
void* processPackets(void *arg) 
{
    struct Parameters params = *(struct Parameters *)arg;
    ssize_t packet_bytes;
    unsigned char buffer[BUFFER_SIZE];
    struct sockaddr_ll sll;
    struct ifreq ifr;
    socklen_t addr_size = (socklen_t)sizeof(sll);

    bzero(&sll, sizeof(sll));
    bzero(&ifr, sizeof(ifr));

    source = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    destination = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    checkSocket(raw_socket);

    // getting interface index...
    strncpy((char *)ifr.ifr_name, params.interface_name, IFNAMSIZ);
    if ((ioctl(raw_socket, SIOCGIFINDEX, &ifr)) == -1) {
        perror("Error getting Interface index !\n");
        _exit(EXIT_FAILURE);
    }
    // and binding socket to it...
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_IP);

    if ((bind(raw_socket, (struct sockaddr *)&sll, sizeof(sll))) == -1) {
        perror("Error binding raw socket to interface\n");
        _exit(EXIT_FAILURE);
    }

    signal(SIGINT, handleSignal);
    
    bytes.packets_read = 0;
    packet_info.bytes_amount = 0;
    packet_info.packet_amount = 0;
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        packet_bytes = recvfrom(raw_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&sll, &addr_size);
        if (packet_bytes < 0) {
            perror("Error receiving data");
            printf("Error code: %d\n", errno);
            _exit(EXIT_FAILURE);
        }
        struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        if (ip_header->protocol == 17) {
            if (isValidPacket(buffer, packet_bytes, &params)) {
                bytes.bytes_by_packets[bytes.packets_read] = packet_bytes;
                ++bytes.packets_read;
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    struct Parameters params;
    if (argc == 6) {
        uint32_t ip_sender = ipAdressFromString(argv[1]);
        if (ip_sender == 0) {
            perror("Please write the valid sender ip");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        uint32_t ip_receiver = ipAdressFromString(argv[2]);
        if (ip_receiver == 0) {
            perror("Please write the valid receiver ip");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        if (!(isNumber(argv[3]))) {
            perror("Sender port must be a number!");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        else if (atoi(argv[3]) > MAX_PORT_VALUE) {
            perror("Sender port must be a number from 0 to 65535");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        if (!(isNumber(argv[4]))) {
            perror("Sender port must be a number!");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        else if (atoi(argv[4]) > MAX_PORT_VALUE) {
            perror("Sender port must be a number from 0 to 65535");
            printf("Error code: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        params.ip_sender = ip_sender;
        params.ip_receiver = ip_receiver;
        params.port_sender = atoi(argv[3]);
        params.port_receiver = atoi(argv[4]);
        params.interface_name = argv[5];
    }
    else {
        perror("Error in parameters number: please write 2 ip adresses and 2 ports for sender and receiver, interface name");
        printf("Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    pthread_t packet_processor_thread;
    pthread_create(&packet_processor_thread, NULL, &processPackets, &params);
    receiveAndSendPacketsInfo();
    pthread_join(packet_processor_thread, NULL);
    return 0;
}

void handleSignal(int sig)
{
    free(source);
    free(destination);
    if (close(raw_socket) != 0) {
        perror("An error occurred while closing the socket: ");
        printf("Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
  
    if (mq_close(qd_server) == -1) {
        perror("Server: mq_close");
        _exit(1);
    }
     
    if (mq_unlink(SERVER_QUEUE_NAME) == -1) {
        perror("Server: mq_unlink");
        _exit(1);
    }
  
    exit(EXIT_SUCCESS);
}
