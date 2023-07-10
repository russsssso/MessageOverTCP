#include "hash.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


HashTable create_table() {
    HashTable ht = (HashTable) malloc(sizeof(struct table));
    if (ht == NULL) {
        perror("Failed to allocate memory for hash table.\n");
        exit(-1);
    }
    ht->size = INITIAL_TABLE_SIZE;
    ht->table = (Topic*) calloc(INITIAL_TABLE_SIZE, sizeof(Topic));
    if (ht->table == NULL) {
        perror("Failed to allocate memory for hash table entries.\n");
        exit(-1);
    }
    ht->nof_topics = 0;
    return ht;
}

void destroy_table(HashTable ht) {
    if (ht == NULL) {
        return;
    }
    for (int i = 0; i < ht->size; i++) {
        Topic t = ht->table[i];
        free_topic(&t);
    }
    free(ht->table);
    free(ht);
}

// djb2
unsigned int hash(char* topic_name, int size) {
    unsigned long hash = 5381;
    int c;

    while ((c = *topic_name++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % size;
}

void resize_table(HashTable ht) {
    int new_size = ht->size * 2;
    Topic *new_table = (Topic*) calloc(new_size, sizeof(Topic));
    if (new_table == NULL) {
        perror("Failed to allocate memory for resized hash table entries.\n");
        exit(-1);
    }
    for (int i = 0; i < ht->size; i++) {
        Topic t = ht->table[i];
        if(t != NULL) {
            int new_index = hash(t->topic, new_size);
            while (new_index < new_size) {
                if (new_table[new_index] == NULL) {
                    new_table[new_index] = t;
                    break;
                }
                new_index++;
            }
            perror("Failed to resize UDP message table");
            exit(-1);
        }
    }
    
    free(ht->table);
    ht->table = new_table;
    ht->size = new_size;
}

int insert_at(HashTable ht, char* topic_name, Message message, int index) {
    if(ht->table[index] == NULL) {
        ht->table[index] = new_topic(topic_name, message);
        ht->nof_topics++; 
        return 1;    
    }
    else if(strcmp(ht->table[index]->topic, topic_name) == 0) {
        ht->table[index]->last_message->next_message = message;
        ht->table[index]->last_message = message;
        return 1;
    }
    return 0;
}

void put(HashTable ht, char* topic_name, Message message) {
    float a, b, load_factor;
    a = ht->nof_topics;
    b = ht->size;
    load_factor = a / b;
    if(load_factor >= 0.7) 
        resize_table(ht);

    int index = hash(topic_name, ht->size);

    while(index < ht->size) {
        if(insert_at(ht, topic_name, message, index))
            return;
        index++;
    }

    //Insert failed
    resize_table(ht);
    put(ht, topic_name, message);
}

Topic get(HashTable ht, char* topic_name) {
    int index = hash(topic_name, ht->size);

    while(index < ht->size && ht->table[index] != NULL) {
        if(strcmp(ht->table[index]->topic, topic_name) == 0)
            return ht->table[index];
        index++;
    }

    //Topic not Found;
    return NULL;
}