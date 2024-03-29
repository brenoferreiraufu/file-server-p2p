#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "env.h"
#include "session.h"

#define PORT 49154
#define MAX_NUM_THREAD 8
#define LISTEN_BACKLOG 12

#define LIST_INSERT_FAILED "FAIL\nno-space\n"
#define TORRENT_NOT_FOUND "FAIL\nnot-found\n"
#define ADD_PEER_SUCCESS "SUCCESS\npeer-added\n"
#define REMOVE_PEER_SUCCESS "SUCCESS\npeer-removed\n"

int sock, status;
const char *tracker_ip;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
list *li;

void error_handler(const char* message) {
    if (status == ERROR) {
        perror(message);
        close(sock);
        exit(EXIT_FAILURE);
    }
}

void seek_torrent(int client_sock, char id[UUID_STR_LEN]) {
    int bytes_written;
    session *se;

    pthread_mutex_lock(&mutex);
    se = get_session_by_id(li, id);

    if (se == NULL) {
        bytes_written = send(client_sock, TORRENT_NOT_FOUND, sizeof(TORRENT_NOT_FOUND), 0);

        if (bytes_written == ERROR)
            perror("[seek_torrent] Failed to send not found message.");

    } else {
        if (se->head == NULL) {
            bytes_written = send(client_sock, TORRENT_NOT_FOUND, sizeof(TORRENT_NOT_FOUND), 0);

            if (bytes_written == ERROR)
                perror("[seek_torrent] Head null.");

            remove_session(li, id);
        } else {
            peer *next = se->head;
            char buffer[BUFFER_SIZE_MSG];
        
            strcat(buffer, se->filename);
            strcat(buffer, "\n");
            strcat(buffer, se->id);
            strcat(buffer, "\n");

            while (next != NULL)
            {
                strcat(buffer, next->address);
                strcat(buffer, "\n");
                next = next->next;
            }
             strcat(buffer, "END");
            
            bytes_written = send(client_sock, buffer, strlen(buffer), 0);

            if (bytes_written == ERROR)
                perror("[seek_torrent] Failed to send message.");
        }
    }
    pthread_mutex_unlock(&mutex);

}

void add_peer_to_session(int client_sock, char id[UUID_STR_LEN], struct sockaddr_in sock_addr) {
    int bytes_written;
    char address[INET_ADDRSTRLEN];
    session *se;

    inet_ntop(AF_INET, &(sock_addr.sin_addr), address, INET_ADDRSTRLEN); // converte para string

    pthread_mutex_lock(&mutex);
    se = get_session_by_id(li, id);

    if (se == NULL) {
        bytes_written = send(client_sock, TORRENT_NOT_FOUND, sizeof(TORRENT_NOT_FOUND), 0);

        if (bytes_written == ERROR)
            perror("[add_peer_to_session] Failed to send not found message.");

    } else {
        insert_peer(se, address);
        
        bytes_written = send(client_sock, ADD_PEER_SUCCESS, strlen(ADD_PEER_SUCCESS), 0);

        if (bytes_written == ERROR)
            perror("[add_peer_to_session] Failed to send ADD_PEER_SUCCESS message.");
    }
    pthread_mutex_unlock(&mutex);

    printf("[add_peer_to_session] PEER ADICIONADO\n");
}

void remove_peer_to_session(int client_sock, char id[UUID_STR_LEN], char peer[INET_ADDRSTRLEN], struct sockaddr_in sock_addr) {
    int bytes_written;
    char address[INET_ADDRSTRLEN];
    session *se;

    inet_ntop(AF_INET, &(sock_addr.sin_addr), address, INET_ADDRSTRLEN); // converte para string

    pthread_mutex_lock(&mutex);
    se = get_session_by_id(li, id);

    if (se == NULL) {
        bytes_written = send(client_sock, TORRENT_NOT_FOUND, sizeof(TORRENT_NOT_FOUND), 0);

        if (bytes_written == ERROR)
            perror("[remove_peer_to_session] Failed to send not found message.");

    } else {
        remove_peer(se, peer);
        
        bytes_written = send(client_sock, REMOVE_PEER_SUCCESS, strlen(REMOVE_PEER_SUCCESS), 0);

        if (bytes_written == ERROR)
            perror("[remove_peer_to_session] Failed to send REMOVE_PEER_SUCCESS message.");

        se = get_session_by_id(li, id);
        if (se->head == NULL) {
            remove_session(li, id);
        }
    }
    pthread_mutex_unlock(&mutex);

    printf("[remove_peer_to_session] PEER REMOVIDO\n");
}

