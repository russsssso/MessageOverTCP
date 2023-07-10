#ifndef MESSAGE_H
#define MESSAGE_H

#include <netinet/in.h>

typedef struct msg {
    struct sockaddr_in src;
    unsigned char data_type;
    char content[1500];
    int content_length;
    struct msg *next_message;
} *Message;

typedef struct tpc {
    char topic[51];
    struct msg *first_message, *last_message;
} *Topic;

Topic new_topic(char* topic_name, Message message);
void free_topic(Topic *topic);

#endif
