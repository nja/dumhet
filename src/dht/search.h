#ifndef _search_h
#define _search_h

#include <lcthw/darray.h>
#include <dht/peers.h>
#include <dht/table.h>

typedef struct Search {
    DhtTable *table;
    Peers *peers;
} Search;

Search *Search_Create(Hash *id);
void Search_Destroy(Search *search);

int Search_CopyTable(Search *search, DhtTable *source);

#define SEARCH_RESPITE 3
#define SEARCH_MAX_PENDING 2

int Search_NodesToQuery(Search *search, DArray *nodes, time_t time);

int Search_AddPeers(Search *search, Peer *peers, int count);

#endif
