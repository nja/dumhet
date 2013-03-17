#ifndef _close_h
#define _close_h

#include <lcthw/darray.h>
#include <dht/hash.h>
#include <dht/node.h>

typedef struct CloseNodes
{
    DhtHash id;
    DArray *close_nodes;
} CloseNodes;

CloseNodes *CloseNodes_Create(DhtHash *id);
void CloseNodes_Destroy(CloseNodes *close);

DArray *CloseNodes_GetNodes(CloseNodes *close);

int CloseNodes_Add(CloseNodes *close, DhtNode *node);

#endif
