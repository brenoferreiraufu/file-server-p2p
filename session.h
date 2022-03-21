#include <arpa/inet.h>
#include <uuid/uuid.h>
#define LENGTH 260

typedef struct peer
{
    char address[INET_ADDRSTRLEN];
    struct peer *next;
} peer;

typedef struct swarm
{
    peer *head;
} swarm;

typedef struct session
{
    char filename[LENGTH];
    char id[UUID_STR_LEN];
    swarm *peers;
} session;

typedef struct list
{
    int size;
    session sessions[LENGTH];
} list;

swarm *create_swarm();
list *create_list();
void free_swarm(swarm *li);
void free_list(list *li);
int insert_peer(swarm *li, char address[INET_ADDRSTRLEN]);
session *insert_session(list *li, char filename[LENGTH], char address[INET_ADDRSTRLEN]);
session *get_session_by_id(list *li, int id);
int remove_peer(swarm *li, char address[INET_ADDRSTRLEN]);
int remove_session(list *li, int id);
