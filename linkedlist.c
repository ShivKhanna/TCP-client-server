#include "linkedlist.h"

client_node *add_client(char *handle, uint32_t socket) {
    client_node *new_node;
    client_node *cur = head;

    if ((new_node = malloc(sizeof(new_node))) == NULL) {
        perror("malloc");
        return NULL;
    } 
    
    if ((new_node->handle = malloc(strlen(handle) + 1)) == NULL) {
        perror("malloc");
        return NULL;
    }

    new_node->handle = strcpy(new_node->handle, handle);
    new_node->socket = socket;
    new_node->next = NULL;
    
    if (head == NULL) {
        head = new_node; 
    } else {
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = new_node;
    }
    return new_node;
}

int remove_socket(uint32_t socket) {
    client_node *cur = head;
    client_node *prev;

    //Check if the head is the node
    if (cur != NULL && cur->socket == socket) {
        head = cur->next;
        free(cur->handle);
        free(cur);
        return 1;
    }

    //Loop through linkedlist till socket is found or we hit the end
    while (cur != NULL && cur->socket != socket) {
        prev = cur;
        cur = cur->next;
    }
    
    //Handle not found
    if (cur == NULL) {
        return 0;
    }
    
    //Unlink node from linked list
    prev->next = cur->next;
    free(cur->handle);
    free(cur); //Free memory
    return 1;
}

int remove_handle(char *handle) {
    client_node *cur = head;
    client_node *prev;

    //Check if the head is the node
    if (cur != NULL && !strcmp(cur->handle, handle)) {
        head = cur->next;
        free(cur->handle);
        free(cur);
        return 1;
    }

    //Loop through linkedlist till handle is found or we hit the end
    while (cur != NULL && strcmp(cur->handle, handle)) {
        prev = cur;
        cur = cur->next;
    }
    
    //Handle not found
    if (cur == NULL) {
        return 0;
    }
    
    //Unlink node from linked list
    prev->next = cur->next;
    free(cur->handle);
    free(cur); //Free memory
    return 1;
}

client_node *search_handle(char *handle) {
    client_node* cur = head;

    while(cur != NULL) {
        if (!strcmp(cur->handle, handle)) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

client_node *search_socket(uint32_t socket) {
    client_node* cur = head;

    while(cur != NULL) {
        if (socket == cur->socket) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int free_list() {
    client_node *cur = head;
    client_node *prev;

    while(cur != NULL) {
        prev = cur;
        cur = cur->next;
        free(prev->handle);
        free(prev);    
    }
    head = NULL;
    return 1;
}

client_node *get_head() {
    return head;
}