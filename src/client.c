

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


int main(int argc, char *argv[]) {
    int socket;
    char client_name[BUF] = "anonymous";
    int port = 2222;
    int opt;
    char *file, fullname[256];
    int fdout = STDOUT;

    system("clear");

    /* parameters */
    while ((opt = getopt(argc, argv, "f:p:n:h")) != -1) {
        switch (opt) {
            case 'f': /* make log file */
                file = optarg;
                break;
            case 'n': /* nickname of the client */
                strncpy(client_name, optarg, strlen(optarg));
                break;
            case 'p': /* set port */
                port = atoi(optarg);
                break;
            case 'h': /* helper */
                printf("\nUsage: client [-f logfile] [-n nickname] [-p port]
                                 host\n");
                printf("Cliente para la recepcion de archivos desde un server
                        escuchando en port.\n");
                printf("\tCommand Summary:\n");
                printf("\t\t -f logfile\t Indica el archivo donde se guarda un
                        log de la transferencia. Por defecto,\n");
                printf("\t\t\t\t el path del archivo es la carpeta del
                        usuario.\n");
                printf("\t\t -h\t\t This help text.\n");
                printf("\t\t -n nickname\t Indica el nickname del cliente. Por
                        defecto es 'anonymous'\n");
                printf("\t\t -p port\t Especifica el puerto de conexion. Por
                        defecto es %d.\n\n", port);
                return 0;
            default: /* default options */
                fprintf(stderr, "Usage: %s [-f logfile] [-n nickname]
                                 [-p port_number] host\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* check for host */
    if (argc <= optind) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

    /* create log file */
    if (file != NULL) {
        sprintf(fullname, "/home/%s/%s", getlogin(), file);
        fdout = open(fullname, O_CREAT | O_RDONLY | O_WRONLY);
        dprintf(fdout, "Recibiendo archivos del servidor.\n\n");
    }

    /* connect to server */
    socket = clientcon(argv[optind], port);
    /* send nickname */
    write_to(socket, client_name, strlen(client_name) + 1);
    printf("\nConectado correctamente al server.\n\n");
    mkfolder(fdout, client_name);
    receive_data(socket, fdout);
    close(socket);

    if (fdout != STDOUT) {
        printf("Log guardado en el archivo '%s'.\n", fullname);
        close(fdout);
    }
    return 0;
}
