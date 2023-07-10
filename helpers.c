#include "helpers.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int create_tcp_socket() {
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {
        perror("Error creating TCP socket\n");
        exit(-1);
    }
    return tcp_socket;
}

int create_udp_socket() {
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("Error creating UDP socket\n");
        exit(-1);
    }
    return udp_socket;
}

void set_server_address(struct sockaddr_in *server_addr, int port) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_port = htons(port);
}

void bind_socket(int socket, struct sockaddr_in *addr) {
    if (bind(socket, (struct sockaddr*) addr, sizeof(*addr)) == -1) {
        perror("Error binding socket\n");
        exit(-1);
    }
}

void listen_on_socket(int socket, int backlog) {
    if (listen(socket, backlog) == -1) {
        perror("Error listening on socket\n");
        exit(-1);
    }
}
