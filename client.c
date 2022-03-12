#include <sys/socket.h> // Funções relacionadas a socket
#include <stdlib.h> 
#include <stdio.h>
#include <arpa/inet.h> // Estrutura sockaddr_in 
#include <string.h> 
#include <pthread.h> // Funções de thread
#include <unistd.h> // close()
#include "env.h"

#define PORT 49153
#define LISTEN_BACKLOG 11
#define MAX_NUM_THREADS 8

int status, sock; // File descriptor do socket.

void error_handler(const char* fail, const char* success) {
    if (status == ERROR) {
        perror(fail);
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", success);
}

int main(int argc, char const *argv[])
{    
    struct sockaddr_in sock_addr; // Estrutura de dados que armazena o endereço.
    pthread_t ts_id, td_id;

    /******************************************/
    /* Criando o socket TCP para conexão.     */
    /******************************************/

    sock = socket(AF_INET, SOCK_STREAM, 0);

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
    error_handler("Bind failed.", "Successful bind.");

    /******************************************/
    /* Escuta requisições de conexão.         */
    /******************************************/
    
    status = listen(sock, LISTEN_BACKLOG);
    error_handler("Listen failed.", "Listen for connections...");

    do
    {
    } while (TRUE);
    
    return 0;
}
