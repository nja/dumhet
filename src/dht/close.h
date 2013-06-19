#ifndef _dht_close_h
#define _dht_close_h

#include <lcthw/darray.h>
#include <dht/hash.h>
#include <dht/node.h>

/* Holds the up to BUCKET_K closet nodes to id. */
typedef struct CloseNodes
{
    Hash id;
    DArray *close_nodes;
} CloseNodes;

CloseNodes *CloseNodes_Create(Hash *id);
void CloseNodes_Destroy(CloseNodes *close);

DArray *CloseNodes_GetNodes(CloseNodes *close);

/* Adds node to the CloseNodes struct if there is still room or if node is
 * closer than any of the present entries. */
int CloseNodes_Add(CloseNodes *close, Node *node);

#endif
