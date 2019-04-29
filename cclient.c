#include "cclient.h"


int server_socket;
int max_socket;
char* handle; 

/* Send this client's handle to the server as its first sent packet */
void verify_handle() {
	fd_set rfds;	
	char packet[BUF_SIZE];
	char *packet_ptr = packet;
	uint8_t handle_len = strlen(handle);
	chat_header recv_header;
	chat_header header = create_header(FLAG_1, sizeof(chat_header) + sizeof(handle_len) + handle_len);
	
	memcpy(packet_ptr, &header, sizeof(chat_header));
	packet_ptr += sizeof(chat_header);
	
	memcpy(packet_ptr, &handle_len, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	
	memcpy(packet_ptr, handle, handle_len);
	packet_ptr += handle_len;
	safe_send(server_socket, packet, packet_ptr - packet, 0);

	FD_ZERO(&rfds);
	FD_SET(server_socket, &rfds);
	if (select(server_socket +1, &rfds, NULL, NULL, NULL) < 0) {
		perror("select");
		exit(0);
	}

	if (FD_ISSET(server_socket, &rfds)) {
		// recv confirmation or error
		if ((recv(server_socket, &recv_header, sizeof(chat_header), MSG_WAITALL)) < 0) {
			perror("recv");
			exit(0);
		}

		if (recv_header.flags == FLAG_3) {
			fprintf(stderr, "Handle already in use: %s\n", handle);
			close(server_socket);
			exit(0);
		}
	}
	printf("$: "); fflush(stdout);
}

void check_args(int argc, char * argv[]) {
	/* check command line arguments  */
	if (argc != 4) {
		printf("Usage: %s handle server-name server-port \n", argv[0]);
		exit(0);
	} 
	else if (strlen(argv[1]) > MAX_HANDLE_LEN - 1) {
		printf("Invalid handle, handle longer than 100 characters: %s", argv[1]);
		exit(0);
	} else if (isdigit(argv[1][0])) {
	    printf("Invalid handle, handle starts with a number.\n");
		exit(0);
	}
}

void send_receive_message() {
	fd_set rfds;

	while (1) {
		/* Clear and set client/server server_sockets */
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(server_socket, &rfds);

		/* wait for select to get a packet */
		if (select(server_socket + 1, &rfds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(0);
		}
		/* handle packets from server */
		if (FD_ISSET(server_socket, &rfds)) {
			handle_packet();
		}
		/* handle input from stdin */
		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			handle_input();
		}
	}
}

void handle_packet() {
	char packet[BUF_SIZE];
	char *packet_ptr = packet;
	chat_header header;
	int len = 0;

	/* Get the header */
	if ((len = recv(server_socket, packet, sizeof(chat_header), MSG_WAITALL)) < 0) {
		perror("recv call");
		exit(-1);
	}

	if (!len) {
		close(server_socket);
		printf("\nServer Terminated\n");
		exit(1);
	}

	memcpy(&header, packet_ptr, sizeof(chat_header));
	header.length = ntohs(header.length);

	packet_ptr += sizeof(chat_header);
	if (header.length > sizeof(chat_header)) {
		if (recv(server_socket, packet_ptr, header.length - sizeof(chat_header), MSG_WAITALL) < 0) {
			perror("recv");
			exit(0);
		}
	}


	parse_packet(header, packet);
}

void parse_packet(chat_header header, char* packet) {
	switch (header.flags) {
		case FLAG_3: /* Client rejeted handle */
			handle_error();
			break;
		case FLAG_4: /*broadcast message */
			recv_broadcast(packet);
			break;	
		case FLAG_5:
			recv_msg(packet);
			break;
		case FLAG_7:
			msg_error(packet);
			break;
		case FLAG_9:
			disconnect();
			break;
		case FLAG_11:
			recv_handle_count(packet);
			break;		
		case FLAG_12:
			recv_handle(packet);
			break;
		case FLAG_13: 
	        printf("$: "); fflush(stdout);
         	break;
		default:
			break;	
	}
}

void disconnect() {
	close(server_socket);
	exit(1);
}

void recv_handle_count(char *packet) {
	uint32_t handle_count;
	packet += sizeof(chat_header);
	memcpy(&handle_count, packet, sizeof(uint32_t));
	handle_count = ntohl(handle_count);
	printf("Number of clients: %u\n", handle_count);
}

void recv_handle(char *packet) {
	uint8_t handle_len;
	char source_handle[MAX_HANDLE_LEN];
	packet += sizeof(chat_header);
	memcpy(&handle_len, packet, sizeof(uint8_t));
	packet += sizeof(uint8_t);
	memcpy(source_handle, packet, handle_len);
	source_handle[handle_len] = '\0';
	printf("\t%s\n", source_handle);
} 

void msg_error(char *packet) {
	chat_header header;
	memcpy(&header, packet, sizeof(chat_header));
	packet += sizeof(chat_header);

	uint8_t handle_len;
	memcpy(&handle_len, packet, sizeof(uint8_t));
	packet += sizeof(uint8_t);

	char source_handle[MAX_HANDLE_LEN];
	memcpy(source_handle, packet, handle_len);
	source_handle[handle_len] = '\0';

	printf("Client with handle %s does not exist.\n", source_handle);
	printf("$: "); fflush(stdout);
	   
}

void recv_broadcast(char *packet) {
	char source_handle[MAX_HANDLE_LEN];
	uint8_t handle_len;

	packet += sizeof(chat_header);
	memcpy(&handle_len, packet, sizeof(uint8_t));
	packet += sizeof(uint8_t);

	memcpy(source_handle, packet, handle_len);
	source_handle[handle_len] = '\0';
	packet += handle_len;
  	printf("\n%s: %s\n", source_handle, packet);
}

void recv_msg(char *packet) {
	uint8_t handle_len;
	char source_handle[MAX_HANDLE_LEN];

	packet += sizeof(chat_header);
	memcpy(&handle_len, packet, sizeof(uint8_t));
	packet += sizeof(uint8_t);

	memcpy(source_handle, packet, handle_len);
	source_handle[handle_len] = '\0';
	packet += handle_len;

	uint8_t num_handles;
	memcpy(&num_handles, packet, sizeof(uint8_t));
	packet += sizeof(uint8_t);

	int i;
	for (i = 0; i < num_handles; i++) {
		uint8_t dest_len = 0;
		memcpy(&dest_len, packet, sizeof(uint8_t));
		packet += sizeof(uint8_t) + dest_len;
	}

  	printf("\n%s: %s\n", source_handle, packet);
   	printf("$: "); fflush(stdout);
}

void handle_error() {
	printf("Handle already in use: %s\n", handle);
	close(server_socket);
	exit(-1);
}

void handle_input() {
	char input_buf[MAX_INPUT];
	input_buf[0] = '\0';
	if (fgets(input_buf, MAX_INPUT, stdin) == NULL) {
		perror("error");
		return;
	}
	//remove newline
	input_buf[strlen(input_buf) - 1] = '\0';

	if (!strncmp(input_buf, "\%M", 2) || !strncmp(input_buf, "\%m", 2)) {
		parse_message(input_buf);
		printf("$: ");
		fflush(stdout);
	}
	else if (!strncmp(input_buf, "\%B", 2) || !strncmp(input_buf, "\%b", 2)) {
		parse_broadcast(input_buf);
		printf("$: ");
		fflush(stdout);
	}
	else if (!strncmp(input_buf, "\%L", 2) || !strncmp(input_buf, "\%l", 2)) {
		send_header(server_socket, FLAG_10);
	}
	else if (!strncmp(input_buf, "\%E", 2) || !strncmp(input_buf, "\%e", 2)) {
		send_header(server_socket, FLAG_8);
	}
	else {
		printf("Invalid command\n");
		printf("$: ");
    	fflush(stdout);
	}
}

void parse_broadcast(char input_buf[MAX_INPUT]) {
	char temp[MAX_INPUT];
	memcpy(temp, input_buf, MAX_INPUT);
	strtok(temp, " ");
	char *tok = strtok(NULL, " ");

	while (strlen(tok) > MESSAGE_CHUNK) {
		char chunk[MESSAGE_CHUNK];
		memcpy(chunk, tok, MESSAGE_CHUNK);
		chunk[MESSAGE_CHUNK] = '\0';
		send_broadcast(chunk);
		tok += MESSAGE_CHUNK;
	} 
	send_broadcast(tok);
}

void send_broadcast(char *message) {
	char packet[BUF_SIZE]; 
	char *packet_ptr = packet;
	chat_header header;

	packet_ptr += sizeof(chat_header);
	uint8_t handle_len = strlen(handle);
	memcpy(packet_ptr, &handle_len, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	memcpy(packet_ptr, handle, handle_len);
	packet_ptr += handle_len;

	memcpy(packet_ptr, message, strlen(message) + 1);
	packet_ptr += strlen(message) + 1;

	uint8_t packet_len = packet_ptr - packet;
	header = create_header(FLAG_4, packet_len);
	memcpy(packet, &header, sizeof(chat_header));
	safe_send(server_socket, packet, packet_len, 0);
}

void parse_message(char input_buf[MAX_INPUT]) {
	char temp[MAX_INPUT];
	int num_handles;
	int i;

	memcpy(temp, input_buf, MAX_INPUT);
	strtok(temp, " ");
	char *tok = strtok(NULL, " ");
	
	if (tok == NULL) {
		printf("Too few handles entered.\n");
		return;
	}

	char *c;
	num_handles = strtol(tok, &c, 10);

	if (!num_handles) {
		num_handles = 1;
		strtok(input_buf, " ");
	} else if (num_handles < 1 || num_handles > 9) {
		printf("Invalid number of destinations\n");
		return;
	}

	char *handle_endpoints[num_handles];
	for (i = 0; i < num_handles; i++) {
		tok = strtok(NULL, " ");
		if(tok == NULL) {
			printf("Too few handles entered.\n");
			return;
		}
		handle_endpoints[i] = tok;
	}

	tok = strtok(NULL, "\0");
	if (tok == NULL)
    	tok = "";

	/* split message into 200 char packets and send*/
	split_message(tok, handle_endpoints, num_handles);
}

void split_message(char *message, char **handle_endpoints, int num_handles) {
	while (strlen(message) > MESSAGE_CHUNK) {
		char chunk[MESSAGE_CHUNK];
		memcpy(chunk, message, MESSAGE_CHUNK);
		chunk[MESSAGE_CHUNK] = '\0';
		send_message(chunk, handle_endpoints, num_handles);
		message += MESSAGE_CHUNK;
	} 
	send_message(message, handle_endpoints, num_handles);
}

void send_message(char *message, char **handle_endpoints, int num_handles) {
	char packet[BUF_SIZE]; 
	char *packet_ptr = packet;
	chat_header header;

	packet_ptr += sizeof(chat_header);
	uint8_t handle_len = strlen(handle);
	memcpy(packet_ptr, &handle_len, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);
	memcpy(packet_ptr, handle, handle_len);
	packet_ptr += handle_len;

	uint8_t dest_handles = num_handles;
	memcpy(packet_ptr, &dest_handles, sizeof(uint8_t));
	packet_ptr += sizeof(uint8_t);

	int i;
	for (i = 0; i < dest_handles; i++) {
		uint8_t dest_len = strlen(handle_endpoints[i]);
		memcpy(packet_ptr, &dest_len, sizeof(uint8_t));
		packet_ptr += sizeof(uint8_t);
		memcpy(packet_ptr, handle_endpoints[i], dest_len);
		packet_ptr += dest_len;
	}

	memcpy(packet_ptr, message, strlen(message) + 1);
	packet_ptr += strlen(message) + 1;

	uint8_t packet_len = packet_ptr - packet;
	header = create_header(FLAG_5, packet_len);
	memcpy(packet, &header, sizeof(chat_header));
	safe_send(server_socket, packet, packet_len, 0);
}

int main(int argc, char * argv[]) {
	check_args(argc, argv);
	handle =  argv[1];
	/* set up the TCP Client server_socket  */
	server_socket = tcp_client_setup(argv[2], argv[3]);
	/* Send initial packet to register handle, wait for confirmation */
	verify_handle();
	
	/*Send messages to server from stdin and recieive/parse packets from server*/  
	send_receive_message();

	return 1;
}
