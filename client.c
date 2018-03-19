/*
 * CSE231: Operating Systems
 * Assignment 2: Multi-User Chat System
 * Brihi Joshi (2016142)
 * Taejas Gupta (2016204)
 * March 18, 2018
 *
 * Client code.
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE 1024



void init_socket(int *sockfd)
{
	if((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Unable to open socket");
		exit(1);
	}
}



void connect_to_server(int sockfd, struct sockaddr_in *server_addr)
{
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = (in_port_t) htons(1337);
	server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(server_addr->sin_zero, '\0', sizeof(server_addr->sin_zero));

	if(connect(sockfd, (struct sockaddr *) server_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("Unable to connect to server");
		exit(1);
	}
}



void send_recv_message(int i, int sockfd)
{
	char send_buf[BUFFER_SIZE];
	char recv_buf[BUFFER_SIZE];

	if(i == 0)
	{
		fgets(send_buf, BUFFER_SIZE, stdin);
		if(strcmp(send_buf, "exit\n") == 0)
		{
			close(sockfd);
			printf("Client has disconnected from the server.\n");
			exit(0);
		}
		send(sockfd, send_buf, strlen(send_buf), 0);
	}

	else
	{
		int recv_size = recv(sockfd, recv_buf, BUFFER_SIZE, 0);
		recv_buf[recv_size] = '\0';
		printf("%s", recv_buf);

		fflush(stdout);
	}
}



int main()
{
	fd_set curr_fds, read_fds;
	int sockfd, max_fd;
	struct sockaddr_in server_addr;

	FD_ZERO(&curr_fds);
	FD_ZERO(&read_fds);

	init_socket(&sockfd);
	connect_to_server(sockfd, &server_addr);

	FD_SET(0, &curr_fds);
	FD_SET(sockfd, &curr_fds);

	max_fd = sockfd;

	while(1)
	{
		read_fds = curr_fds;
		if(select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
		{
			perror("Error in select");
			exit(4);
		}

		int i;
		for(i = 0; i <= max_fd; i++)
		{
			if(FD_ISSET(i, &read_fds))
				send_recv_message(i, sockfd);
		}
	}

	return 0;
}