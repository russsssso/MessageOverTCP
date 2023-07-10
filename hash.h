#ifndef HASH_H
#define HASH_H

#include "message.h"

#define INITIAL_TABLE_SIZE 128

typedef struct table {
    Topic *table;
    int size;
    int nof_topics;
} *HashTable;

HashTable create_table();
void destroy_table();
unsigned int hash(char* topic_name, int size);
void put(HashTable ht, char* topic_name, Message message);
Topic get(HashTable ht, char* topic_name);
void resize(HashTable *ht);

#endif