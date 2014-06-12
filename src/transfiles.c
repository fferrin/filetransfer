
#include "transfiles.h"

int DIR_S 	= 0;
int FILES_S = 0;
int DIR_R 	= 0;
int FILES_R = 0;

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

int read_from(int fd, void *buffer, int size_of_buffer) {

	int bytes = -1;

	if(bytes < 0)
		bytes = read(fd, buffer, size_of_buffer);
		
	return bytes;
}

int write_to(int fd, void *buffer, int size_of_buffer) {

	int bytes = -1;

	while(bytes < 0)
		bytes = write(fd, buffer, size_of_buffer);
		
	return bytes;
}

int send_file(int socket, char filename[], int position, int fdout) {
	file_t 			file;
	struct stat 	stat_filename;
	int 			numb_bytes;
	int 			sended = 0;
	char 			type;

	bzero(&file, sizeof(file_t));
	get_relative_path(file.name_of_file, filename, position);

	stat(filename, &stat_filename);

	if(stat_filename.st_mode & S_IFDIR) {
		type = (char) DIRECTORY;
		dprintf(fdout, "\tSending %s... ", filename);

		file.size_string = (unsigned int) strlen(file.name_of_file) + 1;
		write_to(socket, &type, sizeof(char));
		write(socket, &file.size_string, sizeof(file.size_string));
		write(socket, &file.name_of_file, (int)strlen(file.name_of_file) + 1);
		DIR_S++;
		dprintf(fdout, "OK\n");
		return 0;

	} else if(stat_filename.st_mode & S_IFDIR || stat_filename.st_mode & S_IFLNK) {
		type = (char) FILE;

		file.fd_file = open(filename, 0);
		file.size_file = (unsigned int) stat_filename.st_size;
		file.size_string = (unsigned int) strlen(file.name_of_file) + 1;

		dprintf(fdout, "\tSending %s... ", filename);

		// Envio los datos del archivo
		write_to(socket, &type, sizeof(char));
		write_to(socket, &file.size_file, sizeof(file.size_file));
		write_to(socket, &file.size_string, sizeof(file.size_string));
		write_to(socket, &file.name_of_file, (int)strlen(file.name_of_file) + 1);
		while(1) {
			numb_bytes = -1;
			while(numb_bytes < 0) {
				numb_bytes = sendfile(socket, file.fd_file, NULL, SIZE_BUF);
				if(numb_bytes < 0) {
					error("ERROR");
					return errno;
				}
			}
				
			sended += numb_bytes;
			if(sended == file.size_file) {
				break;
			}
		}
		dprintf(fdout, "OK\n");
		//printf("\t\tSended %.3f of %.3f kB [%.3f%%]\n", sended/1000., file.size_file/1000., (sended*100.)/file.size_file);
		FILES_S++;
	
		close(file.fd_file);

		return sended;
	}

	return -1;
}


