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
#define ADD_PEER_SUCCESS "SUCCESS\npeer-added\n"

typedef struct file_info {
    char filename[FILENAME_MAX_LENGTH];
    char id[UUID_STR_LEN];
} file_info;

long num_threads = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int seederfd, finfo_size = 0;
file_info finfo[30];

void *peer_conn(void* arg) {
    int client_sock;
    int bytes_read;
    int data_length = 0;
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char fbuffer[BUFFER_SIZE_FILE] = {'\0'};
    int index_file = 0;

    do {
        client_sock = accept(seederfd, NULL, NULL);

        if (client_sock == ERROR) {
            perror("[peer_seeder] Accept failed.");
            continue;
        }

        data_length = recv(client_sock, buffer, BUFFER_SIZE_MSG, 0);

        if (data_length == ERROR)
        {
            perror("[peer_conn] Failed to recieve data.");
            close(client_sock);
            continue;
        }

        if (data_length == 0)
        {
            perror("[peer_conn] Connection closed by tracker.");
            close(client_sock);
            continue;
        }

        for (index_file = 0; index_file < finfo_size; index_file++)
        {
            if (!strcmp(finfo[index_file].filename, buffer))
                break;
        }

        if (index_file >= finfo_size) {
            perror("[peer_conn] File not found.");

            int bytes_written = send(client_sock, FILE_NOT_FOUND, strlen(FILE_NOT_FOUND), 0);

            if (bytes_written == ERROR)
            {
                perror("[peer_conn] Failed to send message.");
            }

            close(client_sock);
            continue;
        }

        int fp = open(finfo[index_file].filename, O_RDONLY);

        do
        {
            memset(&fbuffer, 0, sizeof(fbuffer));
            bytes_read = read(fp, fbuffer, BUFFER_SIZE_FILE);
            if (bytes_read > 0)
            {
                printf("[send_file] send....\n");
                send(client_sock, fbuffer, bytes_read, 0);
            } else {
                printf("[send_file] bytes read <= 0\n");
            }
        } while (bytes_read > 0);

        close(fp);
        
        close(client_sock);
    } while(TRUE);
    
    pthread_exit(NULL);
}

void *peer_seeder(void* arg) {
    int status;
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
        close(seederfd);
        perror("[peer_seeder] Listen Failed");
        pthread_exit(NULL);
    }

    do
    {
        status = pthread_create(&threads[num_threads], NULL, peer_conn, (void *) num_threads);

        if (status != SUCCESS) {
            perror("[peer_seeder] failed to create thread.");
            continue;
        }

        status = pthread_detach(threads[num_threads]);
        
        if (status != SUCCESS) {
            perror("[peer_seeder] failed to detach thread.");
            continue;
        }

    } while (++num_threads < MAX_NUM_THREADS);

    while(TRUE);
}

void conn_tracker(const char *send_b, char **recv_b, char *ip_tracker)
{
    int sockfd;
    int bytes_written, data_length;
    struct sockaddr_in servaddr;

    /************************************************/
    /* Configura a conexão do socket com o tracker. */
    /************************************************/

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == ERROR)
    {
        perror("[conn_tracker] Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    inet_pton(AF_INET, ip_tracker, &(servaddr.sin_addr.s_addr));
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
    char add_msg[BUFFER_SIZE_MSG] = {'\0'};
    char seekfile[FILENAME_MAX_LENGTH + 4] = {'\0'};
    char fbuffer[BUFFER_SIZE_FILE] = {'\0'};
    int bytes_read;
    char *filename, *id, *address;
    char *recv_buffer = calloc(BUFFER_SIZE_MSG, 1);
    int sockfd;
    int bytes_written;
    struct sockaddr_in peeraddr;

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

    strtok(buffer, "\n");
    strtok(NULL, "\n");
    char *ip_tracker = strtok(NULL, "\n");

    /******************************************************/
    /* Conecta com o tracker envia mensagem e recebe      */
    /* respota.                                           */
    /******************************************************/

    conn_tracker(buffer, &recv_buffer, ip_tracker);

    /******************************************************/
    /* Tenta se conectar com os peers para baixar o       */
    /* arquivo.                                           */
    /******************************************************/

    filename = strtok(recv_buffer, "\n");
    id = strtok(NULL, "\n");
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
            address = strtok(NULL, "\n");
            continue;
        }

        /******************************************************/
        /* Recebe o arquivo do peer                           */
        /******************************************************/
        FILE *file = fopen(filename, "w");

        if (file == NULL)
        {
            perror("[get_file] File write dont work.\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        do
        {
            bytes_read = recv(sockfd, fbuffer, BUFFER_SIZE_FILE, 0);

            if (bytes_read == ERROR)
            {
                perror("[get_file] Failed to recieve file.");
                fclose(file);
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if (bytes_read <= 0)
            {
                fclose(file);
                break;
            }

            bytes_written = fwrite(fbuffer, sizeof(char), strlen(fbuffer), file);
            
            if (ferror(file))
            {
                perror("[get_file] ERROR writing to file\n");
                fclose(file);
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if (bytes_written <= 0)
            {
                perror("[get_file] Failed to write data.\n");
                fclose(file);
                close(sockfd);
                exit(EXIT_FAILURE);
            }

        } while (TRUE);

        close(sockfd);

        printf("[get_file] File received >>%s<<\n", filename);

        sprintf(add_msg, "ADD\n%s", id);
        memset(&recv_buffer, 0, sizeof(recv_buffer));
        conn_tracker(add_msg, &recv_buffer, ip_tracker);

        if (!strcmp(recv_buffer, ADD_PEER_SUCCESS)) {
            printf("[get_file] PEER ADICIONADO\n");
            printf("%s\n", recv_buffer);
        } else {
            printf("[get_file] PEER NÃO ADICIONADO\n");
        }
        
        break;
    }
}

void share_file()
{
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char filename[FILENAME_MAX_LENGTH] = {'\0'};
    char recvfilename[FILENAME_MAX_LENGTH + 4] = {'\0'};
    char *recv_buffer = calloc(BUFFER_SIZE_MSG, 1);
    char tracker_ip[INET_ADDRSTRLEN] = {'\0'};

    /* Create table; no error checking is performed. */

    /******************************************************/
    /* Lê nome do arquivo que vai compartilhar e monta    */
    /* mensagem.                                          */
    /******************************************************/

    printf("\nNome do arquivo para compartilhar: ");
    fgets(filename, FILENAME_MAX_LENGTH, stdin);
    filename[strcspn(filename, "\n")] = '\0';
    sprintf(buffer, "POST\n%s", filename);

    printf("Digite IP do tracker: ");
    fgets(tracker_ip, INET_ADDRSTRLEN, stdin);
    tracker_ip[strcspn(tracker_ip, "\n")] = '\0';

    /******************************************************/
    /* Conecta com o tracker envia mensagem e recebe      */
    /* respota.                                           */
    /******************************************************/

    conn_tracker(buffer, &recv_buffer, tracker_ip);

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
