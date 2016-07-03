
// TODO: Make only send_dir and send_file and put in send_data function
#include "transfiles.h"

int DIR_S = 0;
int DIR_R = 0;
int FILES_S = 0;
int FILES_R = 0;

/* Print error on standar output
 * @param msg: message error displayed
 *
 * @return: integer with error value
 */
void error(const char *msg) {
    perror(msg);
    exit(1);
}

/* Create client connection to server
 * @param host: server address
 * @param port: server port
 *
 * @return: file descriptor of the client socket
 */
int clientcon(char *host, int port) {
    conection_t conection;

    if ((conection.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("ERROR opening socket");
        return -1;
    }

    if ((conection.server = gethostbyname(host)) == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return -1;
    }

    bzero((char *) &conection.serv_addr, sizeof(conection.serv_addr));
    conection.serv_addr.sin_family = AF_INET;
    bcopy((char *)conection.server->h_addr, \
          (char *)&conection.serv_addr.sin_addr.s_addr, \
          conection.server->h_length);
    conection.serv_addr.sin_port = port;

    /* try to connect to server */
    if (connect(conection.sockfd, \
                (struct sockaddr *) &conection.serv_addr, \
                sizeof(conection.serv_addr)) < 0)
        error("ERROR connecting");

    return conection.sockfd;
}

/* Create server
 * @param port: server port
 *
 * @return: file descriptor of the server socket
 */
int servercon(int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    int val = 1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("ERROR opening socket");
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = port;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* port assignment for binding */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
        return -1;
    }

    return sockfd;
}

/* Auxiliary function for read bytes from given file descriptor
 * @param fd: file descriptor to read
 * @param buffer: buffer where bytes are stored
 * @param size_of_buffer: size of the buffer
 *
 * @return: return bytes readed
 */
int read_from(int fd, void *buffer, int size_of_buffer) {
    int bytes = -1;
    // VER SI NO VA UN WHILE
    if (bytes < 0)
        bytes = read(fd, buffer, size_of_buffer);

    return bytes;
}

/* Auxiliary function for write bytes in given file descriptor
 * @param fd: file descriptor to write
 * @param buffer: buffer where bytes are stored
 * @param size_of_buffer: size of the buffer
 *
 * @return: return bytes written
 */
int write_to(int fd, void *buffer, int size_of_buffer) {
    int bytes = -1;
    while (bytes < 0)
        bytes = write(fd, buffer, size_of_buffer);

    return bytes;
}

/* Send file through socket
 * @param socket: file descriptor of client
 * @param filename: name of the file
 * @param position: index of the filename in absolute path
 * @param fdout: file descriptor to write
 *
 * @return: return bytes sended
 */
int send_file(int socket, char filename[], int position, int fdout) {
    file_t file;
    struct stat stat_filename;
    int numb_bytes;
    int sended = 0;
    char type;

    bzero(&file, sizeof(file_t));
    get_relative_path(file.name_of_file, filename, position);
    stat(filename, &stat_filename);

    /* if is a directory, send only the name of the directory */
    if (stat_filename.st_mode & S_IFDIR) {
        type = (char) DIRECTORY;
        dprintf(fdout, "\tSending %s... ", filename);

        file.size_string = (unsigned int) strlen(file.name_of_file) + 1;
        /* tells server we are sending a directory */
        write_to(socket, &type, sizeof(char));
        write(socket, &file.size_string, sizeof(file.size_string));
        write(socket, &file.name_of_file, (int) strlen(file.name_of_file) + 1);
        DIR_S++;
        dprintf(fdout, "OK\n");
        return 0;
    /* if is a file */
    } else if (stat_filename.st_mode & S_IFDIR || \
               stat_filename.st_mode & S_IFLNK) {
        type = (char) FILE;

        /* open file to read */
        file.fd_file = open(filename, O_RDONLY);
        file.size_file = (unsigned int) stat_filename.st_size;
        file.size_string = (unsigned int) strlen(file.name_of_file) + 1;

        dprintf(fdout, "\tSending %s... ", filename);

        /* tells server we are sending a file */
        write_to(socket, &type, sizeof(char));
        write_to(socket, &file.size_file, sizeof(file.size_file));
        write_to(socket, &file.size_string, sizeof(file.size_string));
        write_to(socket, &file.name_of_file, (int)strlen(file.name_of_file) + 1);

        while (TRUE) {
            numb_bytes = sendfile(socket, file.fd_file, NULL, SIZE_BUF);
            if (numb_bytes < 0) {
                error("ERROR");
                return errno;
            }

            sended += numb_bytes;
            if (sended == file.size_file)
                break;
        }
        dprintf(fdout, "OK\n");
        /*
        printf("\t\tSended %.3f of %.3f kB [%.3f%%]\n",
                    sended/1000., file.size_file/1000.,
                    (sended*100.)/file.size_file);
        */
        FILES_S++;
        close(file.fd_file);
        return sended;
    }
    return -1;
}

