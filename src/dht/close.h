#ifndef _dht_close_h
#define _dht_close_h

#include <lcthw/darray.h>
#include <dht/hash.h>
#include <dht/node.h>

typedef struct CloseNodes
{
    Hash id;
    DArray *close_nodes;
} CloseNodes;

CloseNodes *CloseNodes_Create(Hash *id);
void CloseNodes_Destroy(CloseNodes *close);

DArray *CloseNodes_GetNodes(CloseNodes *close);

int CloseNodes_Add(CloseNodes *close, Node *node);

#endif
