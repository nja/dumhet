#ifndef _dht_pendingresponses_h
#define _dht_pendingresponses_h

#include <dht/protocol.h>
#include <dht/search.h>
#include <lcthw/hashmap.h>

/* A PendingResponses collection using a hashmap. */
typedef struct HashmapPendingResponses {
    GetPendingResponse_fp getPendingResponse;
    AddPendingResponse_fp addPendingResponse;
    Hashmap *hashmap;
} HashmapPendingResponses;

HashmapPendingResponses *HashmapPendingResponses_Create();
void HashmapPendingResponses_Destroy(HashmapPendingResponses *pending);

#endif
