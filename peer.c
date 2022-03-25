#include <sys/socket.h> // Funções relacionadas a socket
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h> // Estrutura sockaddr_in
#include <string.h>
#include <fcntl.h>
#include <pthread.h> // Funções de thread
#include <unistd.h>  // close()
#include <uuid/uuid.h>
#include "env.h"

#define PORT 49153
#define TRACKER_PORT 49154
#define LISTEN_BACKLOG 11
#define MAX_NUM_THREADS 8
#define FILE_NOT_FOUND "FAIL\nnot-found\n"
#define ADD_PEER_MSG "ADD\n"

typedef struct file_info {
    char filename[FILENAME_MAX_LENGTH];
    char id[UUID_STR_LEN];
} file_info;

char tracker_ip[INET_ADDRSTRLEN] = "192.168.100.192";
long num_threads = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int seederfd, finfo_size = 0;
file_info finfo[30];

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

void recv_file(int peer_fd, int fd)
{
    char buffer[BUFFER_SIZE_FILE];
    int bytes_read;

    do
    {
        memset(&buffer, 0, sizeof(buffer));
        bytes_read = recv(peer_fd, buffer, BUFFER_SIZE_MSG, 0);
        if (bytes_read > 0)
        {
            print("[recv_file] write....\n");
            write(fd, buffer, bytes_read);
        }
    } while (bytes_read > 0);

    close(fd);
}

