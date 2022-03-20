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

int sock, status;

void error_handler(const char* message) {
    if (status == ERROR) {
        perror(message);
        close(sock);
        exit(EXIT_FAILURE);
    }
}

void seek_torrent() {
}

void post_torrent() {
}

void *handle_connections(void *arg) {
    int client_sock, data_length;
    long id = (long) arg + 1;
    char buffer[BUFFER_SIZE_MSG];
    char method[10];

    do
    {
        client_sock = accept(sock, NULL, NULL);
        
        if (client_sock == ERROR) {
            printf("[thread-%d] Accept failed for %d client socket.\n", id, client_sock);
            continue;
        }

        /******************************************************/
        /* Recebe os dados da requisição e guarda no buffer.  */
        /******************************************************/

        data_length = recv(client_sock, buffer, sizeof(buffer), 0);

        if (data_length == ERROR)
        {
            printf("[thread-%d] Receive message failed for %d", id, client_sock);
            perror(" client socket.\n");
            close(client_sock);
            continue;
        }

        if (data_length == 0)
        {
            printf("[thread-%d] Connection closed by %d client socket.", id, client_sock);
            close(client_sock);
            continue;
        }

        sscanf(buffer, "%s\n", method);

        if (!strcmp(method, "SEEK")) {
            seek_torrent();
        } else if (!strcmp(method, "POST")) {
            post_torrent();
        } else {
            printf("[thread-%d] Invalid message from %d client socket.", id, client_sock);
            close(client_sock);
            continue;
        }

    } while (TRUE);
    
    
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    struct sockaddr_in sock_addr;
    int num_threads = 0;
    pthread_t threads[MAX_NUM_THREAD];

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

        printf("[main] %d thread created.\n", num_threads + 1);

        status = pthread_detach(threads[num_threads]);
        
        if (status != SUCCESS) {
            status = ERROR;
            error_handler("[main] pthread detach failed.");
        }

        printf("[main] %d thread detached.\n", num_threads + 1);

    } while (++num_threads < MAX_NUM_THREAD);

    while (TRUE);
      
    return 0;
}
