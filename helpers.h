#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include <netinet/in.h>

typedef struct myprot {
    struct sockaddr_in src;
    char topic[50];
    uint8_t data_type;
    char content[1500]; 
} Protocol;

typedef struct {
    int type;
    char data[100];
} Client_Message;

int create_tcp_socket();
int create_udp_socket();
void set_server_address(struct sockaddr_in *server_addr, int port);
void bind_socket(int socket, struct sockaddr_in *addr);
void listen_on_socket(int socket, int backlog);


#endif