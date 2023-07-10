#include "helpers.h"
#include "handle.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/tcp.h>


#define LOCALHOST "127.0.0.1"
#define BACKLOG 64

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    
    if (argc == 1) {
        perror("Please provide a PORT!\n");
        exit(-1);
    }

    int tcp_socket = create_tcp_socket();
    int udp_socket = create_udp_socket();
    struct sockaddr_in server_addr;
    set_server_address(&server_addr, atoi(argv[1]));

    int one = 1;
    setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(udp_socket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    bind_socket(tcp_socket, &server_addr);
    bind_socket(udp_socket, &server_addr);

    listen_on_socket(tcp_socket, BACKLOG);

    int max_fds = 3; 
    struct pollfd *fds = calloc(max_fds, sizeof(struct pollfd));
    if (!fds) {
        perror("Error allocating memory for pollfd array\n");
        exit(-1);
    }

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = tcp_socket;
    fds[1].events = POLLIN;
    fds[2].fd = udp_socket;
    fds[2].events = POLLIN;

    int nfds = 3; 

    HashTable ht = create_table();
    ClientsList cl = malloc(sizeof(struct clients_list));
    cl->nof_clients = 0;
    cl->size = 128;
    cl->clients = calloc(cl->size, sizeof(TCP_Client));

    while (1) {
        if (poll(fds, nfds, 20000) <= 0) {
            perror("Error polling sockets\n ");
            exit(-1);
        }

        for (int i = 0; i < nfds; i++) {
            
            //No event or unexpected event 
            if (fds[i].revents == 0 || fds[i].revents != POLLIN)
                continue;

            if (fds[i].fd == STDIN_FILENO)
                handle_stdin();

            else if (fds[i].fd == udp_socket) {
                store_and_send(cl, udp_socket, ht);
            }

            else if (fds[i].fd == tcp_socket) {
                int old_client = handle_new_connection(cl, &fds, tcp_socket, &nfds, &max_fds);
                if (old_client != -1) {
                    update(cl->clients[old_client], ht);
                }
            }

            else {
                handle_client_request(cl, fds[i].fd, fds, &nfds, ht);
            }
        }


    }
    

    return 0;
}