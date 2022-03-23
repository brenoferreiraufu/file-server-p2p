#include <sys/socket.h> // Funções relacionadas a socket
#include <stdlib.h> 
#include <stdio.h>
#include <arpa/inet.h> // Estrutura sockaddr_in 
#include <string.h> 
#include <fcntl.h>
#include <pthread.h> // Funções de thread
#include <unistd.h> // close()
#include <uuid/uuid.h>
#include "env.h"

#define PORT 49153
#define TRACKER_PORT 49154
#define LISTEN_BACKLOG 11
#define MAX_NUM_THREADS 8

char tracker_ip[INET_ADDRSTRLEN] = "127.0.0.1";

void conn_tracker(const char *send_b, char** recv_b) {
    int sockfd;
    int bytes_written, data_length;
    struct sockaddr_in servaddr;

    /************************************************/
    /* Configura a conexão do socket com o tracker. */
    /************************************************/
    
    // printf("Digite IP do tracker:\n");
    // fgets(tracker_ip, INET_ADDRSTRLEN, stdin);
    // tracker_ip[strcspn(tracker_ip, "\n")] = '\0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd == ERROR) {
        perror("[conn_tracker] Failed to create socket.");
        exit(EXIT_FAILURE);
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    inet_pton(AF_INET, tracker_ip, &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(TRACKER_PORT);
    servaddr.sin_family = AF_INET;
    
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("[conn_tracker] connection with the tracker failed...\n");
		close(sockfd);
        exit(EXIT_FAILURE);
    }
	
    /************************************************/
    /* Envia mensagem do buffer para o tracker.     */
    /************************************************/
    
    bytes_written = send(sockfd, send_b, strlen(send_b), 0);

    if (bytes_written == ERROR)
    {
        perror("[conn_tracker] Failed to send message.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /******************************************************/
    /* Lê a resposta do tracker.                          */
    /******************************************************/

    data_length = recv(sockfd, *recv_b, BUFFER_SIZE_MSG, 0);

    if (data_length == ERROR) {
        perror("[conn_tracker] Failed to recieve data.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (data_length == 0) {
        perror("[conn_tracker] Connection closed by tracker.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
        
    close(sockfd);
}

void get_file() {
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char filename[FILENAME_MAX_LENGTH + 4] = {'\0'};
    char *recv_buffer = calloc(BUFFER_SIZE_MSG, 1);

    /******************************************************/
    /* Lê nome do arquivo que vai compartilhar e monta    */
    /* mensagem.                                          */
    /******************************************************/
    
    printf("\nNome do arquivo SEEK: ");
    fgets(filename, FILENAME_MAX_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = '\0';

    /******************************************************/
    /* Abre e lê o arquivo SEEK torrent                   */
    /******************************************************/

    FILE *fp;
    fp = fopen(filename, "r");

    if (fp == NULL) {
        perror("[share_file] Open file failed.");
        exit(EXIT_FAILURE);
    }

    fread(buffer, sizeof(buffer), 1, fp);

    fclose(fp);

    /******************************************************/
    /* Conecta com o tracker envia mensagem e recebe      */ 
    /* respota.                                           */
    /******************************************************/
    
    conn_tracker(buffer, &recv_buffer);

    /******************************************************/
    /* Tenta se conectar com os peers para baixar o       */ 
    /* arquivo.                                           */
    /******************************************************/
}

void share_file() {
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char filename[FILENAME_MAX_LENGTH] = {'\0'};
    char recvfilename[FILENAME_MAX_LENGTH + 4] = {'\0'};    
    char *recv_buffer = calloc(BUFFER_SIZE_MSG, 1);

    /******************************************************/
    /* Lê nome do arquivo que vai compartilhar e monta    */
    /* mensagem.                                          */
    /******************************************************/

    printf("\nNome do arquivo para compartilhar: ");
    fgets(filename, FILENAME_MAX_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = '\0';
    sprintf(buffer, "POST\n%s", filename);
    
    /******************************************************/
    /* Conecta com o tracker envia mensagem e recebe      */ 
    /* respota.                                           */
    /******************************************************/
    
    conn_tracker(buffer, &recv_buffer);

    /******************************************************/
    /* Cria nome do arquivo que vai guardar respota do    */ 
    /* tracker.                                           */
    /******************************************************/

    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse(binuuid, recvfilename);
    strcat(recvfilename, ".txt");

    /******************************************************/
    /* Escreve a resposta do tracker.                     */
    /******************************************************/

    FILE *fp;
    fp = fopen(recvfilename, "w");

    if (fp == NULL) {
        perror("[share_file] Open file failed.");
        exit(EXIT_FAILURE);
    }

    fwrite(recv_buffer, strlen(recv_buffer), sizeof(char), fp);

    fclose(fp);
    free(recv_buffer);
}

void menu() {
    char option[4];
    do
    {
        printf("\n1 - Solicitar arquivo"); // SEEK =: Entrada um arquivo SEEK
        printf("\n2 - Compartilhar arquivo"); // POST =: Entrada arquivo POST
        printf("\n3 - Sair\n");

        printf("\nDigite uma opção: ");
        fgets(option, sizeof(option), stdin);
        option[strcspn(option, "\n")] = '\0';

        if (option[0] == '1') {
            get_file();
        } else if (option[0] == '2') {    
            share_file();
        } else if (option[0] == '3') {
            exit(EXIT_SUCCESS);
        }
    } while (TRUE);
}

int main(int argc, char const *argv[])
{  
    menu();
    return 0;
}
