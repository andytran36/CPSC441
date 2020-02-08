/* 
 * Assignment 1: Fake News Proxy
 * Created: Feb 2, 2020
 * Author: Andy Tran
 * UCID: 30040469
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <signal.h>

#define HTTP_PORT 80 // default web port
#define MESSAGE_SIZE 1024 // 1KB

// Sockets for proxy to listen to, write to, and communicated with server.
int lstn_sock, data_sock, web_sock;

void handleTXT(char* server_response, int serverBytes);
void handleHTML(char* server_response, int serverBytes);

void catcher(int sig) {
	close(lstn_sock);
	close(data_sock);
	close(web_sock);
	exit(0);
}

int main (int argc, char* argv[]) {
	// For storing HTTP requests
	char client_request[MESSAGE_SIZE], server_request[MESSAGE_SIZE];
	// For storing response from web server
	char server_response[10 * MESSAGE_SIZE], client_response[10 * MESSAGE_SIZE];
	// For parsing HTTP request
	char host[MESSAGE_SIZE], url[MESSAGE_SIZE], path[MESSAGE_SIZE];
	// For parsing HTTP response
	char type[MESSAGE_SIZE];
	int i, serverBytes, clientBytes;


	// Making sure there is an input with a port number
	if (argc != 2){
		printf("Error: command line arguments\n");
		exit(-1);
	}

	// Parse command line argument for proxy port number
	int PROXY_PORT = atoi(argv[1]);

	// Catch ungraceful exits
	signal(SIGTERM, catcher);
	signal(SIGINT, catcher);

	// Initialize socket address
	printf("Initializing proxy address...\n");
	struct sockaddr_in proxy_addr;
	proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_port = htons(PROXY_PORT);

	// Create listening socket for proxy to listen to user requests
	printf("Creating lstn sock...\n");
	lstn_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (lstn_sock < 0) {
		printf("socket() call failed.\n");
		exit(-1);
	}

	// Bind lstn_socket to address and port
	printf("Binding lstn_sock...\n");
	int bind_status;
	bind_status = bind (lstn_sock, (struct sockaddr*) &proxy_addr, sizeof(struct sockaddr_in));
	if (bind_status < 0) {
		printf("bind() call failed.\n");
		exit(-1);
	}

	// Listen on binded port number
	printf("Listening on lsten_sock...\n");
	int lstn_status;
	lstn_status = listen(lstn_sock, 10);
	if (lstn_status < 0) {
		printf("listen() call failed.\n");
		exit(-1);
	}

	// Infinite while loop to listen to requests
	while(1) {
		// Accept connection requests
		printf("Accepting connection requests...\n");
		data_sock = accept(lstn_sock, NULL, NULL);
		if (data_sock < 0) {
			printf("accept() call failed.\n");
			exit(-1);
		}

		// Receive HTTP message from web browser
		bzero(client_request, MESSAGE_SIZE);
		clientBytes = recv(data_sock, client_request, sizeof(client_request), 0);
		if (clientBytes < 0) {
			printf("recv() call failed for client request.\n");
			exit(-1);
		} else {
			printf("HTTP message received: \n");
			printf("%s\n", client_request);
		}

		// Parse HTTP message to extract information from the web server
		char* line = strtok(client_request, "\r\n");

		// If request is a jpg image that contains "Floppy", change to trollface.jpg
		int count = strlen(line);
		char* floppy_img_ptr = strstr(line, "Floppy");
		char* jpg_ptr = strstr(line, ".jpg");
		if (floppy_img_ptr != NULL && jpg_ptr != NULL) {
			strncpy(line, "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/trollface.jpg HTTP/1.1", count);
		}

		printf("HTTP request: %s\n", line);
		sscanf(line, "GET http://%s", url);
		
		for (i = 0; i < strlen(url); i++) {
			if (url[i] == '/') {
				// copy hostname
				strncpy(host, url, i);
				host[i] = '\0';
				break;
			}
		}

		bzero(path, MESSAGE_SIZE);
		strcat(path, &url[i]);

		printf("******************************\n");
		printf("Host: %s\n", host);
		printf("Path: %s\n", path);
		printf("******************************\n");

		// Create a GET request for the web server
		bzero(server_request, MESSAGE_SIZE);
		strcpy(server_request, line);
		strcat(server_request, "\r\n");
		strcat(server_request, "Host: ");
		strcat(server_request, host);
		strcat(server_request, "\r\n\r\n");

		// Create TCP socket for web server
		struct sockaddr_in server_addr;
		struct hostent* server;

		// Get web server address
		server = gethostbyname(host);
		if (server == NULL)
			printf("gethostbyname() call failed.\n");
		else
			printf("Web server: %s\n", server->h_name);

		// Initialize socket structure
		bzero((char*) &server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		bcopy((char*) server->h_addr, (char*)&server_addr.sin_addr.s_addr, server->h_length);
		server_addr.sin_port = htons(HTTP_PORT);

		// Create socket
		web_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (web_sock < 0) {
			printf("Error in socket() call for web server socket.\n");
			exit(-1);
		}

		// Connecting to web server socket
		int connect_status;
		connect_status = connect(web_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
		if (connect_status < 0) {
			printf("Error in connect() call for web server socket.\n");
			exit(-1);
		}

		// Sending HTTP request of client to web server
		int send_status;
		send_status = send(web_sock, server_request, sizeof(server_request), 0);
		if (send_status < 0) {
			printf("Error in send() call to web server.\n");
			exit(-1);
		}
		else {
			printf("Sending HTTP request to the server: \n");
			printf("%s\n", server_request);
		}

		// Receive HTTP response from the web server
		while((serverBytes = recv(web_sock, server_response, sizeof(server_response), 0)) > 0) {
			// Any modification to response goes here
			// printf("%s\n", server_response);

			// Parse Content-Type from the response
			char* type_ptr = strstr(server_response, "Content-Type: ");
			if (type_ptr != NULL) {
				strncpy(type, type_ptr, strcspn(type_ptr, ";"));
			}

			// Decide how to handle the request based on text or html (images handled beforehand)
			if (strcmp(type, "Content-Type: text/plain") == 0) {
				handleTXT(server_response, serverBytes);
			}
			else if (strcmp(type, "Content-Type: text/html") == 0){
				handleHTML(server_response, serverBytes);
			}

			bcopy(server_response, client_response, serverBytes);

			if (send(data_sock, client_response, serverBytes, 0) < 0) {
				printf("Error in send() call to client.\n");
				exit(-1);
			}

			bzero(client_response, 10 * MESSAGE_SIZE);
			bzero(server_response, 10 * MESSAGE_SIZE);
			bzero(type, MESSAGE_SIZE);
		} // End of server recv() while
	} // End of infinite while

    return 0;
}

void handleTXT(char* server_response, int serverBytes) {
	// Replace all instances of Floppy with Trolly
	char* str_ptr = strstr(server_response, "Floppy");
	while (str_ptr != NULL) {
		strncpy(str_ptr, "Trolly", 6);
		str_ptr = strstr(server_response, "Floppy");
	}

	// Replaces all instances of Italy with Germany
	str_ptr = strstr(server_response, "Italy");
	while (str_ptr != NULL) {
		server_response[serverBytes + 1] = '\0';
		server_response[serverBytes + 2] = '\0';
		memmove(str_ptr + 2, str_ptr, strlen(str_ptr) + 2);
		strncpy(str_ptr, "Germany", 7);
		str_ptr = strstr(server_response, "Italy");
	}
}

void handleHTML(char* server_response, int serverBytes) {
	char* start_ptr = server_response;
	while (start_ptr != NULL) {
		// Point to next occuring image or link tag
		char* img_ptr = strstr(start_ptr, "<img src=\"");
		char* a_ptr = strstr(start_ptr, "<a href=\"");
		// Image tag arbitrarily set as end
		char* end_ptr = img_ptr;
		
		// If the link tag comes first, set it as the end instead of image tag
		if (a_ptr < img_ptr)
			end_ptr = a_ptr;
		
		// Making sure they arent NULL; if both are null, set end-ptr to null
		if (img_ptr == NULL) {
			end_ptr = a_ptr;
		} else if (a_ptr == NULL) {
			end_ptr = img_ptr;
		} else if (img_ptr == NULL  && a_ptr == NULL) {
			end_ptr = NULL;
		}

		if (end_ptr != NULL) {
			// count is how long the image tag is
			int count = strcspn(end_ptr, ">");
			
			// Replace all instances of Floppy with Trolly until an image tag is reached
			char* str_ptr = strstr(start_ptr, "Floppy");
			while (str_ptr != NULL && str_ptr < end_ptr) {
				strncpy(str_ptr, "Trolly", 6);
				str_ptr = strstr(start_ptr, "Floppy");
			}

			// Replaces all instances of Italy with Germany until an image tag is reached
			str_ptr = strstr(start_ptr, "Italy");
			while (str_ptr != NULL && str_ptr < end_ptr) {
				// Make space for new bytes
				server_response[serverBytes + 1] = '\0';
				server_response[serverBytes + 2] = '\0';
				memmove(str_ptr + 2, str_ptr, strlen(str_ptr) + 2);
				strncpy(str_ptr, "Germany", 7);
				str_ptr = strstr(start_ptr, "Italy");
			}
			// Moves the start pointer to past the first image tag
			start_ptr = end_ptr + count;
		} 
		// When the last image tag is found, all subsequent occurences of "Floppy" and "Italy" can be changed without worrying about img_ptr
		else {
			// Replace all instances of Floppy with Trolly
			char* str_ptr = strstr(start_ptr, "Floppy");
			while (str_ptr != NULL && str_ptr) {
				strncpy(str_ptr, "Trolly", 6);
				str_ptr = strstr(start_ptr, "Floppy");
			}

			// Replaces all instances of Italy with Germany
			str_ptr = strstr(start_ptr, "Italy");
			while (str_ptr != NULL && str_ptr) {
				// Make space for new bytes
				server_response[serverBytes + 1] = '\0';
				server_response[serverBytes + 2] = '\0';
				memmove(str_ptr + 2, str_ptr, strlen(str_ptr) + 2);
				strncpy(str_ptr, "Germany", 7);
				str_ptr = strstr(start_ptr, "Italy");
			}
			break;
		}
	}
}