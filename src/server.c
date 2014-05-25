/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include "transfiles.h"


// =============================================================================================

int main(int argc, char *argv[]) {
	int 				sockfd;
	client_t 			client;

	/**************************************/

	system("clear");

	// Creo el socket por donde se van a recibir las conexiones
	sockfd = servercon(PORT);
	// Dejo al socket sockfd a la escucha
	listen(sockfd, 1);

	printf("\nEsperando conexiones entrantes...\n\n");

	client.clilen = sizeof(client.cli_addr);
	client.fd_client = accept(sockfd, (struct sockaddr *) &client.cli_addr, &client.clilen);

	// Leo el nombre del cliente
	read_from(client.fd_client, client.nickname, SIZE_BUF);
	printf("- %s se acaba de conectar:\n", client.nickname);

	mkfolder();
/*
	receive_file(client.fd_client);
	receive_file(client.fd_client);
	receive_file(client.fd_client);
	receive_file(client.fd_client);
	receive_file(client.fd_client);
	receive_file(client.fd_client);
	*/

	//printf("return: %d\n", receive_file(client.fd_client));
	receive_data(client.fd_client);

	// Cierra los file descriptor
	close(client.fd_client);
	close(sockfd);

	return 0; 
}
