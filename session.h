typedef struct peer {
    char address[260];
    peer *next;
} peer;

typedef struct swarm {
    int size;
    peer *head;
} swarm;

typedef struct session {
    int id;
    char filename[260];
    char sha[260];
    swarm peers;
} session;
