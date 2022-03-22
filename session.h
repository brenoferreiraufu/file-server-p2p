#include <arpa/inet.h>
#include <uuid/uuid.h>
#define LENGTH 260

typedef struct peer
{
    char address[INET_ADDRSTRLEN];
    struct peer *next;
} peer;

typedef struct session
{
    char filename[LENGTH];
    char id[UUID_STR_LEN];
    peer *head;
} session;

typedef struct list
{
    int size;
    session sessions[LENGTH];
} list;

list *create_list();
void free_list(list *li);
int insert_peer(session *se, char address[INET_ADDRSTRLEN]);
session *insert_session(list *li, char filename[LENGTH], char address[INET_ADDRSTRLEN]);
session *get_session_by_id(list *li, char uuid[UUID_STR_LEN]);
int remove_peer(session *se, char address[INET_ADDRSTRLEN]);
int remove_session(list *li, char uuid[UUID_STR_LEN]);
void free_swarm(session *se);
