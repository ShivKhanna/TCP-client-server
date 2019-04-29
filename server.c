#include "server.h"

void handle_packet(client_node *client) {
	char packet[BUF_SIZE];
	char *packet_ptr = packet;
	chat_header header;
	int message_len = 0;
	
	//Get header from our packet
	if ((message_len = recv(client->socket, packet_ptr, sizeof(chat_header), MSG_WAITALL)) < 0) {
		perror("recv call");
		exit(-1);
	}

	/* No message indicates disconnect, remove client's packet*/
	if (message_len == 0) {
		close(client->socket);
		remove_socket(client->socket);
		return;
	}
	//First copy in the header to our header struct
	memcpy(&header, packet_ptr, sizeof(chat_header));

	//Then get the rest of the packet
	packet_ptr += sizeof(chat_header);
	if (ntohs(header.length) > sizeof(chat_header)) {
		if ((message_len = recv(client->socket, packet_ptr, ntohs(header.length) - sizeof(chat_header), MSG_WAITALL)) < 0) {
			perror("recv call");
			exit(-1);
		}
	}
	//Finally parse the packet
	parse_packet(&header, client, packet);
}

void parse_packet(chat_header *header, client_node *client, char *packet) {
	switch (header->flags) {
			case FLAG_1: /* New client wants to connect */
				assign_handle(header, client, packet);
				break;
			case FLAG_4: /* Client wants to broadcast a message to all clients */
				forward_broadcast(header, client, packet);	
				break;
			case FLAG_5: /* Client wants to send a message to a specific list of clients*/
				forward_message(header, client, packet);
				break;
			case FLAG_8: /* Client wants to disconnect */
				client_disconnect(client->socket);
				break;
			case FLAG_10: /* Client wants a list of all clients */
				list_handles(client);
				break;
			default: 
				printf("Unknown packet\n");
				break;
	}
}

void assign_handle(chat_header *header, client_node *client, char* packet) {
	uint8_t handle_len;
	uint8_t *handle_len_ptr = &handle_len;
	char handle[BUF_SIZE];
	//get pointer to handle and handle length
	memcpy(handle_len_ptr, packet + sizeof(chat_header), sizeof(uint8_t));
	
	//copy handle into handle packetf
	memcpy(handle, packet + sizeof(chat_header) + sizeof(uint8_t), handle_len);
	handle[handle_len] = '\0';
	/* Check if handle exists */
	if(search_handle(handle) == NULL) {
		//didn't find it, we can assign the handle to this client
		if ((client->handle = realloc(client->handle, strlen(handle) + 1)) == NULL) {
            perror("realloc");
        }
        memcpy(client->handle, handle, strlen(handle) + 1);
		send_header(client->socket, FLAG_2);
	} else {
		//we found the handle in our list, send error pkt and remove client
		send_header(client->socket, FLAG_3); //Send error packet to client that handle was taken
		close(client->socket);
		remove_socket(client->socket);
	}
}

void forward_broadcast(chat_header *header, client_node *client, char* packet) {
	client_node *cur = get_head();

	while (cur != NULL) {
		if (strlen(cur->handle) && strcmp(client->handle, cur->handle)) {
			safe_send(cur->socket, packet, ntohs(header->length), 0);	
		}
		cur = cur->next;
	}
}

