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



char usernames[1024][256];



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
	printf("Client at socket %d has connected on port %d.\n", client_sockfd, ntohs(client_addr->sin_port));

	char *username_prompt = "Enter your username: ";
	if(send(client_sockfd, username_prompt, strlen(username_prompt), 0) < 0)
		perror("Unable to prompt username");

	int recv_size;

	int done = 0;
	do
	{
		if((recv_size = recv(client_sockfd, usernames[client_sockfd], 256, 0)) <= 1)
		{
			if(recv_size == 1)
			{
				char *retry_message = "The username cannot be empty, try again: ";
				if(send(client_sockfd, retry_message, strlen(retry_message), 0) < 0)
					perror("Error sending retry message");
			}

			else
			{
				if(recv_size == 0)
					printf("Connection at socket %d has been terminated\n", client_sockfd);
				else
					perror("Error in receiving message from client");
				close(client_sockfd);
				FD_CLR(client_sockfd, curr_fds);
			}
		}
		else
			done = 1;
	} while(!done);

	usernames[client_sockfd][recv_size - 1] = '\0';
	printf("Client at socket %d has been assigned username '%s'\n", client_sockfd, usernames[client_sockfd]);
}



void send_message(int i, int j, int sockfd, int recv_size, char *recv_buf, fd_set *curr_fds, int max_fd)
{
	char messageToParse[BUFFER_SIZE];
	strcpy(messageToParse, recv_buf);
	messageToParse[recv_size] = '\0';
	char *tokens[128];
	int k = 0;
	tokens[k] = strtok(messageToParse, " ");
	while(tokens[k] != NULL)
		tokens[++k] = strtok(NULL, " ");

	char args[128][128];
	int l = 0;
	while(l < k)
	{
		strcpy(args[l], tokens[l]);
		l++;
	}
	strcpy(args[l], "");

	int new_word = 1;
	int targets = 0;
	int message_start = 0;
	int done = 0;
	while(message_start < recv_size)
	{
		if(new_word)
		{
			if(recv_buf[message_start] == '@')
			{
				targets++;
				new_word = 0;
			}
			else if(recv_buf[message_start] != ' ')
			{
				done = 1;
				break;
			}
		}

		else if(recv_buf[message_start] == ' ')
			new_word = 1;

		message_start++;
	}

	if(!done)
		return;

	if(targets == 0)
	{
		if(FD_ISSET(j, curr_fds) && j != sockfd && j != i)
		{
			char message[BUFFER_SIZE], messageToSend[BUFFER_SIZE];
			strcpy(messageToSend, recv_buf);
			messageToSend[recv_size] = '\0';
			sprintf(message, "[%s @Group]: %s", usernames[i], messageToSend);
			if(send(j, message, sizeof(message), 0) < 0)
				perror("Unable to send message");
		}
	}

	else
	{
		int m;
		for(m = 0; m < targets; m++)
		{
			if(strcmp(args[m], "@all") == 0 || strcmp(args[m], "@group") == 0)
			{
				if(FD_ISSET(j, curr_fds) && j != sockfd && j != i)
				{
					char message[BUFFER_SIZE], messageToSend[BUFFER_SIZE];
					strcpy(messageToSend, recv_buf);
					messageToSend[recv_size] = '\0';
					sprintf(message, "[%s @Group]: %s", usernames[i], &messageToSend[message_start]);
					if(send(j, message, sizeof(message), 0) < 0)
						perror("Unable to send message");
				}
			}

			else
			{
				if(FD_ISSET(j, curr_fds) && j != sockfd && j != i && strcmp(&args[m][1], usernames[j]) == 0)
				{
					char message[BUFFER_SIZE], messageToSend[BUFFER_SIZE];
					strcpy(messageToSend, recv_buf);
					messageToSend[recv_size] = '\0';
					sprintf(message, "[%s]: %s", usernames[i], &messageToSend[message_start]);
					if(send(j, message, sizeof(message), 0) < 0)
						perror("Unable to send message");
				}
			}
		}
	}
}



void relay_message(int i, fd_set *curr_fds, int sockfd, int max_fd)
{
	int recv_size;
	char recv_buf[BUFFER_SIZE], buf[BUFFER_SIZE];

	if((recv_size = recv(i, recv_buf, BUFFER_SIZE, 0)) <= 0)
	{
		if(recv_size == 0)
			printf("Connection at socket %d has been terminated\n", i);
		else
			perror("Error in receiving message from client");
		close(i);
		FD_CLR(i, curr_fds);
	}

	else
	{
		int j;
		for(j = 0; j <= max_fd; j++)
			send_message(i, j, sockfd, recv_size, recv_buf, curr_fds, max_fd);
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