/* Send file through socket
 * @param socket: file descriptor of client
 * @param filename: name of the file
 * @param position: index of the filename in absolute path
 * @param fdout: file descriptor to write
 *
 * @return: 0 on success. Error code otherwise
 */
int send_dir(int socket, char *dir_name, int position, int fdout) {
    DIR *d = NULL;

    send_file(socket, dir_name, position, fdout);

    /* Check it was opened. -1 on error */
    if (!(d = opendir (dir_name))) {
        fprintf (stderr, "Cannot open directory '%s': %s\n", \
                 dir_name, strerror (errno));
        return -1;
    } else {
        while (TRUE) {
            struct dirent *d_struct;
            char name[PATH_MAX];

            /* readdir gets subsequent entries from "d" until the end (NULL) */
            if ((d_struct = readdir(d))) {
                sprintf(name, "%s%s", dir_name, d_struct->d_name);
                if (d_struct->d_type == DT_DIR) {
                    if (strcmp(d_struct->d_name, ".") && \
                        strcmp(d_struct->d_name, "..")) {
                        sprintf(name, "%s%s/", dir_name, d_struct->d_name);
                        send_dir(socket, name, position, fdout);
                    } else
                        /* SEE THIS */
                        sprintf(name, "%s%s", dir_name, d_struct->d_name);
                } else
                    send_file(socket, name, position, fdout);
            } else
                break;
        }

        if (closedir(d)) {
            fprintf (stderr, "Could not close '%s': %s\n", \
                     dir_name, strerror (errno));
            exit (EXIT_FAILURE);
        }
    }
    return 0;
}

/* Send data through socket
 * @param socket: file descriptor of client
 * @param filename: name of the file
 * @param fdout: file descriptor to write
 *
 * @return: 0 on success. Error code otherwise
 */
