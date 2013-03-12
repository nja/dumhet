#ifndef _pendingresponses_h
#define _pendingresponses_h

#include <dht/protocol.h>
#include <dht/search.h>
#include <lcthw/hashmap.h>

typedef struct HashmapPendingResponses {
    GetPendingResponse_fp getPendingResponse;
    Hashmap *hashmap;
} HashmapPendingResponses;

HashmapPendingResponses *HashmapPendingResponses_Create();
void HashmapPendingResponses_Destroy(HashmapPendingResponses *pending);

int HashmapPendingResponses_Add(HashmapPendingResponses *responses, PendingResponseEntry entry);
PendingResponseEntry HashmapPendingResponses_Remove(void *responses, char *tid, int *rc);

int PendingResponse_Compare(void *a, void *b);
uint32_t PendingResponse_Hash(void *tid);

#endif
