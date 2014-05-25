
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
#define PORT 			666
#define MAX_SIZE_FILE 	1024000
#define SIZE_BUF 		10240
#define BUF 			128000
#define FILE 			0
#define DIRECTORY		1


typedef struct {
	char			name_of_file[SIZE_BUF];
	char 			relative_path[SIZE_BUF];
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


void error(const char *msg);

int clientcon(char *host, int port);

int servercon(int port);

int read_from(int fd, char *buffer, int size_of_buffer);

int write_to(int fd, char *buffer, int size_of_buffer);

void get_relative_path(char relative[], char absolute[], int position);

int send_file(int fd, char filename[], int position);

int receive_file(int socket);

int send_data(int socket, char filename[]);

int receive_data(int socket);

int getdate( char date[] );

int mkfolder(void);

int search_root(char text[]);

int send_dir(int socket, char *dir_name, int position);