void forward_message(chat_header *header, client_node *client, char* packet) {
	/* Get all destination clients from packet */
	client_node *cur = get_head();
	uint8_t recv_handle_len;
	char *packet_ptr = packet;
	uint8_t loc;
	int i;


	packet_ptr += sizeof(chat_header);
	memcpy(&recv_handle_len, packet_ptr, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	
	char recv_handle[recv_handle_len +1];
	memcpy(recv_handle, packet_ptr, recv_handle_len);
	recv_handle[recv_handle_len] = '\0';
	packet_ptr += recv_handle_len;
	
	uint8_t num_handles;
	memcpy(&num_handles, packet_ptr, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	

	char handles[num_handles][100];
	for (i = 0; i < num_handles; i++) {
		memcpy(&loc, packet_ptr, sizeof(uint8_t));
		packet_ptr += sizeof(uint8_t);
		memcpy(handles[i], packet_ptr, loc);
		packet_ptr += loc;	
		handles[i][loc] = '\0';

		cur = search_handle(handles[i]);
		if (cur != NULL) {
			safe_send(cur->socket, packet, ntohs(header->length), 0);
		} else {
			return_error(client->socket, handles[i]);
		}
	}
}

void return_error(int socket, char* handle) {
	chat_header header;
	char packet[BUF_SIZE];
	char *packet_ptr = packet;
	uint8_t handle_len = strlen(handle);
	
	packet_ptr += sizeof(header);
	memcpy(packet_ptr, &handle_len, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	memcpy(packet_ptr, handle, handle_len);
	packet_ptr += handle_len;
	uint16_t len = packet_ptr - packet;
	header = create_header(FLAG_7, len);
	memcpy(packet, &header, sizeof(chat_header));

	safe_send(socket, packet, len, 0);
}

void list_handles(client_node *client) {
	client_node *cur = get_head();
	/* Send a packet with the number of handles*/
	send_num_handles(client->socket);
	/* Send each handles */
	while (cur != NULL) {
		send_handle(client->socket, cur->handle);
		cur = cur->next;
	}
	/* Send a packet with flag 13 to tell the client you are done */
	send_header(client->socket, FLAG_13);
} 

void send_num_handles(int socket) {
	char packet[BUF_SIZE];
	char *packet_ptr = packet;
	chat_header header;
	uint32_t num_handles = 0;
	client_node *cur = get_head();
	uint16_t packet_len = sizeof(chat_header) + sizeof(uint32_t);
	
	while(cur != NULL) {
		num_handles++;
		cur = cur->next;
	}

	header = create_header(FLAG_11, packet_len);
	memcpy(packet_ptr, &header, sizeof(chat_header));
	packet_ptr += sizeof(chat_header);
	num_handles = htonl(num_handles);
	memcpy(packet_ptr, &num_handles, sizeof(uint32_t));
	safe_send(socket, packet, packet_len, 0);
}

void send_handle(int socket, char *handle) {
	char packet[BUF_SIZE];
	char *packet_ptr = packet;
	chat_header header;

	uint16_t packet_len = sizeof(chat_header) + sizeof(uint8_t) + sizeof(handle);
	uint8_t handle_len = strlen(handle);
	
	header = create_header(FLAG_12, packet_len);
	memcpy(packet_ptr, &header, sizeof(chat_header));
	packet_ptr += sizeof(chat_header);
	memcpy(packet_ptr, &handle_len, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	memcpy(packet_ptr, handle, handle_len);
	safe_send(socket, packet, packet_len, 0);
}

void client_disconnect(int socket) {
	remove_socket(socket); //remove client from linked list
	send_header(socket, FLAG_9); // send a client confirmation
	close(socket);
}

int checkArgs(int argc, char *argv[]) {
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2) {
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	//check for optional port number
	if (argc == 2) {
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

void get_packets(int server_socket) {
	int max_socket = 3;
	int client_socket;
	fd_set rfds;
	client_node *cur;
	/* Get packets until program is exitted with ctrl-c */
	while(1) {
		/* Rebuild our set of sockets */ 
		cur = get_head();
		FD_ZERO(&rfds);
		FD_SET(server_socket, &rfds);
		/* Add all sockets from our list of registerred clients*/
		while (cur != NULL) {
			max_socket = MAX(max_socket, cur->socket);
			FD_SET(cur->socket, &rfds);
			cur = cur->next;			
		}
		/* Wait for a packet to arrive from a socket*/
		if (select(max_socket + 1, &rfds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}
		
		/* Check if we have a new client to handle */
		if (FD_ISSET(server_socket, &rfds)) {
			//accept the new client
			client_socket = tcpAccept(server_socket);
			//add client to linked list
			add_client("", client_socket);
		}

		/*Check if any of our clients have a packet to handle*/
		cur = get_head();
		client_node *next;
		while (cur != NULL) {
			next = cur->next;
			if (FD_ISSET(cur->socket, &rfds)) { 
				handle_packet(cur);
				if (cur != NULL) {
					FD_CLR(cur->socket, &rfds);
				}
			}
			cur = next;
		}
	}
}

int main(int argc, char *argv[]) {
	int server_socket = 0;   //socket descriptor for the server socket
	int port_number = 0;
	
	port_number = checkArgs(argc, argv);
	//create the server socket
	server_socket = tcpServerSetup(port_number);
	//get packets from clients 
	get_packets(server_socket);
	return 1;
}