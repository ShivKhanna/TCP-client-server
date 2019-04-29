#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct client_node {
    struct client_node *next;
    char *handle;
    uint32_t socket;
} client_node;

client_node *head;

client_node *add_client(char *handle, uint32_t socket);
int remove_socket(uint32_t socket);
int remove_handle(char *handle);
client_node *search_handle(char *handle);
client_node *get_head();
int free_list();

#endif