
#include "transfiles.h"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int clientcon(char *host, int port) {
	conection_t 	conection;

	// Creo el socket
	if ((conection.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		error("ERROR opening socket");
		return -1;
	}

	// Retorna una estructura hostent con el nombre del host en *h_name
	if ((conection.server = gethostbyname(host)) == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		return -1;
	}

	// Llevo a cero la estructura serv_addr
	bzero((char *) &conection.serv_addr, sizeof(conection.serv_addr));

	conection.serv_addr.sin_family = AF_INET;

	// Copia el address del server en serv_addr
	bcopy((char *)conection.server->h_addr, (char *)&conection.serv_addr.sin_addr.s_addr, conection.server->h_length);

	// Puerto del server
	conection.serv_addr.sin_port = port;

	// Trata de conectarse al server
	if (connect(conection.sockfd,(struct sockaddr *) &conection.serv_addr, sizeof(conection.serv_addr)) < 0) 
		error("ERROR connecting");

	return conection.sockfd;
}

int servercon(int port) {

	int					sockfd;
	struct sockaddr_in 	serv_addr;
	int 				val = 1;

	// Creo el socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		error("ERROR opening socket");
		return -1;
	}

	// Llevo a cero la estructura serv_addr
	bzero((char *) &serv_addr, sizeof(serv_addr));

	// Caracteristicas del socket (?
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = port;

	// Seteo las opciones para el socket
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	// Asigno un puerto de escucha al socket
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
		return -1;
	}

	return sockfd;
}

int read_from(int fd, char *buffer, int size_of_buffer) {

	int bytes;

	bytes = read(fd, buffer, size_of_buffer);

	if(bytes < 0) {
		printf("No se pudo leer en el file descriptor.\n");
		return -1;
	}

	return bytes;
}

int write_to(int fd, char *buffer, int size_of_buffer) {

	int bytes;

	bytes = write(fd, buffer, size_of_buffer);

	if(bytes < 0) {
		printf("No se pudo escribir en el file descriptor.\n");
		return -1;
	}

	return bytes;
}

int send_file(int socket, char *filename) {
	file_t 			file;
	struct stat 	stat_file;
	int 			numb_bytes;
	int 			sended = 0;
	char 			type;

	strncpy(file.name_of_file, filename, strlen(filename));	
	split(file.relative_path, filename, '/');

	if((file.relative_path[strlen(file.relative_path) - 1]) == '/') {
		type = (char) DIRECTORY;
		//printf("Directory!\n");
		printf("Sending: %s\n", file.name_of_file);

		file.size_string = (unsigned int) strlen(file.relative_path) + 1;

		//strncpy(file.relative_path, filename, strlen(filename));
		write(socket, &type, sizeof(char));
		write(socket, &file.size_string, sizeof(file.size_string));
		write(socket, &file.relative_path, (int)strlen(file.relative_path) + 1);
		printf("Datos:\n");
		printf("\t- type: %d\n", type);
		printf("\t- size string: %u\n", file.size_string);
		printf("\t- name: %s\n", file.relative_path);

		return 0;
	} else {
		type = (char) FILE;
		//printf("File!\n");

		file.fd_file = open(file.name_of_file, 0); 
		fstat(file.fd_file, &stat_file);
		file.size_file = (unsigned int) stat_file.st_size;
		file.size_string = (unsigned int) strlen(file.relative_path) + 1;
		printf("\tEnviando el archivo '%s'...\n", file.name_of_file);

		// Envio los datos del archivo
		write(socket, &type, sizeof(char));
		write(socket, &file.size_file, sizeof(file.size_file));
		write(socket, &file.size_string, sizeof(file.size_string));
		write(socket, &file.relative_path, (int)strlen(file.relative_path) + 1);
		printf("Datos:\n");
		printf("\t- type: %d\n", type);
		printf("\t- size file: %ukB\n", file.size_file);
		printf("\t- size string: %u\n", file.size_string);
		printf("\t- name: %s\n", file.relative_path);

		while(1) {
			numb_bytes = sendfile(socket, file.fd_file, NULL, SIZE_BUF);
			sended += numb_bytes;
			//printf("nbytes = %d\n", numb_bytes);
		//printf("\tEnviados %.3f/%.3f kB [%.3f%%]\n", sended/1000., file.size_file/1000., (sended*100.)/file.size_file);
			if(sended == file.size_file)
				break;
		}

		printf("\t\tEnviados %.3f/%.3f kB [%.3f%%]\n", sended/1000., file.size_file/1000., (sended*100.)/file.size_file);
	
		close(file.fd_file);

		return sended;
	}

	return -1;
}


