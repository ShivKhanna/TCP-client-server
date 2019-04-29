#ifndef CCLIENT_H
#define CCLIENT_H

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
#include <ctype.h>
#include <errno.h>

#include "networks.h"

#define xstr(a) str(a)
#define str(a) #a
#define MAX_HANDLE_LEN 100
#define MESSAGE_CHUNK 200
void verify_handle();
void check_args(int argc, char * argv[]);
void send_receive_message();
void handle_packet();
void parse_packet(chat_header header, char* packet);
void disconnect();
void recv_handle_count(char *packet);
void recv_handle(char *packet);
void msg_error(char *packet);
void recv_broadcast(char *packet);
void recv_msg(char *packet);
void handle_error();
void handle_input();
void parse_message(char input_buf[MAX_INPUT]);
void split_message(char *message, char **handle_endpoints, int num_handles);
void send_message(char *message, char **handle_endpoints, int num_handles);
void parse_broadcast(char input_buf[MAX_INPUT]);
void send_broadcast(char *message);
#endif
