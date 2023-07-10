#include "helpers.h"
#include "hash.h"
#include "handle.h"
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define MAX_PAYLOAD_LENGTH 1551

void send_to_client(TCP_Client client, Protocol prt) {
    int bytes_sent = send(client->tcp_socket, &prt, sizeof(prt), 0);

    if (bytes_sent == -1) {
        perror("Error sending");
        exit(-1);
    }
}

void close_connection(ClientsList cl, int client_fd, struct pollfd *fds, int *nfds) {
    int i = 0;
    for (; i < *nfds; i++) {
        if(fds[i].fd == client_fd)
            break;
    }

    for(; i < *nfds; i++) {
        if(i + 1 != *nfds)
            fds[i] = fds[i + 1];
        else {
            memset(fds + i, 0, sizeof(struct pollfd));
            break;
        }
    }
    close(client_fd);
    (*nfds)--;

    for (i = 0; i < cl->nof_clients; i++) {
        if(cl->clients[i]->tcp_socket == client_fd) {
            printf("Client %s disconnected.\n", cl->clients[i]->id);
            cl->clients[i]->connected = 0;
            cl->clients[i]->tcp_socket = -1;
            memset(&(cl->clients[i]->addr), 0, sizeof(struct sockaddr_in));
            break;
        }
    }
}

void unsubscribe(TCP_Client client, char* topic) {
    int i = 0;
    char tpc[51];
    memcpy(tpc, topic, 50);
    tpc[50] = '\0';
    for (; i < client->nof_subscriptions; i++) {
        if(strcmp(client->subscription_list[i].topic_name, tpc) == 0)
            break;
    }
    
    for (; i < client->nof_subscriptions; i++) {
        if(i+1 != client->nof_subscriptions)
            client->subscription_list[i] = client->subscription_list[i+1];
        else {
            memset(&(client->subscription_list[i]), 0, sizeof(Subscription));
            client->nof_subscriptions--;
        }
    }
}

void subscribe(TCP_Client client, char* data, HashTable ht) {
    int index;
    if (client->subscription_list == NULL) {
        client->subscription_list = calloc(5, sizeof(Subscription));
        client->list_size = 5;
        index = 0;
    } else if (client->nof_subscriptions == client->list_size) {
        index = client->list_size;
        client->list_size *= 2;
        client->subscription_list = realloc(client->subscription_list, client->list_size * sizeof(Subscription));
    } else {
        index = client->nof_subscriptions;
    }

    memcpy(client->subscription_list[index].topic_name, data + 1, 50);

    if(data[0] == '0') {
        client->subscription_list[index].sf = -1;
    } else {
        int sf = 0;

        Topic tpc = get(ht, client->subscription_list[index].topic_name);
        Message msg = NULL;
        if(tpc)
            msg = tpc->first_message;

        while(msg != NULL) {
            sf++;
            msg = msg->next_message;
        }

        client->subscription_list[index].sf = sf;
    }

    client->nof_subscriptions++;
}

/*
*   Realloc the pollfd array when a new client wants to connect
*   but pollfd array is full.
*/
void realloc_fds(struct pollfd **fds, int *max_fds) {
    *max_fds *= 2;
    *fds = realloc(*fds, *max_fds * sizeof(struct pollfd));
    if (!*fds) {
        perror("Error allocating memory for pollfd array\n");
        exit(-1);
    }
}

void handle_stdin(ClientsList cl) {
    char in[50];
    int num_bytes = read(STDIN_FILENO, in, sizeof(in));
    
    if (num_bytes == -1) {
        perror("Error reading from stdin\n");
        exit(-1);
    }
    
    if (num_bytes > 0) {
        in[num_bytes-1] = '\0';
        if (strcmp(in, "exit") == 0) {
            for (int i = 0; i < cl->nof_clients; i++) {
                if(cl->clients[i]->connected) {
                    Protocol prt;
                    memset(&prt, 0, sizeof(prt));
                    prt.data_type = 4;
                    send(cl->clients[i]->tcp_socket, &prt, sizeof(prt), 0);
                    close(cl->clients[i]->tcp_socket);
                }
            }            
            exit(0);
        }
    }
}

void store_and_send(ClientsList cl, int udp_socket, HashTable ht) {
    char buffer[MAX_PAYLOAD_LENGTH];

    Message new_message = malloc(sizeof(struct msg));
    memset(new_message, 0, sizeof(*new_message));
    socklen_t len = sizeof(new_message->src);

    int n = recvfrom(udp_socket, buffer, MAX_PAYLOAD_LENGTH, 0, (struct sockaddr *) &(new_message->src), &len);

    if (n <= 0) {
        perror("Error reading datagram\n");
        exit(-1);
    }

    int content_length = n - 51;

    char topic[51];
    topic[50] = '\0';

    memcpy(topic, buffer, 50);
    memcpy(&new_message->data_type, buffer+50, 1);
    memcpy(new_message->content, buffer + 51, content_length);
    new_message->content_length = content_length;
    new_message->next_message = NULL;

    put(ht, topic, new_message);

    /*    The server sends a buffer of the following format: 
    update:checksum:<IP_CLIENT_UDP>:<PORT_CLIENT_UDP>:<topic on 50 bytes>:<data_type on char>:<content length on int>
    */ 

    Protocol prt;
    memset(&prt, 0, sizeof(prt));
    memcpy(prt.topic, topic, 50);    
    memcpy(&prt.data_type, &new_message->data_type, 1);
    memcpy(prt.content, new_message->content, content_length);
    memcpy(&prt.src, &new_message->src, sizeof(struct sockaddr_in));

    for (int c = 0; c < cl->nof_clients; c++) {
        TCP_Client client = cl->clients[c];
        for (int i = 0; i < client->nof_subscriptions; i++) {
            if(strcmp(client->subscription_list[i].topic_name, topic) == 0 && client->connected) {  
                send_to_client(client, prt);
                if(client->subscription_list[i].sf != -1)
                    client->subscription_list[i].sf++;
                break;
            }
        }
    }
}


