

/*************************************************************
*
* A simple client
*   The hostname and port number is passed as an argument 
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include "transfiles.h"

// =============================================================================================


int main(int argc, char *argv[]) {

	int 		socket;
	char 		client_name[BUF];

	/**************************************/

	system("clear");

	// Verifico que paso bien los datos
	if (argc != 4) {
		fprintf(stderr,"usage %s nickname file\n", argv[0]);
		return -1;
	}

	socket = clientcon(argv[2], PORT);

	// Guardo el nickname y lo mando al server
	strncpy(client_name, argv[1], strlen(argv[1]));
	write_to(socket, client_name, strlen(client_name)+1);

	printf("\nConectado correctamente al server.\n\n");

	send_data(socket, argv[3]);

	close(socket);
	return 0;
}

