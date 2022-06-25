#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

#define SERVER_QUEUE_NAME   "/server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MSG_SIZE sizeof(unsigned long)
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 1

struct PacketInfo {
    int bytes_amount;
    int packet_amount;
};

int main ()
{
    char client_queue_name [20];
    mqd_t qd_server, qd_client;   // queue descriptors

    // create the client queue for receiving messages from server
    sprintf (client_queue_name, "/client-%d", getpid ());

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_client = mq_open (client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("Client: mq_open (client)");
        exit (1);
    }

    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror ("Client: mq_open (server)");
        exit (1);
    }

    if (mq_send(qd_server, client_queue_name, sizeof(client_queue_name), 0) == -1) {
         fprintf(stderr, "Q name send error: %s\n", strerror(errno));
         exit(1);
    }

    struct PacketInfo received_statistic;
    // receive response from server

    if (mq_receive (qd_client, (char *)&received_statistic, MSG_BUFFER_SIZE, NULL) == -1) {
        perror ("Client: mq_receive");
        exit (1);
    }

    if (mq_close (qd_client) == -1) {
        perror ("Client: mq_close");
        exit (1);
    }

    if (mq_unlink (client_queue_name) == -1) {
        perror ("Client: mq_unlink");
        exit (1);
    }

    printf("Packets read: %d", received_statistic.packet_amount);
    printf("\nBytes read: %d\n", received_statistic.bytes_amount);
    exit (0);
}
