void create_server_socket(int *server_sock_fd, struct sockaddr_in *server_sock, int port_number){

	if (*server_sock_fd = socket(AF_INET, SOCK_STREAM, 0) < 0){
		error("ERROR Opening socket");
	}

	server_sock->sin_family = AF_INET;
	server_sock->sin_addr.s_addr = INADDR_ANY;
	server_sock->sin_port = htons(port_number);

	if (bind(*server_sock_fd, (struct sockaddr* ) server_sock , sizeof(struct sockaddr))< 0){
		error("Unable to bind");
	}

	if (listen(*server_sock_fd, 5) < 0){
		error ("Cannot listen");
	}

	printf("Waiting for the connection ...");

}

void main(){
	



	
}