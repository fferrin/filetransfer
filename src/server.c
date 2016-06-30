

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


int main(int argc, char *argv[]) {
    int sockfd;
    client_t client;
    int opt;
    char *file, fullname[256];
    int fdout = STDOUT;
    int port = 2222;

    system("clear");

    /* parameters */
    while ((opt = getopt(argc, argv, "p:f:h")) != -1) {
        switch (opt) {
            case 'p': /* set port */
                port = atoi(optarg);
                break;
            case 'f': /* make log file */
                file = optarg;
                break;
            case 'h': /* helper */
                printf("\nUsage: server [-f logfile] [-p port] file/dir\n");
                printf("Server para el envio de archivos a un cliente.\n");
                printf("\tCommand Summary:\n");
                printf("\t\t -f logfile\t Indica el archivo donde se guarda un
                        log de la transferencia. Por defecto,\n");
                printf("\t\t\t\t el path del archivo es la carpeta del
                        usuario.\n");
                printf("\t\t -h\t\t This help text.\n");
                printf("\t\t -p port\t Especifica el puerto de conexion. Por
                        defecto es %d.\n\n", port);
                return 0;
            default: /* default options */
                fprintf(stderr, "Usage: %s [-f logfile] [-p port] file/dir\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* check for host */
    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

    /* create log file */
    if (file != NULL) {
        sprintf(fullname, "/home/%s/%s", getlogin(), file);
        fdout = open(fullname, O_CREAT | O_RDONLY | O_WRONLY);
        dprintf(fdout, "Copiando archivos al servidor.\n\n");
    }

    sockfd = servercon(port);
    listen(sockfd, 1);
    printf("\nEsperando la aceptacion...\n\n");

    /* accept client connection */
    client.clilen = sizeof(client.cli_addr);
    client.fd_client = accept(sockfd, (struct sockaddr *) &client.cli_addr,
                              &client.clilen);
    read_from(client.fd_client, client.nickname, SIZE_BUF);
    printf("Conexion aceptada con %s\n", client.nickname);
    printf("\nComenzando el envio de archivos\n");

    send_data(client.fd_client, argv[optind], fdout);
    close(client.fd_client);
    close(sockfd);
    if (fdout != STDOUT) {
        printf("Log guardado en el archivo '%s'.\n", fullname);
        close(fdout);
    }
    return 0;
}
