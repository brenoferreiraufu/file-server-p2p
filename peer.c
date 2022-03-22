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
#define TRACKER_PORT 49154
#define LISTEN_BACKLOG 11
#define MAX_NUM_THREADS 8

int sockfd;
struct sockaddr_in servaddr;
char tracker_ip[INET_ADDRSTRLEN];

void share_file() {
    char buffer[BUFFER_SIZE_MSG];
    char filename[FILENAME_MAX_LENGTH];

    /************************************************/
    /* Configura a conexão do socket com o tracker. */
    /************************************************/
    
    fgets(tracker_ip, INET_ADDRSTRLEN, stdin);
    tracker_ip[strcspn(filename, "\n")] = '\0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd == ERROR) {
        perror("[share_file] Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    printf("[share_file] Socket created.\n");
    
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_addr.s_addr = tracker_ip;
    sock_addr.sin_port = htons(TRACKER_PORT);
    sock_addr.sin_family = AF_INET;
    
    status = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) {
        printf("[share_file] connection with the tracker failed...\n");
		close(sockfd);
        exit(EXIT_FAILURE);
    }
	
    /************************************************/
    /* Envia mensagem POST para o tracker.          */
    /************************************************/
    
    fgets(filename, FILENAME_MAX_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = '\0';

    sprintf(buffer, "POST\n%s", filename);
    bytes_written = send(sockfd, buffer, strlen(buffer), 0);

    if (bytes_written == ERROR)
    {
        perror("[share_file] Failed to send message.\n");
        close(client_sock);
        return;
    }

    /******************************************************/
    /* Lê a resposta do tracker.                          */
    /******************************************************/

    data_length = recv(sockfd, buffer, sizeof(buffer), 0);

    if (data_length == ERROR)
    {
        perror("Failed to recieve data.\n");
        close(sockfd);
        return;
    }

    if (data_length == 0)
    {
        printf("[+] Thread %ld: Closed connection.\n", thread_n);
        close(sockfd);
        return;
    }

    close(sockfd);

    /******************************************************/
    /* Escreve a resposta do tracker.                     */
    /******************************************************/

    
}

void menu() {
    int option = 0;
    do
    {
        printf("\n1 - Solicitar arquivo"); // SEEK =: Entrada um arquivo SEEK
        printf("\n2 - Compartilhar arquivo"); // POST =: Entrada arquivo POST
        printf("\n3 - Sair");

        scanf("%d", &option);

        if (option == 1) {
            // TODO SEEK message para o tracker
        } else if (option == 2) {    
            share_file();
        } else (option == 3) {
            exit(EXIT_SUCCESS);
        }

        request_file(filename);
    } while (TRUE);
}

int main(int argc, char const *argv[])
{  
    menu();
    return 0;
}
