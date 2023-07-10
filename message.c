#include "message.h"
#include <stdlib.h>
#include <string.h>

Topic new_topic(char* topic_name, Message message) {
    Topic topic = malloc(sizeof(struct tpc));
    topic->first_message = message;
    topic->last_message = message;
    strncpy(topic->topic, topic_name, 51);

    return topic;
}

void free_topic(Topic *topic) {
    if(!topic || !(*topic))
        return;

    Message msg , aux;
    msg = (*topic)->first_message;
    while (msg != NULL) {
        aux = msg;
        msg = msg->next_message;
        free(aux);
    }
    free(*topic);
    *topic = NULL;
}