int send_dir(int socket, char *dir_name, int position, int fdout) {
    DIR *d = NULL;				/* Where result's opendir is saved */

	send_file(socket, dir_name, position, fdout);

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
               			/* Recursively call "list_dir" with the new path. */
			    		send_dir(socket, name, position, fdout);   
			    	/* Por ahora, esto está al pedo */
		    		} else
		    			sprintf(name, "%s%s", dir_name, d_struct->d_name);
			    /* Si no era un directorio */
			    } else {
			    	/* Imprimo el nombre del mismo */
			    	send_file(socket, name, position, fdout);
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

int send_data(int socket, char filename[], int fdout) {
	struct stat file_stat;
	char type = END;

	if(stat(filename, &file_stat) < 0)
		return -1;

	if(file_stat.st_mode & S_IFDIR) {
		send_dir(socket, filename, search_root(filename), fdout);
		write(socket, &type, sizeof(char));
	} else if(file_stat.st_mode & S_IFREG || file_stat.st_mode & S_IFLNK) {
		send_file(socket, filename, search_root(filename), fdout);
		write(socket, &type, sizeof(char));
	} else {
		dprintf(fdout, "It can't be sended that kind of file\n");
		dprintf(fdout, "Quitting...\n");
		return -1;
	}
	dprintf(fdout, "\nSe han enviado: - %d directorios\n", DIR_S);
	dprintf(fdout, "                - %d archivos\n", FILES_S);
	dprintf(fdout, "\nTransferencia realizada con éxito.\n");

	write_to(socket, &DIR_S, sizeof(int));
	write_to(socket, &FILES_S, sizeof(int));

	return 0;
}


int receive_file(int socket, int fdout) {
	file_t 		file;
	char 		buffer[SIZE_BUF];
	int 		numb_bytes;
	int 		recibed = 0;
	char 		type;
	int 		offset;

	bzero(&file, sizeof(file_t));
	// Leo el tamaño del archivo
	read_from(socket, &type, sizeof(char));

	if(type == (char) FILE) { 
		read_from(socket, &file.size_file, sizeof(int));
		read_from(socket, &file.size_string, sizeof(int));
		read_from(socket, &file.name_of_file, file.size_string);

		offset = file.size_file;

		dprintf(fdout, "\tReceiving '%s'...", file.name_of_file);

		if((file.fd_file = creat(file.name_of_file, S_IRWXU)) < 0)
			error("ERROR");

		while(1) {
			bzero(buffer, SIZE_BUF);
			if(SIZE_BUF < offset) {
				if((numb_bytes = read(socket, buffer, SIZE_BUF)) < 0) {
					dprintf(fdout, "\tbytes = %d\n", numb_bytes);
					error("\tNo se pudo leer el archivo: ");
					break;
				}
			} else {
				if((numb_bytes = read(socket, buffer, offset)) < 0) {
					dprintf(fdout, "\tbytes = %d\n", numb_bytes);
					error("\tNo se pudo leer el archivo: ");
					break;
				}
			}
			write(file.fd_file, buffer, numb_bytes);
			offset -= numb_bytes;
			recibed += numb_bytes;
			//printf("Recibidos: %.3fkB (%.3f/%.3f) kB\n", numb_bytes/1000., recibed/1000., file.size_file/1000.);
			if(offset <= 0)
				break;
		}
		
		close(file.fd_file);

		if(recibed != file.size_file) {
			error("No se pudo recibir el archivo.");
			return -1;
		} else {
			dprintf(fdout, " %.3f/%.3f kB [%.3f%%]\n", recibed/1000., file.size_file/1000., (recibed*100.)/file.size_file);
			FILES_R++;
			return recibed;
		}
	} else if(type == (char) DIRECTORY) {
		read_from(socket, &file.size_string, sizeof(int));
		read_from(socket, &file.name_of_file, file.size_string);

		mkdir(file.name_of_file, 0700);
		dprintf(fdout, "\tCreating '%s'...\n", file.name_of_file);
		DIR_R++;
		return 0;
	} else if(type == (char) END)
		return 2;

	return -1;
}

int receive_data(int socket, int fdout) {
	int aux;
	int dir, files;

	while(1) {
		aux = receive_file(socket, fdout);
		
		if(aux == 2) {
			read_from(socket, &dir, sizeof(int));
			read_from(socket, &files, sizeof(int));
			if((DIR_R == dir) && (FILES_R == files)) {
				dprintf(fdout, "\nTransferencia realizada con exito.\n");
				dprintf(fdout, "\nSe han transferido: - %d de %d carpetas\n", DIR_R, dir);
				dprintf(fdout, "                    - %d de %d archivos\n", FILES_R, files);
			} else {
				dprintf(fdout, "\nAlgunos archivos no se pudieron transferir.\n");
				dprintf(fdout, "\nSe han transferido: - %d carpetas\n", DIR_R);
				dprintf(fdout, "                    - %d archivos\n", FILES_R);
			}
			break;
		} else if(aux < 0) {
			dprintf(fdout, "\nSe ha producido un error en la transferencia\n");
			return -1;
		}
	}
	return 0;
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

int mkfolder(int fdout, char client[]) {
	char folder[BUF];
	char *name = NULL;
	char date[128];

	name = getlogin();
	getdate(date);

	sprintf(folder, "/home/%s/transfers/", name);
	
	if( mkdir(folder, 0700) < 0 ) {
		if( errno == 17 ) {
			sprintf(folder, "/home/%s/transfers/%s - %s/", name, date, client);
			dprintf(fdout, "\tCreating '%s'...\n", folder);
			mkdir(folder, 0700);
			chdir(folder);
		} else {
			dprintf(fdout, "ERROR%d\n", errno);
			return errno;
		}
	} else {
		dprintf(fdout, "\tCreating '%s'...\n", folder);
		mkdir(folder, 0700);
		sprintf(folder, "/home/%s/transfers/%s - %s/", name, date, client);
		dprintf(fdout, "\tCreating '%s'...\n", folder);
		mkdir(folder, 0700);
		chdir(folder);
	}		

	return 0;
}

int search_root(char text[]) {
	int 	i;
	int 	size = strlen(text);

	for(i = (size - 2) ; 0 < i ; i--) {
		if(text[i] == '/')
			return i;
	}

	return -1;
}

void get_relative_path(char relative[], char absolute[], int position) {
	int i;

	bzero(relative, strlen(relative));
	relative[0] = '.';

	for( i = position ; i < strlen(absolute) ; i++)
		relative[(i - position) + 1] = absolute[i];
}