int send_dir(int socket, char *dir_name) {
    DIR *d = NULL;				/* Where result's opendir is saved */

	send_file(socket, dir_name);

    /* Check it was opened. -1 on error */
    if(!(d = opendir (dir_name))) {
        fprintf (stderr, "Cannot open directory '%s': %s\n", dir_name, strerror (errno));
        return -1;
    } else {
    	/* Si el directorio se puede abrir */
	    while(1) {
	    	struct dirent *d_struct;
	    	char name[10000];

       		/* "Readdir" gets subsequent entries from "d" until the end (NULL) */
	    	if((d_struct = readdir(d))) {
	    		/* Concateno el nombre de dir_name con la nueva entrada */
	    		sprintf(name, "%s%s", dir_name, d_struct->d_name);

       			/* See if "entry" is a subdirectory of "d". */
	    		if(d_struct->d_type == DT_DIR) {
            		/* Check that the directory is not "d" or d's parent. */
	    			if(strcmp(d_struct->d_name, ".") && strcmp(d_struct->d_name, "..")) {
        				/* Print the name of the file and directory. */
		    			sprintf(name, "%s%s/", dir_name, d_struct->d_name);
		    			send_file(socket, name);
			    		printf("Sending: %s\n", name);
               			/* Recursively call "list_dir" with the new path. */
			    		send_dir(socket, name);   
			    	/* Por ahora, esto está al pedo */
		    		} else {
		    			sprintf(name, "%s%s", dir_name, d_struct->d_name);
			    		//printf("Folder: %s\n", name);
			    	}
			    /* Si no era un directorio */
			    } else {
			    	/* Imprimo el nombre del mismo */
			    	sprintf(name, "%s%s", dir_name, d_struct->d_name);
			    	send_file(socket, name);
			    	printf("Sending: %s\n", name);
			    }
            /* There are no more entries in this directory, so break out of the while loop. */
		    } else
		    	break;
	    }
    	/* After going through all the entries, close the directory. Else error EMFILE (24) [Too many open files] */
	    if (closedir (d)) {
	        fprintf (stderr, "Could not close '%s': %s\n", dir_name, strerror (errno));
	        exit (EXIT_FAILURE);
	    }
	}

	/* Si salió todo ok, retorna cero */
	return 0;
}

int send_data(int socket, char *filename) {
	char 	root[BUF];

	split(root, filename, '/');
	if((root[strlen(root) - 1]) != '/') {
		send_file(socket, filename);
	} else {
		send_dir(socket, filename);
		//printf("'send_dir' under construction\n");
	}
	return 0;
}


int receive_file(int socket) {
	file_t 		file;
	char 		buffer[SIZE_BUF];
	int 		numb_bytes;
	int 		recibed = 0;
	char 		type;

	// Leo el tamaño del archivo
	recv(socket, &type, sizeof(char), 0);

	if(type == (char) FILE) { 
		recv(socket, &file.size_file, sizeof(int), 0);
		recv(socket, &file.size_string, sizeof(int), 0);
		recv(socket, &file.name_of_file, file.size_string, 0);

		printf("\tReceiving '%s'...\n", file.name_of_file);

		if((file.fd_file = creat(file.name_of_file, S_IRWXU)) < 0) {
			error("No se puede abrir el archivo");
		}

		while(1) {
			bzero(buffer, SIZE_BUF);
			if((numb_bytes = read(socket, buffer, SIZE_BUF)) < 0) {
				printf("\tbytes = %d\n", numb_bytes);
				error("\tNo se pudo leer el archivo: ");
				break;
			}
			//printf("nbytes = %d\n", numb_bytes);
			write(file.fd_file, buffer, numb_bytes);
			recibed += numb_bytes;
			//printf("Recibidos: %.3fkB (%.3f/%.3f) kB\n", numb_bytes/1000., recibed/1000., file.size_file/1000.);
			if(numb_bytes == 0)
				break;
		}
		
		close(file.fd_file);

		printf("rec = %d\n", recibed);
		printf("sz = %d\n", file.size_file);
		if(recibed != file.size_file) {
			error("No se pudo enviar el archivo");
			return -1;
		} else {
			printf("\tRecibidos %.3f/%.3f kB [%.3f%%]\n", recibed/1000., file.size_file/1000., (recibed*100.)/file.size_file);
			return recibed;
		}
	} else if(type == (char) DIRECTORY) {
		recv(socket, &file.size_string, sizeof(int), 0);
		recv(socket, &file.name_of_file, file.size_string, 0);

		mkdir(file.name_of_file, 0700);
		chdir(file.name_of_file);
		printf("\tCreating '%s'...\n", file.name_of_file);
		return 0;
	}

	return -1;
}

int receive_data(int socket) {
	
	return receive_file(socket);
}

int getdate( char date[] ) {
	time_t 	rawtime;
	struct tm *timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	if(sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec) != 20)
		return -1;

	return 0;
}

int mkfolder(void) {
	char folder[BUF];
	char *name = NULL;
	char date[128];

	name = getlogin();
	getdate(date);

	sprintf(folder, "/home/%s/transfers/", name);
	
	if( mkdir(folder, 0700) < 0 ) {
		if( errno == 17 ) {
			sprintf(folder, "/home/%s/transfers/%s/", name, date);
			printf("\tCreating '%s'...\n", folder);
			mkdir(folder, 0700);
			chdir(folder);
		} else {
			printf("ERROR: %d\n", errno);
			return errno;
		}
	} else {
		printf("\tCreating '%s'...\n", folder);
		mkdir(folder, 0700);
		sprintf(folder, "/home/%s/transfers/%s/", name, date);
		printf("\tCreating '%s'...\n", folder);
		mkdir(folder, 0700);
		chdir(folder);
	}		

	return 0;
}

int split(char buffer[], char text[], char delim) {
	int 	i;
	int 	size = strlen(text);
	int 	begin = 0;

	// Desde 'size - 2' me evito el \0 y en caso de ser carpeta, el último '/'
	for(i = (size - 2); 0 < i; i--) {
		if(text[i] == delim) {
			begin = i;
			break;
		}
	}

	if((size - begin) < BUF) {
		buffer[0] = '.';
		for(i = begin; i < size; i++)
			buffer[(i - begin) + 1] = text[i];
		buffer[(i - begin) + 1] = '\0';
		return size - begin;
	} else {
		printf("Buffer overflow\n");
		return -1;
	}
}