#ifndef __NETWORKS_H__
#define __NETWORKS_H__

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

#include "gethostbyname6.h"
#include "linkedlist.h"

#define BUF_SIZE 1024
#define BACKLOG 10
#define MAX_INPUT 1400

#define FLAG_1 1
#define FLAG_2 2
#define FLAG_3 3
#define FLAG_4 4
#define FLAG_5 5

#define FLAG_7 7
#define FLAG_8 8
#define FLAG_9 9
#define FLAG_10 10
#define FLAG_11 11
#define FLAG_12 12
#define FLAG_13 13

#define MAX(a,b) (((a)>(b))?(a):(b))

struct __attribute__ ((__packed__)) chat_header {
    uint16_t length;
    uint8_t flags;
} typedef chat_header;

// for the server side
int tcpServerSetup(int portNumber);
int tcpAccept(int server_socket);

// for the client side
int tcp_client_setup(char * serverName, char * port);

//Send a packet with just a header
ssize_t send_header(int socket, uint8_t flag);

//Create a chat header given a flag and length
chat_header create_header(uint8_t flag, uint16_t len);

//Send a packet and check return value
ssize_t safe_send(int socket, const void *buffer, size_t len, uint8_t flags);

#endif
