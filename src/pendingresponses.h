#ifndef _pendingresponses_h
#define _pendingresponses_h

#include <protocol.h>
#include <lcthw/hashmap.h>

typedef struct HashmapPendingResponses {
    GetResponseType_fp getResponseType;
    Hashmap *hashmap;
} HashmapPendingResponses;

typedef struct PendingResponseEntry {
    MessageType type;
    tid_t tid;
} PendingResponseEntry;

HashmapPendingResponses *HashmapPendingResponses_Create();
void HashmapPendingResponses_Destroy(HashmapPendingResponses *pending);

int HashmapPendingResponses_Add(HashmapPendingResponses *responses, MessageType type, tid_t tid);
int HashmapPendingResponses_Remove(void *pending, char *tid, MessageType *type);

int PendingResponse_Compare(void *a, void *b);
uint32_t PendingResponse_Hash(void *tid);

#endif
