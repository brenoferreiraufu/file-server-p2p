#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "env.h"

#define PORT 49154
#define MAX_NUM_THREAD 8

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

void insert_torrent() {
}

void *handle_connections(void *arg) {
    char buffer[BUFFER_SIZE_MSG];
    int bytes_read;
    struct sockaddr_in client_addr;
    
    memset(&client_addr, 0, sizeof(client_addr));

    while(TRUE) {
        bytes_read = recvfrom(sock, buffer, BUFFER_SIZE_MSG, MSG_WAITALL, 
            (struct sockaddr *) &client_addr, sizeof(client_addr));
        
    }

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

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == ERROR) {
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

    status = bind(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr));
    error_handler("Bind failed.");

    printf("Successful bind.\n");

    do
    {
        status = pthread_create(&threads[num_threads], NULL, handle_connections, NULL);

        if (status != SUCCESS) {
            status = ERROR;
            error_handler("pthread create failed.");
        }

        printf("%d thread created.\n", num_threads + 1);

        status = pthread_detach(threads[num_threads]);
        
        if (status != SUCCESS) {
            status = ERROR;
            error_handler("pthread detach failed.");
        }

        printf("%d thread detached.\n", num_threads + 1);

    } while (++num_threads < MAX_NUM_THREAD);

    while (TRUE);
      
    return 0;
}