int send_data(int socket, char filename[], int fdout) {
    struct stat file_stat;
    char type = END;

    if (stat(filename, &file_stat) < 0)
        return -1;

    if (file_stat.st_mode & S_IFDIR) {
        send_dir(socket, filename, search_root(filename), fdout);
        write(socket, &type, sizeof(char));
    } else if (file_stat.st_mode & S_IFREG || file_stat.st_mode & S_IFLNK) {
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

/* Receive file through socket
 * @param socket: file descriptor of server
 * @param fdout: file descriptor to write
 *
 * @return: -1 on error. Positive integer otherwise
 */
int receive_file(int socket, int fdout) {
    file_t file;
    char buffer[SIZE_BUF];
    int numb_bytes;
    int recibed = 0;
    char type;
    int offset;
    int read_until;

    /* read type of file to receive */
    bzero(&file, sizeof(file_t));
    read_from(socket, &type, sizeof(char));
    read_from(socket, &file.size_string, sizeof(int));
    read_from(socket, &file.name_of_file, file.size_string);

    /* receiving file */
    if (type == (char) FILE) {
        read_from(socket, &file.size_file, sizeof(int));
        offset = file.size_file;
        dprintf(fdout, "\tReceiving '%s'...", file.name_of_file);

        /* create file with user permissions */
        if ((file.fd_file = creat(file.name_of_file, S_IRWXU)) < 0)
            error("ERROR");

        while (TRUE) {
            bzero(buffer, SIZE_BUF);
            if (SIZE_BUF < offset)
                read_until = SIZE_BUF;
            else
                read_until = offset;

            if ((numb_bytes = read(socket, buffer, read_until)) < 0) {
                dprintf(fdout, "\tbytes = %d\n", numb_bytes);
                error("\tNo se pudo leer el archivo: ");
                break;
            }
            /* write readed bytes in file */
            write(file.fd_file, buffer, numb_bytes);
            offset -= numb_bytes;
            recibed += numb_bytes;
            /* printf("Recibidos: %.3fkB (%.3f/%.3f) kB\n", numb_bytes/1000.,
                       recibed/1000., file.size_file/1000.); */
            if (offset <= 0)
                break;
        }
        close(file.fd_file);

        if (recibed != file.size_file) {
            error("No se pudo recibir el archivo.");
            return -1;
        } else {
            dprintf(fdout, " %.3f/%.3f kB [%.3f%%]\n", recibed/1000., \
                    file.size_file/1000., (recibed*100.)/file.size_file);
            FILES_R++;
            return recibed;
        }
    /* receiving directory */
    } else if (type == (char) DIRECTORY) {
        mkdir(file.name_of_file, 0700);
        dprintf(fdout, "\tCreating '%s'...\n", file.name_of_file);
        DIR_R++;
        return 0;
    /* end transmission */
    } else if (type == (char) END) {
        return 2;
    }
    return -1;
}

/* Receive data through socket
 * @param socket: file descriptor of server
 * @param fdout: file descriptor to write
 *
 * @return: -1 on error. Positive integer otherwise
 */
int receive_data(int socket, int fdout) {
    int aux;
    int dir, files;

    while (TRUE) {
        aux = receive_file(socket, fdout);
        if (aux == END) {
            read_from(socket, &dir, sizeof(int));
            read_from(socket, &files, sizeof(int));
            if ((DIR_R == dir) && (FILES_R == files)) {
                dprintf(fdout, "\nTransferencia realizada con exito.\n");
                dprintf(fdout, "\nSe han transferido: - %d de %d carpetas\n", \
                        DIR_R, dir);
                dprintf(fdout, "                    - %d de %d archivos\n", \
                        FILES_R, files);
            } else {
                dprintf(fdout, \
                        "\nAlgunos archivos no se pudieron transferir.\n");
                dprintf(fdout, "\nSe han transferido: - %d carpetas\n", DIR_R);
                dprintf(fdout, "                    - %d archivos\n", FILES_R);
            }
            break;
        } else if (aux < 0) {
            dprintf(fdout, "\nSe ha producido un error en la transferencia.\n");
            return -1;
        }
    }
    return 0;
}

/* Get actual date and time
 * @param date: string with actual date
 *
 * @return: 0 on success. Error code otherwise
 */
int getdate(char date[]) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (sprintf(date, "%04d-%02d-%02d %02d:%02d:%02d", \
                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, \
                timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, \
                timeinfo->tm_sec) != 20)
        return -1;

    return 0;
}

/* Make folder in '/home/<user>/transfers/' with actual date as name
 * @param fdout: file descriptor to write
 * @param client: client name
 *
 * @return: 0 on success. Error code otherwise
 */
int mkfolder(int fdout, char client[]) {
    char folder[BUF];
    char *name = NULL;
    char date[128];

    name = getlogin();
    getdate(date);
    sprintf(folder, "/home/%s/transfers/", name);

    if (mkdir(folder, 0700) < 0) {
        /* folder exists? */
        if (errno == 17) {
            sprintf(folder, "/home/%s/transfers/%s - %s/", name, date, client);
            dprintf(fdout, "\tCreating '%s'...\n", folder);
            mkdir(folder, 0700);
            chdir(folder);
            return 0;
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
        return 0;
    }
}

/* Search root path of the given file path
 * @param text: absolute path of the file
 *
 * @return: root path position. -1 otherwise
 */
int search_root(char text[]) {
    int i;
    int size = strlen(text);

    for (i = (size - 2); 0 < i; i--)
        if (text[i] == '/')
            return i;

    return -1;
}

/* Make relative path from absolute path
 * @param relative: relative path of the file
 * @param absolute: absolute path of the file
 * @param position: index of begin of relative path in absolute path
 */
void get_relative_path(char relative[], char absolute[], int position) {
    int i;

    bzero(relative, strlen(relative));
    relative[0] = '.';
    for (i = position; i < strlen(absolute); i++)
        relative[(i - position) + 1] = absolute[i];
}
