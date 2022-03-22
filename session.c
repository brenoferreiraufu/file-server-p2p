#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

list *create_list()
{
    list *li;
    li = malloc(sizeof(list));
    li->size = 0;
    return li;
}

void free_list(list *li)
{
    if (li != NULL)
    {
        for (int i = 0; i < li->size; i++)
        {
            free_swarm(&li->sessions[i]);
            
        }

        free(li);
    }
}

int insert_peer(session *se, char address[INET_ADDRSTRLEN])
{
    if (se == NULL)
        return -1;
    peer *element = malloc(sizeof(peer));
    if (element == NULL)
        return -2;

    strcpy(element->address, address);

    if (se->head == NULL)
    {
        se->head = element;
        element->next = NULL;
        return 0;
    }
    else
    {
        element->next = se->head;
        se->head = element;
        return 0;
    }
}

session *insert_session(list *li, char filename[LENGTH], char address[INET_ADDRSTRLEN])
{
    if (li == NULL)
        return NULL;

    if (li->size < LENGTH)
    {
        uuid_t binuuid;
        uuid_generate_random(binuuid);
        uuid_unparse(binuuid, li->sessions[li->size].id);

        strcpy(li->sessions[li->size].filename, filename);
        li->sessions[li->size].head = NULL;

        insert_peer(&li->sessions[li->size], address);

        li->size++;
        return &li->sessions[li->size-1];
    }

    return NULL;
}

session *get_session_by_id(list *li, char uuid[UUID_STR_LEN])
{
    if (li == NULL)
        return NULL;
    
    for (int i = 0; i < li->size; i++) {
        if (!strcmp(li->sessions[i].id, uuid))
            return &li->sessions[i];
    }
    
    return NULL;
}

int remove_peer(session *se, char address[INET_ADDRSTRLEN])
{
    if (se == NULL)
        return -1;
    peer *p = NULL;
    p = se->head;
    if (p->next == NULL && !strcmp(p->address, address))
    {
        free(p);
        se->head = NULL;
        return 0;
    }
    peer *prev = p;
    while (!strcmp(p->address, address) && p->next != NULL)
    {
        prev = p;
        p = p->next;
    }
    if (p == NULL)
    {
        return -1;
    }
    else
    {
        prev->next = p->next;
        free(p);
        return 0;
    }
}

void free_swarm(session *se) {
    if (se == NULL)
        return;

    peer *next = se->head;
    for (peer *pe = next; next != NULL; pe = next)
    {
        next = pe->next;
        free(pe);
    }
}

int remove_session(list *li, char uuid[UUID_STR_LEN])
{
    if (li == NULL)
        return -1;

    int i;
    for (i = 0; i < li->size; i++) {
        if (!strcmp(li->sessions[i].id, uuid))
            break;
    }

    if (i >= li->size) return -1;

    free_swarm(&li->sessions[i]);

    for (int k = i; k < li->size - 1; k++) {
        li->sessions[k] = li->sessions[k + 1];
    }

    li->size--;

    return 0;
}
