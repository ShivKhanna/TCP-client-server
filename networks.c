#include "networks.h"

/* This function sets the server socket.  It lets the system
determine the port number.  The function returns the server
socket number and prints the port number to the screen.  */

int tcpServerSetup(int portNumber) {
	int server_socket= 0;
	struct sockaddr_in6 server;      /* socket address for local side  */
	socklen_t len= sizeof(server);  /* length of local address        */

	/* create the tcp socket  */
	server_socket= socket(AF_INET6, SOCK_STREAM, 0);
	if (server_socket < 0) {
		perror("socket call");
		exit(1);
	}

	server.sin6_family= AF_INET6;         		
	server.sin6_addr = in6addr_any;   //wild card machine address
	server.sin6_port= htons(portNumber);         

	/* bind the name (address) to a port */
	if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("bind call");
		exit(-1);
	}
	
	//get the port name and print it out
	if (getsockname(server_socket, (struct sockaddr*)&server, &len) < 0) {
		perror("getsockname call");
		exit(-1);
	}

	if (listen(server_socket, BACKLOG) < 0) {
		perror("listen call");
		exit(-1);
	}
	printf("Server is using port %d \n", ntohs(server.sin6_port));

	return server_socket;
}

// This function waits for a client to ask for services.  It returns
// the client socket number.   

int tcpAccept(int server_socket) {
	struct sockaddr_in6 clientInfo;   
	int clientInfoSize = sizeof(clientInfo);
	int client_socket= 0;

	if ((client_socket = accept(server_socket, (struct sockaddr*) &clientInfo, (socklen_t *) &clientInfoSize)) < 0) {
		perror("accept call");
		exit(-1);
	}

	return client_socket;
}

int tcp_client_setup(char * serverName, char * port) {
	// This is used by the client to connect to a server using TCP
	
	int socket_num;
	uint8_t * ipAddress = NULL;
	struct sockaddr_in6 server;      
	
	// create the socket
	if ((socket_num = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket call");
		exit(-1);
	}

	// setup the server structure
	server.sin6_family = AF_INET6;
	server.sin6_port = htons(atoi(port));
	
	// get the address of the server 
	if ((ipAddress = getIPAddress6(serverName, &server)) == NULL) {
		exit(-1);
	}

	if(connect(socket_num, (struct sockaddr*)&server, sizeof(server)) < 0) {
		perror("connect call");
		exit(-1);
	}

	return socket_num;
}

ssize_t send_header(int socket, uint8_t flag) {
	char buff[BUF_SIZE];
	chat_header new_header = create_header(flag, sizeof(chat_header));
	memcpy(buff, &new_header, sizeof(chat_header));
	return safe_send(socket, buff, sizeof(chat_header), 0);
}

chat_header create_header(uint8_t flag, uint16_t len) {
	chat_header new_header;
	new_header.length = htons(len);
	new_header.flags = flag;
	return new_header;
}

ssize_t safe_send(int socket, const void *buffer, size_t len, uint8_t flags) {
	int ret = send(socket, buffer, len, flags); 
	if (ret < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}
	return ret;
}
