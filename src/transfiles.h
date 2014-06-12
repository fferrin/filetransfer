
#include <stdio.h>
#include <unistd.h>
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
#include <netdb.h> 
#include <sys/sendfile.h>
#include <dirent.h>		/* "readdir" etc. are defined here. */
#include <limits.h>		/* limits.h defines "PATH_MAX". */

#define HOSTNAME 		"localhost"
#define MAX_SIZE_FILE 	1024000
#define SIZE_BUF 		10240
#define BUF 			128000
#define STDOUT 			1
#define FILE 			0
#define DIRECTORY		1
#define END				2


typedef struct {
	char			name_of_file[SIZE_BUF];
	int 			fd_file;
	unsigned int 	size_file;
	unsigned int	size_string;
} file_t;


typedef struct {
	int 				fd_client;
	char				nickname[SIZE_BUF];
	struct sockaddr_in	cli_addr;
	socklen_t			clilen;
} client_t;


typedef struct {
	int 				sockfd;
	struct sockaddr_in 	serv_addr;
	struct hostent 		*server;	
} conection_t;

/*
int 	FILES_SENDED = 0;
int 	DIR_SENDED = 0;
int 	FILES_RECEIVED = 0;
int 	DIR_RECEIVED = 0;
*/

void error(const char *msg);

int clientcon(char *host, int port);

int servercon(int port);

int read_from(int fd, void *buffer, int size_of_buffer);

int write_to(int fd, void *buffer, int size_of_buffer);

void get_relative_path(char relative[], char absolute[], int position);

int send_file(int fd, char filename[], int position, int fdout);

int receive_file(int socket, int fdout);

int send_data(int socket, char filename[], int fdout);

int receive_data(int socket, int fdout);

int getdate( char date[] );

int mkfolder(int fdout, char clietn[]);

int search_root(char text[]);

int send_dir(int socket, char *dir_name, int position, int fdout);