void *peer_conn(void* arg) {
    int client_fd = (long) arg;
    int data_length = 0;
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    int index_file = 0;

    data_length = recv(client_fd, buffer, BUFFER_SIZE_MSG, 0);

    if (data_length == ERROR)
    {
        perror("[conn_tracker] Failed to recieve data.");
        close(client_fd);
        pthread_mutex_lock(&mutex);
        num_threads--;
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    if (data_length == 0)
    {
        perror("[conn_tracker] Connection closed by tracker.");
        close(client_fd);
        pthread_mutex_lock(&mutex);
        num_threads--;
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    for (index_file = 0; index_file < finfo_size; index_file++)
    {
        if (!strcmp(finfo[index_file].id, buffer))
            break;
    }
    
    if (index_file >= finfo_size) {
        perror("[conn_tracker] File not found.");

        int bytes_written = send(client_fd, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);

        if (bytes_written == ERROR)
        {
            perror("[conn_tracker] Failed to send message.");
        }


        close(client_fd);
        pthread_mutex_lock(&mutex);
        num_threads--;
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    int fp = open(finfo[index_file].filename, O_RDONLY);

    send_file(client_fd, fp);

    pthread_mutex_lock(&mutex);
    num_threads--;
    pthread_mutex_unlock(&mutex);
    
    close(client_fd);
    pthread_exit(NULL);
}

void *peer_seeder(void* arg) {
    int status, client_sock;
    struct sockaddr_in sock_addr;
    pthread_t threads[MAX_NUM_THREADS];

    /******************************************/
    /* Criando o socket UDP para conexão.     */
    /******************************************/

    seederfd = socket(AF_INET, SOCK_STREAM, 0);

    if (seederfd == ERROR) {
        perror("[peer_seeder] Failed to create socket.");
        pthread_exit(NULL);
    }

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

    status = bind(seederfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr));
    if (status == ERROR) {
        close(seederfd);
        perror("[peer_seeder] Bind fail.");
        pthread_exit(NULL);
    }

    status = listen(seederfd, LISTEN_BACKLOG);
    if (status == ERROR) {
        perror("[peer_seeder] Listen Failed");
        pthread_exit(NULL);
    }

    do
    {
        pthread_mutex_lock(&mutex);
        if (num_threads >= MAX_NUM_THREADS) {
            continue;
        }
        pthread_mutex_unlock(&mutex);

        client_sock = accept(seederfd, NULL, NULL);

        if (client_sock == ERROR) {
            perror("[peer_seeder] Accept failed.");
            continue;
        }

        pthread_mutex_lock(&mutex);
        status = pthread_create(&threads[num_threads], NULL, peer_conn, (void *) &client_sock);

        if (status != SUCCESS) {
            perror("[peer_seeder] failed to create thread.");
            close(client_sock);
            continue;
        }

        num_threads++;

        status = pthread_detach(threads[num_threads]);
        
        if (status != SUCCESS) {
            perror("[peer_seeder] failed to detach thread.");
            close(client_sock);
            continue;
        }

        pthread_mutex_unlock(&mutex);

    } while (TRUE);
}

void conn_tracker(const char *send_b, char **recv_b)
{
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

    if (sockfd == ERROR)
    {
        perror("[conn_tracker] Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    inet_pton(AF_INET, tracker_ip, &(servaddr.sin_addr.s_addr));
    servaddr.sin_port = htons(TRACKER_PORT);
    servaddr.sin_family = AF_INET;

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
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

    if (data_length == ERROR)
    {
        perror("[conn_tracker] Failed to recieve data.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (data_length == 0)
    {
        perror("[conn_tracker] Connection closed by tracker.");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
}

void get_file()
{
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char seekfile[FILENAME_MAX_LENGTH + 4] = {'\0'};
    char *filename, *id, *address;
    char *recv_buffer = calloc(BUFFER_SIZE_MSG, 1);
    int sockfd;
    int bytes_written, data_length;
    struct sockaddr_in peeraddr;
    struct sockaddr_in trackeraddr;

    memset(&trackeraddr, 0, sizeof(trackeraddr));
    inet_pton(AF_INET, tracker_ip, &(trackeraddr.sin_addr.s_addr));
    trackeraddr.sin_port = htons(TRACKER_PORT);
    trackeraddr.sin_family = AF_INET;

    /******************************************************/
    /* Lê nome do arquivo que vai compartilhar e monta    */
    /* mensagem.                                          */
    /******************************************************/

    printf("\nNome do arquivo SEEK: ");
    fgets(seekfile, FILENAME_MAX_LENGTH, stdin);
    seekfile[strcspn(seekfile, "\n")] = '\0';

    /******************************************************/
    /* Abre e lê o arquivo SEEK torrent                   */
    /******************************************************/

    FILE *fp;
    fp = fopen(seekfile, "r");

    if (fp == NULL)
    {
        perror("[get_file] Open file failed.");
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

    filename = strtok(recv_buffer, "\n");
    puts(filename);
    id = strtok(NULL, "\n");
    puts(id);
    address = strtok(NULL, "\n");

    while (strcmp(address, "END"))
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd == ERROR)
        {
            perror("[get_file] Failed to create socket.");
            exit(EXIT_FAILURE);
        }

        memset(&peeraddr, 0, sizeof(peeraddr));
        inet_pton(AF_INET, address, &(peeraddr.sin_addr.s_addr));
        peeraddr.sin_port = htons(PORT);
        peeraddr.sin_family = AF_INET;

        if (connect(sockfd, (struct sockaddr *)&peeraddr, sizeof(peeraddr)) < 0)
        {
            printf("[get_file] connection with peer failed...\n");
            address = strtok(NULL, "\n");
            continue;
        }

        /*********************************************************/
        /* Envia mensagem com o nome do arquivo para o peer.     */
        /*********************************************************/

        bytes_written = send(sockfd, filename, strlen(filename), 0);

        if (bytes_written == ERROR)
        {
            perror("[get_file] Failed to send filname message.");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        /******************************************************/
        /* Recebe o arquivo do peer                           */
        /******************************************************/

        int fp = open(filename, O_WRONLY);

        recv_file(sockfd, fp);
        printf("[get_file] File received >>%s<<\n", filename);

        // TODO CRIAR OUTRO SOCKER PRA COMUNICAR COM O TRACKER \/ ERRO
        if (connect(sockfd, (struct sockaddr *)&trackeraddr, sizeof(trackeraddr)) < 0)
        {
            printf("[get_file] connection with tracker failed, cannot add peer...\n");
            close(sockfd);
            break;
        }

        bytes_written = send(sockfd, ADD_PEER_MSG, strlen(ADD_PEER_MSG), 0);

        if (bytes_written == ERROR)
        {
            perror("[get_file] Failed to send ADD_PEER_MSG message.");
            close(sockfd);
            break;
        }

        /******************************************************/
        /* Lê a resposta do tracker "OK".                     */
        /******************************************************/

        data_length = recv(sockfd, recv_buffer, BUFFER_SIZE_MSG, 0);

        if (data_length == ERROR)
        {
            perror("[get_file] Failed to recieve ADD_PEER response from tracker.");
            close(sockfd);
            break;
        }

        puts(recv_buffer);

        close(sockfd);
    }
}

void share_file()
{
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char filename[FILENAME_MAX_LENGTH] = {'\0'};
    char recvfilename[FILENAME_MAX_LENGTH + 4] = {'\0'};
    char *recv_buffer = calloc(BUFFER_SIZE_MSG, 1);

    /* Create table; no error checking is performed. */

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

    if (fp == NULL)
    {
        perror("[share_file] Open file failed.");
        exit(EXIT_FAILURE);
    }

    fwrite(recv_buffer, strlen(recv_buffer), sizeof(char), fp);

    fclose(fp);

    strtok(recv_buffer, "\n");
    char *id = strtok(NULL, "\n");
    strcpy(finfo[finfo_size].filename, filename);
    strcpy(finfo[finfo_size].id, id);
    finfo_size++;

    free(recv_buffer);
}

void menu()
{
    char option[4];
    do
    {
        printf("\n1 - Solicitar arquivo");    // SEEK =: Entrada um arquivo SEEK
        printf("\n2 - Compartilhar arquivo"); // POST =: Entrada arquivo POST
        printf("\n3 - Sair\n");

        printf("\nDigite uma opção: ");
        fgets(option, sizeof(option), stdin);
        option[strcspn(option, "\n")] = '\0';

        if (option[0] == '1')
        {
            get_file();
        }
        else if (option[0] == '2')
        {
            share_file();
        }
        else if (option[0] == '3')
        {
            exit(EXIT_SUCCESS);
        }
    } while (TRUE);
}

int main(int argc, char const *argv[])
{
    int status;
    pthread_t tid;

    status = pthread_create(&tid, NULL, peer_seeder, NULL);

    if (status != SUCCESS) {
        perror("[main] failed to create thread.\n");
        exit(EXIT_FAILURE);
    }

    status = pthread_detach(tid);
    
    if (status != SUCCESS) {
        perror("[main] pthread detach failed.");
    }

    menu();
    return 0;
}
