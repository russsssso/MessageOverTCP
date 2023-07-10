#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <arpa/inet.h>
#include "helpers.h"

void divide_uint16(uint16_t nr, char* result) {
    char nr_string[10];
    int n = sprintf(nr_string, "%u", nr); 
    
    char zero[10] = "0";
    if(n < 3) {
        sprintf(result, "%s", zero);
        n++;
    }

    while (n < 3) {
        strcat(result, zero);
        n++;
    }

    strcat(result, nr_string);

    result[n] = result[n-1];
    result[n-1] = result[n-2];
    result[n-2] = '.';
}

void divide_uint32(uint32_t nr, uint8_t pow, char* result, int sign) {
    char nr_string[20];
    int n = sprintf(nr_string, "%u", nr);

    char zero[10] = "0";
    char sgn[30] = "-";
    

    if(n < pow + 1) {
        sprintf(result, "%s", zero);
        n++;
    }

    while (n < pow + 1) {
        strcat(result, zero);
        n++;
    }

    strcat(result, nr_string);

    for (int i = 0; i < pow; i++) {
        result[n - i] = result[n - i - 1]; 
    }

    if(pow)
        result[n - pow] = '.';

    if(sign) {
        strcat(sgn, result);
        strcpy(result, sgn);
    }
}

void parse_command(int tcp_socket) {
    char in[200];
    int num_bytes = read(STDIN_FILENO, in, sizeof(in));
    if (num_bytes == -1) {
        perror("Error reading from stdin\n");
        exit(-1);
    }

    char command[20];
    sscanf(in, "%s %*s", command);

    if (strcmp(command, "subscribe") != 0 && strcmp(command, "unsubscribe") != 0 && strcmp(command, "exit") != 0) {
        
        perror("Invalid input!\n");
        return;
    }

    Client_Message msg;
    memset (&msg, 0, sizeof(msg));

    if (strcmp(command, "exit") == 0) {
        msg.type = 0;
        send(tcp_socket, &msg, sizeof(msg), 0);
        exit(0);
    } else if (strcmp(command, "unsubscribe") == 0) {
        msg.type = 1;

        char topic[51];
        memset(topic, 0, 51);
        sscanf(in, "%*s %s", topic);
        memcpy(msg.data, topic, 50);
        
        send(tcp_socket, &msg, sizeof(msg), 0);

        printf("Unsubscribed from topic.\n");
    } else {
        msg.type = 2;

        char topic[51];
        char sf[2];
        memset(sf, 0, 2);
        memset(topic, 0, 51);
        sscanf(in, "%*s %s %s", topic, sf);
        
        memcpy(msg.data, sf, 1);
        memcpy(msg.data + 1, topic, 50);

        if(send(tcp_socket, &msg, sizeof(msg), 0) <= 0) {
            perror("ERROR SENDING");
        }
        printf("Subscribed to topic.\n");
    }

}

void print_message(int tcp_socket) {
    Protocol prt;
    recv(tcp_socket, &prt, sizeof(prt), 0);

    char addr[INET_ADDRSTRLEN];
    memset(addr, 0 , INET_ADDRSTRLEN + 1);

    inet_ntop(AF_INET, &prt.src.sin_addr, addr, INET_ADDRSTRLEN);
    uint16_t port = ntohs(prt.src.sin_port);
    
    char topic[51];
    memcpy(topic, prt.topic, 50);
    topic[50] = '\0';
    char content[30];
    memset(content, 0, 30);

    switch (prt.data_type)
    {
    case 0:
        uint32_t nr1;
        memcpy(&nr1, prt.content+1, 4);
        if(prt.content[0] == 0) {
            sprintf(content, "%u",  ntohl(nr1)); 
        } else if (prt.content[0] == 1) {
            sprintf(content, "-%u",  ntohl(nr1));
        }
        printf("%s:%hu - %s - INT - %s\n", addr, port, topic, content);
        break;
    case 1:
        uint16_t nr2;
        memcpy(&nr2, prt.content, 2);
        nr2 = ntohs(nr2);
        
        divide_uint16(nr2, content);

        printf("%s:%hu - %s - SHORT_REAL - %s\n", addr, port, topic, content);
        break;        
    case 2:
        uint32_t nr3;
        memcpy(&nr3, prt.content+1, 4);
        nr3 = ntohl(nr3);
        uint8_t power;
        memcpy(&power, prt.content+5, 1);
        
        divide_uint32(nr3, power, content, prt.content[0]);

        printf("%s:%hu - %s - FLOAT - %s\n", addr, port, topic, content);
        break;
    case 3:
        printf("%s:%hu - %s - STRING - %s\n", addr, port, topic, prt.content);
        break;          
    default:
        close(tcp_socket);
        exit(0);
        break;
    }

}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        perror("Please provide a ID IP PORT!\n");
        exit(-1);
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int tcp_socket = create_tcp_socket();
    int one = 1;
    setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[3]));
    if (inet_pton(AF_INET, argv[2], &serveraddr.sin_addr.s_addr) == 0) {
        perror("Please provide a valid ip for server\n");
        exit(-1);
    }
    
    if (connect(tcp_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) != 0) {
        perror("Error connecting to server");
        exit(1);
    }

    char id[10];
    memset(id, 0, 10);
    sprintf(id, "%s", argv[1]);

    send(tcp_socket, id, 10, 0);

    char response[7];
    char ok[3] = "ok";
    recv(tcp_socket, response, 7, 0);

    if(strcmp(ok, response) != 0) {
        close(tcp_socket);
        perror("Client with same ID already existes.");
        exit(-1);
    }

    int max_fds = 2;
    struct pollfd *fds = calloc(max_fds, sizeof(struct pollfd));
        if (!fds) {
        perror("Error allocating memory for pollfd array\n");
        exit(-1);
    }

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = tcp_socket;
    fds[1].events = POLLIN;

    while (1) {
        if (poll(fds, 2, 10000) <= 0) {
            exit(0);
        }
        
        for (int i = 0; i < 2; i++) {
            //No event or unexpected event 
            if (fds[i].revents == 0 || fds[i].revents != POLLIN)
                continue;        

            if (fds[i].fd == STDIN_FILENO) {
                parse_command(tcp_socket);
            }
 

            else if (fds[i].fd == tcp_socket) {
                print_message(tcp_socket);
            }
        }
        
    }
    
 

}