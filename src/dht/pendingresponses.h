#ifndef _dht_pendingresponses_h
#define _dht_pendingresponses_h

#include <dht/protocol.h>
#include <dht/search.h>
#include <lcthw/hashmap.h>

typedef struct HashmapPendingResponses {
    GetPendingResponse_fp getPendingResponse;
    AddPendingResponse_fp addPendingResponse;
    Hashmap *hashmap;
} HashmapPendingResponses;

HashmapPendingResponses *HashmapPendingResponses_Create();
void HashmapPendingResponses_Destroy(HashmapPendingResponses *pending);

int HashmapPendingResponses_Add(void *responses, PendingResponse entry);
PendingResponse HashmapPendingResponses_Remove(void *responses, char *tid, int *rc);

int PendingResponse_Compare(tid_t *a, tid_t *b);
uint32_t PendingResponse_Hash(tid_t *tid);

#endif
