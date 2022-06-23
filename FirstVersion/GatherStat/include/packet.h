#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED

#include "packet.h"

struct PacketInfo {
    int  bytes_amount;
    int  packet_amount;
};

struct Parameters {
    char *ip_sender;
    char *ip_receiver;
    int  port_sender;
    int  port_receiver;
    char *interface_name;
};

int isNumber(char *str);
int isIpAdress(char *ip);
bool compareIps(char *ip_lhs, char *ip_rhs);
void checkSocket(int socket);
bool isValidPacket(unsigned char *buffer, ssize_t size, struct Parameters *params);

#endif // PACKET_H_INCLUDED
