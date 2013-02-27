#include <stdlib.h>

#include <dht/pendingresponses.h>
#include <lcthw/hashmap.h>

HashmapPendingResponses *HashmapPendingResponses_Create()
{
    HashmapPendingResponses *pending = malloc(sizeof(HashmapPendingResponses));
    check_mem(pending);

    pending->getResponseType = HashmapPendingResponses_Remove;

    pending->hashmap = Hashmap_create(PendingResponse_Compare, PendingResponse_Hash);
    check_mem(pending->hashmap);

    return pending;
error:
    return NULL;
}

int FreeEntry(HashmapNode *node)
{
    free(node->data);
    return 0;
}

void HashmapPendingResponses_Destroy(HashmapPendingResponses *pending)
{
    assert(pending != NULL && "NULL HashmapPendingResponses pointer");
    
    Hashmap_traverse(pending->hashmap, FreeEntry);
    Hashmap_destroy(pending->hashmap);
    free(pending);
}

uint32_t PendingResponse_Hash(void *key)
{
    assert(key != NULL && "NULL tid pointer");

    tid_t tid = *(tid_t *)key;

    uint32_t hash = 0, i = 0;

    for (i = 0; i < sizeof(uint32_t); i += sizeof(tid_t))
    {
	hash += tid;
	hash += (hash << 10);
	hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

int PendingResponse_Compare(void *a, void *b)
{
    assert(a != NULL && "NULL PendingResponseEntry pointer");
    assert(b != NULL && "NULL PendingResponseEntry pointer");

    tid_t atid = *(tid_t *)a,
	btid = *(tid_t *)b;

    if (atid < btid)
	return -1;
    else if (btid < atid)
	return 1;
    else
	return 0;
}

int HashmapPendingResponses_Add(HashmapPendingResponses *responses, MessageType type, tid_t tid)
{
    assert(responses != NULL && "NULL HashmapPendingResponses pointer");

    PendingResponseEntry *entry = malloc(sizeof(PendingResponseEntry));
    check_mem(entry);

    entry->type = type;
    entry->tid = tid;

    int rc = Hashmap_set(responses->hashmap, &entry->tid, entry);
    check(rc == 0, "Hashmap_set failed");

    return 0;
error:
    free(entry);
    return -1;
}
    
int HashmapPendingResponses_Remove(void *responses, char *tid, MessageType *type)
{
    assert(responses != NULL && "NULL HashmapPendingResponses pointer");
    assert(tid != NULL && "NULL tid pointer");
    assert(type != NULL && "NULL MessageType pointer");

    PendingResponseEntry *entry = Hashmap_delete(((HashmapPendingResponses *)responses)->hashmap, tid);
    check(entry != NULL, "Hashmap_delete failed");

    *type = entry->type;

    free(entry);

    return 0;
error:
    return -1;
}
