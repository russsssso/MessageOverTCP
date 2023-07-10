#ifndef HANDLE_H
#define HANDLE_H

#include "hash.h"
#include <poll.h>

typedef struct sbs {
    int sf;
    char topic_name[51];
} Subscription;

typedef struct client {
    char connected;
    char id[10];
    int tcp_socket;
    struct sockaddr_in addr;
    int nof_subscriptions;
    Subscription *subscription_list;
    int list_size;
} *TCP_Client;

typedef struct clients_list {
    int nof_clients;
    int size;
    TCP_Client *clients;
} *ClientsList;

void send_to_client(TCP_Client client, Protocol prt);
void realloc_fds(struct pollfd **fds, int *max_fds);
void handle_stdin();
void store_and_send(ClientsList cl, int udp_socket, HashTable ht);
int handle_new_connection(ClientsList cl, struct pollfd **fds, int tcp_socket, int *nfds, int *max_fds);
void update(TCP_Client client, HashTable ht);
void handle_client_request(ClientsList cl, int client_fd, struct pollfd *fds, int *nfds, HashTable ht);

#endif