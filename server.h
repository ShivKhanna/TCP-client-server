#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "networks.h"
#include "linkedlist.h"

void handle_packet(client_node *client);
void parse_packet(chat_header *header, client_node *client, char *buf);		
void assign_handle(chat_header *header, client_node *client, char* buf);
void forward_broadcast(chat_header *header, client_node *client, char* buf);
void forward_message(chat_header *header, client_node *client, char* buf);
void return_error(int socket, char* handle) ;
void list_handles(client_node *client);
void send_num_handles(int socket);
void send_handle(int socket, char *handle);
void client_disconnect(int socket);
int checkArgs(int argc, char *argv[]);
void get_packets(int server_socket);


#endif

