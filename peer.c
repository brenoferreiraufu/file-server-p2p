#include <sys/socket.h> // Funções relacionadas a socket
#include <stdlib.h> 
#include <stdio.h>
#include <arpa/inet.h> // Estrutura sockaddr_in 
#include <string.h> 
#include <fcntl.h>
#include <pthread.h> // Funções de thread
#include <unistd.h> // close()
#include "env.h"

#define PORT 49153
#define LISTEN_BACKLOG 11
#define MAX_NUM_THREADS 8

int status, sock_fd; // File descriptor do socket.

void error_handler(const char* fail, const char* success) {
    if (status == ERROR) {
        perror(fail);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", success);
}

void send_file(int client_fd, int fd)
{
    char buffer[BUFFER_SIZE_FILE];
    int bytes_read;

    do
    {
        memset(&buffer, 0, sizeof(buffer));
        bytes_read = read(fd, buffer, BUFFER_SIZE_FILE);
        if (bytes_read > 0)
        {
            send(client_fd, buffer, bytes_read, 0);
        }
    } while (bytes_read > 0);

    close(fd);
}

int request_file(char filename[FILENAME_MAX_LENGTH]) {
    struct sockaddr_in sock_addr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == ERROR) {
        perror("[request_file] Failed to create socket.");
        return ERROR;
    }

    printf("[request_file] Socket created.\n");

    /******************************************/
    /* Inicializa a estrutura de endereço.    */
    /******************************************/

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_family = AF_INET;
    
    /******************************************/
    /* Anexa o endereço local a um socket.    */
    /******************************************/

    if (bind(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
        perror("[request_file] Bind failed.");
        return ERROR;
    }

    struct sockaddr_in tracker_address;
    tracker_address.sin_family = AF_INET;
    tracker_address.sin_port = htons(49152);
    tracker_address.sin_addr.s_addr = inet_addr(TRACKER_IP);

    // TODO Arquivo torrent que terá nome do arquivo e IP do tracker

    //Envia dados.
    //ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
    char msg[10] = "Hello!";
    if ((sendto(sock, msg, sizeof(msg), 0, (struct sockaddr *) &tracker_address, sizeof(tracker_address))) < 0)
        check_falha("Falha sendto()");
    
    //Recebe dados.
    socklen_t tracker_address_len = sizeof(tracker_address);
    char recv_msg[30];
    //ssize_t recvfrom(int sockfd, void *restrict buf, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen);
    if ((recvfrom(sock, recv_msg, sizeof(recv_msg), 0, (struct sockaddr*) &tracker_address, &tracker_address_len)) < 0)
        check_falha("Falha recvfrom()");
}

void menu() {
    // Não será necessário pois o peer vai ler o arquivo "torrent"
    int option = 0;
    char filename[FILENAME_MAX_LENGTH];
    do
    {
        printf("\n1 - Solicitar arquivo"); // SEEK =: Entrada um arquivo SEEK
        printf("\n2 - Compartilhar arquivo"); // POST =: Entrada arquivo POST
        printf("\n3 - Sair");

        scanf("%d", &option);

        if (option == 3) {
            exit(SUCCESS);
        }

        fgets(filename, FILENAME_MAX_LENGTH + 1, stdin);
        filename[strcspn(filename, "\n")] = '\0';

        request_file(filename);
    } while (TRUE);
}

int main(int argc, char const *argv[])
{    
    struct sockaddr_in sock_addr; // Estrutura de dados que armazena o endereço.
    pthread_t ts_id, td_id;
    char buffer[1024];

    /******************************************/
    /* Criando o socket TCP para conexão.     */
    /******************************************/

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd == ERROR) {
        perror("Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    printf("Socket created.\n");

    /******************************************/
    /* Inicializa a estrutura de endereço.    */
    /******************************************/

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_family = AF_INET;
    
    /******************************************/
    /* Anexa o endereço local a um socket.    */
    /******************************************/

    status = bind(sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr));
    error_handler("Bind failed.", "Successful bind.");

    /******************************************/
    /* Escuta requisições de conexão.         */
    /******************************************/
    
    status = listen(sock_fd, LISTEN_BACKLOG);
    error_handler("Listen failed.", "Listen for connections...");

    do
    {
        int client_fd = accept(sock_fd, NULL, NULL);
        int received_characters = 0;

        if (client_fd == -1)
        {
            status = ERROR;
            error_handler("Accept failed.", "Accept..");
        }

        received_characters = recv(client_fd, buffer, BUFFER_SIZE_MSG, 0);

        if (received_characters < 1)
        {
            close(client_fd);
            return;
        }

        char *method, *filename;
        method = strtok(buffer, " ");
        filename = strtok(NULL, " ") + 1;

        if (strcmp(method, "GET") != 0)
        {
            close(client_fd);
            continue;
        }

        int fp = open(filename, O_RDONLY);

        if (fp < 0)
        {
            // TODO enviar msg ao tracker para retirar peer do arquivo
            close(client_fd);
            continue;
        }

        send_file(client_fd, fp);
        close(client_fd);
    } while (TRUE);
    
    return 0;
}
