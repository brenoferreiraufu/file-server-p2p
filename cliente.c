#include <sys/socket.h> // Funções relacionadas a socket
#include <stdlib.h> 
#include <stdio.h>
#include <arpa/inet.h> // Estrutura sockaddr_in 
#include <string.h> 
#include <pthread.h> // Funções de thread
#include <unistd.h> // close()

#define ERROR -1
#define TRUE 1

#define PORT 49153
#define LISTEN_BACKLOG 11

int status = 0;
int sock; // File descriptor do socket.
struct sockaddr_in sock_addr; // Estrutura de dados que armazena o endereço.

void error_handler(const char* message) {
    if (status == ERROR) {
        perror(message);
        close(sock);
        exit(EXIT_FAILURE);
    }
}

void *enviar(void *arg) {
    int client_sock;    
    
    client_sock = accept(sock, (struct sockaddr *) &sock_addr, NULL);
    error_handler("Falha ao aceitar conexão!");
}

void *baixar(void *arg) {
    int client_sock;

    client_sock = accept(sock, (struct sockaddr *) &sock_addr, NULL);
    error_handler("Falha ao aceitar conexão!");
}



int main(int argc, char const *argv[])
{
    int size_addr = sizeof(sock_addr); // Tamanho da estrutura de endereço.
    
    pthread_t ts_id, td_id;

    /******************************************/
    /* Criando o socket TCP para conexão.     */
    /******************************************/

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == ERROR) {
        perror("Falhao ao criar o socket.");
        exit(EXIT_FAILURE);
    }

    /******************************************/
    /* Inicializa a estrutura de endereço.    */
    /******************************************/

    memset(&sock_addr, 0,size_addr);
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons(PORT);
    sock_addr.sin_family = AF_INET;
    
    /******************************************/
    /* Anexa o endereço local a um socket.    */
    /******************************************/

    status = bind(sock, (struct sockaddr *) &sock_addr, size_addr);
    error_handler("Falha ao anexar endereço ao socket!");

    /******************************************/
    /* Escuta requisições de conexão.         */
    /******************************************/
    
    status = listen(sock, LISTEN_BACKLOG);
    error_handler("Falha ao escutar conexões!");

    pthread_create(&ts_id, NULL, &enviar, NULL);
    pthread_create(&td_id, NULL, &baixar, NULL);

    while (TRUE);
    
    return 0;
}