void post_torrent(int client_sock, struct sockaddr_in sock_addr, char filename[260]) {
    char buffer[BUFFER_SIZE_MSG];
    char address[INET_ADDRSTRLEN];
    int bytes_written;
    inet_ntop(AF_INET, &(sock_addr.sin_addr), address, INET_ADDRSTRLEN); // converte para string
    
    pthread_mutex_lock(&mutex);
    session *se = insert_session(li, filename, address);
    pthread_mutex_unlock(&mutex);

    if (se == NULL) {
        bytes_written = send(client_sock, LIST_INSERT_FAILED, sizeof(LIST_INSERT_FAILED), 0);
        
        if (bytes_written == ERROR)
            perror("[post_torrent] Failed to send insert failed message.");


    } else {
        sprintf(buffer, "SEEK\n%s\n%s", se->id, tracker_ip);
        
        bytes_written = send(client_sock, buffer, strlen(buffer), 0);

        if (bytes_written == ERROR)
            perror("[post_torrent] Failed to send message.");
    }
}

void *handle_connections(void *arg) {
    int client_sock, data_length;
    long id = (long) arg + 1;
    char buffer[BUFFER_SIZE_MSG] = {'\0'};
    char *method, *filename, *uuid, *peer;
    struct sockaddr_in sock_addr;
    socklen_t client_address_len = sizeof(sock_addr);

    do
    {
        memset(&sock_addr, 0, sizeof(sock_addr));
        client_sock = accept(sock, (struct sockaddr*) &sock_addr, &client_address_len);
        
        if (client_sock == ERROR) {
            printf("[thread-%ld] Accept failed for %d client socket.\n", id, client_sock);
            continue;
        }

        /******************************************************/
        /* Recebe os dados da requisição e guarda no buffer.  */
        /******************************************************/

        data_length = recv(client_sock, buffer, sizeof(buffer), 0);

        if (data_length == ERROR)
        {
            printf("[thread-%ld] Receive message failed for %d", id, client_sock);
            perror(" client socket.\n");
            close(client_sock);
            continue;
        }

        if (data_length == 0)
        {
            printf("[thread-%ld] Connection closed by %d client socket.\n", id, client_sock);
            close(client_sock);
            continue;
        }

        printf("[thread-%ld] Receive for %d client socket.\n", id, client_sock);

        method = strtok(buffer, "\n");

        if (!strcmp(method, "SEEK")) {
            uuid = strtok(NULL, "\n");
            seek_torrent(client_sock, uuid);
        } else if (!strcmp(method, "POST")) {
            filename = strtok(NULL, "\n");
            post_torrent(client_sock, sock_addr, filename);
        } else if (!strcmp(method, "ADD")) {
            uuid = strtok(NULL, "\n");
            add_peer_to_session(client_sock, uuid, sock_addr);
        } else if (!strcmp(method, "REMOVE")) {
            uuid = strtok(NULL, "\n");
            peer = strtok(NULL, "\n");
            remove_peer_to_session(client_sock, uuid, peer, sock_addr);
        } else {
            printf("[thread-%ld] Invalid message from %d client socket.", id, client_sock);
            close(client_sock);
            continue;
        }

        close(client_sock);
    } while (TRUE);
    
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        perror("[main] No arguments");
        exit(EXIT_FAILURE);
    }

    tracker_ip = argv[1];
    
    struct sockaddr_in sock_addr;
    long num_threads = 0;
    pthread_t threads[MAX_NUM_THREAD];

    li = create_list();

    if (li == NULL)
    {
        perror("[main] Failed to create list.");
        exit(EXIT_FAILURE);
    }

    /******************************************/
    /* Criando o socket UDP para conexão.     */
    /******************************************/

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == ERROR) {
        perror("[main] Failed to create socket.");
        exit(EXIT_FAILURE);
    }

    printf("[main] Socket created.\n");

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

    status = bind(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr));
    error_handler("[main] Bind failed.");

    printf("[main] Successful bind.\n");

    status = listen(sock, LISTEN_BACKLOG);
    error_handler("[main] Listen Failed");

    printf("[main] Listen for connections.\n");

    do
    {
        status = pthread_create(&threads[num_threads], NULL, handle_connections, (void *) num_threads);

        if (status != SUCCESS) {
            status = ERROR;
            error_handler("[main] pthread create failed.");
        }
        
        status = pthread_detach(threads[num_threads]);
        
        if (status != SUCCESS) {
            status = ERROR;
            error_handler("[main] pthread detach failed.");
        }

    } while (++num_threads < MAX_NUM_THREAD);

    while (TRUE);
      
    return 0;
}
