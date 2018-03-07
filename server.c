#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread

//struct describing client data
struct client_data{
	int sock;
	char *name;
};

//list of all the clients that we have
struct client_data clients[50];
//Number of clients, used to index the array;
int count = 0;

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*) socket_desc;
		printf("HERE\n");
		printf("%d\n",sock);
		clients[count].sock = sock;

		int temp = 0; //for the first argument to be a name of the person

		if (temp == 0){
			char name[100];
			int get_name = recv(sock, name, 100, 0);
			printf("Got name %s\n",name);
			clients[count].name = name;
			count++;
			temp = 1;
		}

		int read_size;
    char *message , client_message[2000];

    while( (read_size = recv(sock ,client_message ,2000 , 0)) > 0 )
    {
        //Send the message back to client
				char *name;
				for(int i=0;i<count;i++){
					if (clients[i].sock == sock){
						name = clients[i].name;
					}
				}

				for (int i=0;i < count; i++){
						char formatted_message[100];
						sprintf(formatted_message, "%s: %s", name, client_message);
        		write(clients[i].sock , formatted_message, strlen(formatted_message));
				}
    }

    if(read_size == 0){
        puts("Client disconnected");
        fflush(stdout);
    }

    else if(read_size == -1){
        perror("recv failed");
    }

    //Free the socket pointer
    free(socket_desc);

    return 0;
}

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t* )&c)) )
    {
        puts("Connection accepted");
				printf("%d\n", socket_desc);

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }

    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }

    return 0;
}