int is_client_connected(ClientsList cl, char *id) {
    for (int i = 0; i < cl->nof_clients; i++) {
        if (strcmp(cl->clients[i]->id, id) == 0 && cl->clients[i]->connected == 1)
            return -2;
        else if (strcmp(cl->clients[i]->id, id) == 0 && cl->clients[i]->connected == 0)
            return i;
    }
    return -1;
}

int handle_new_connection(ClientsList cl, struct pollfd **fds, 
                            int tcp_socket, int *nfds, int *max_fds) {
    if (*nfds == *max_fds)
        realloc_fds(fds, max_fds);

    if(cl->nof_clients == cl->size) {
        cl->size *= 2;
        cl->clients = realloc(cl->clients, cl->size * sizeof(TCP_Client));
    }

    TCP_Client new_client = malloc(sizeof(struct client));
    memset(new_client, 0, sizeof(*new_client));
    socklen_t addr_len = sizeof(new_client->addr); 

    int new_fd = accept(tcp_socket, (struct sockaddr*)&(new_client->addr), &addr_len);
    int one = 1;
    setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    char id[10];
    int n = recv(new_fd, id, 10, 0);

    if(n == -1) {
        perror("Error establishing connection with TCP Client");
        exit(-1);
    }

    int index = is_client_connected(cl, id);

    if(index == -2) {
        printf("Client %s already connected.\n", id);
        char nok[7] = "not ok";
        send(new_fd, nok, 7, 0);
        free(new_client);
        close(new_fd);
        return -1;
    }
    
    char ok[3] = "ok";
    send(new_fd, ok, 3, 0);


    
    (*fds)[*nfds].fd = new_fd;
    (*fds)[*nfds].events = POLLIN;
    (*nfds)++;


    char addr[30];
    inet_ntop(AF_INET, &(new_client->addr.sin_addr), addr, 30);
    printf("New client %s connected from %s:%hu.\n", id, addr, new_client->addr.sin_port);

    if (index >= 0) {
        cl->clients[index]->connected = 1;
        cl->clients[index]->tcp_socket = new_fd;
        memcpy(&cl->clients[index]->addr, &new_client->addr, addr_len);
        free(new_client);
        return index;
    }

    new_client->connected = 1;
    memcpy(new_client->id, id, 10);
    new_client->tcp_socket = new_fd;
    cl->clients[cl->nof_clients++] = new_client;
    return -1;
}

void update(TCP_Client client, HashTable ht) {
    for (int i = 0; i < client->nof_subscriptions; i++) {
        if (client->subscription_list[i].sf != -1) {
            int sf = client->subscription_list[i].sf;
            Topic tpc = get(ht, client->subscription_list[i].topic_name);
            if(!tpc)
                continue;
            Message msg = tpc->first_message;
            Protocol prt;
            memset(&prt, 0, sizeof(prt));
            memcpy(prt.topic, tpc->topic, 50); 

            int k = 0;
            while(k++ < sf) {
                msg = msg->next_message;
            }

            while (msg != NULL) {
                memcpy(&prt.src, &msg->src, sizeof(struct sockaddr_in));
                memcpy(&prt.data_type, &msg->data_type, 1);
                memcpy(prt.content, msg->content, msg->content_length);
                send_to_client(client, prt);
                client->subscription_list[i].sf++;
                msg = msg->next_message;
            }
        }
    }
    
}

void handle_client_request(ClientsList cl, int client_fd, struct pollfd *fds, int *nfds, HashTable ht) {
    Client_Message msg;
    memset(&msg, 0, sizeof(msg));
    recv(client_fd, &msg, sizeof(msg), 0);
    if (msg.type == 0) {
        close_connection(cl, client_fd, fds, nfds);
    } else if (msg.type == 1) {
        for (int i = 0; i < cl->nof_clients; i++){
            if(cl->clients[i]->tcp_socket == client_fd)
                unsubscribe(cl->clients[i], msg.data);
        }
    } else if (msg.type == 2) {
        for (int i = 0; i < cl->nof_clients; i++){
            if(cl->clients[i]->tcp_socket == client_fd)
                subscribe(cl->clients[i], msg.data, ht);
        }
    }
}
