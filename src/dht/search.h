#ifndef _dht_search_h
#define _dht_search_h

#include <lcthw/darray.h>
#include <dht/client.h>
#include <dht/peers.h>
#include <dht/table.h>

/* A search uses a Table and a Peers collection. It begins by adding
 * to the search table copies of the nodes from a client's routing
 * table. It then queries the search table nodes, adding any nodes and
 * peers from the replies to the table and peers collection. New
 * queries can be sent as long as there are nodes in the table that
 * has yet to reply.  Both the table and the peers holds the target
 * hash. */
typedef struct Search {
    Table *table;
    Peers *peers;
    int find_node_sent;
    Hashmap *tokens;
} Search;

Search *Search_Create(Hash *id);
void Search_Destroy(Search *search);

/* Copy the nodes from source and insert them to the search table. */
int Search_CopyTable(Search *search, Table *source);

#define SEARCH_RESPITE 3
#define SEARCH_MAX_PENDING 2

/* Finds nodes in the search table that should be queried. */
int Search_NodesToQuery(Search *search, DArray *nodes, time_t time);

/* Adds to the collection of found peers by the search. */
int Search_AddPeers(Search *search, Peer *peers, int count);

/* Gets the token, if any, for the node id. */
struct FToken *Search_GetToken(Search *search, Hash *id);

/* Sets the token for the node id. */
int Search_SetToken(Search *search, Hash *id, struct FToken token);

#endif
