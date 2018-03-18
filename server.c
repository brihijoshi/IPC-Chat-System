/*
 * CSE231: Operating Systems
 * Assignment 2: Multi-User Chat System
 * Brihi Joshi (2016142)
 * Taejas Gupta (2016204)
 * March 18, 2018
 *
 * Server code.
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024



void init_socket(int *sockfd)
{
	if((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Unable to open socket");
		exit(1);
	}
}



void bind_to_port(int sockfd, struct sockaddr_in *server_addr)
{
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = (in_port_t) htons(1337);
	server_addr->sin_addr.s_addr = INADDR_ANY;
	memset(server_addr->sin_zero, '\0', sizeof(server_addr->sin_zero));

	int reuse = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int)) < 0)
	{
		perror("Unable to set reuse option on socket");
		exit(1);
	}

	if(bind(sockfd, (struct sockaddr *) server_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("Unable to bind to port");
		exit(1);
	}

	printf("Server waiting for client on port 1337\n");

	fflush(stdout);
}



void listen_to_clients(int sockfd)
{
	if(listen(sockfd, 10) < 0)
	{
		perror("Unable to listen");
		exit(1);
	}
}



void accept_connection(fd_set *curr_fds, int *max_fd, int sockfd, struct sockaddr_in *client_addr)
{
	socklen_t addrlen;
	int client_sockfd;

	addrlen = sizeof(struct sockaddr_in);
	if((client_sockfd = accept(sockfd, (struct sockaddr *) client_addr, &addrlen)) < 0)
	{
		perror("Unable to accept connection");
		exit(1);
	}
	
	FD_SET(client_sockfd, curr_fds);
	if(client_sockfd > *max_fd)
		*max_fd = client_sockfd;
	printf("Client %s has connected on port %d.\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
}



void group_message(int i, int j, int sockfd, int recv_size, char *recv_buf, fd_set *curr_fds)
{
	if(FD_ISSET(j, curr_fds) && j != sockfd && j != i)
	{
		if(send(j, recv_buf, recv_size, 0) < 0)
			perror("Unable to send message");
	}
}



void relay_message(int i, fd_set *curr_fds, int sockfd, int max_fd)
{
	int recv_size;
	char recv_buf[BUFFER_SIZE], buf[BUFFER_SIZE];

	if((recv_size = recv(i, recv_buf, BUFFER_SIZE, 0)) <= 0)
	{
		if(recv_size == 0)
			printf("Connection at socket %d has been terminated.\n", i);
		else
			perror("Error in receiving message from client");
		close(i);
		FD_CLR(i, curr_fds);
	}

	else
	{
		int j;
		for(j = 0; j <= max_fd; j++)
			group_message(i, j, sockfd, recv_size, recv_buf, curr_fds);
	}
}



int main()
{
	fd_set curr_fds, read_fds;
	int sockfd, max_fd;
	struct sockaddr_in server_addr, client_addr;

	FD_ZERO(&curr_fds);
	FD_ZERO(&read_fds);

	init_socket(&sockfd);
	bind_to_port(sockfd, &server_addr);
	listen_to_clients(sockfd);

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
			{
				if(i == sockfd)
					accept_connection(&curr_fds, &max_fd, sockfd, &client_addr);
				else
					relay_message(i, &curr_fds, sockfd, max_fd);
			}
		}
	}

	return 0;
}