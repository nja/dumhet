#include <stdlib.h>

#include <dht/pendingresponses.h>
#include <lcthw/hashmap.h>

HashmapPendingResponses *HashmapPendingResponses_Create()
{
    HashmapPendingResponses *pending = malloc(sizeof(HashmapPendingResponses));
    check_mem(pending);

    pending->getPendingResponse = HashmapPendingResponses_Remove;

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

int HashmapPendingResponses_Add(HashmapPendingResponses *responses, PendingResponseEntry entry)
{
    assert(responses != NULL && "NULL HashmapPendingResponses pointer");

    PendingResponseEntry *mentry = malloc(sizeof(PendingResponseEntry));
    check_mem(mentry);

    *mentry = entry;

    int rc = Hashmap_set(responses->hashmap, &mentry->tid, mentry);
    check(rc == 0, "Hashmap_set failed");

    return 0;
error:
    free(mentry);
    return -1;
}
    
PendingResponseEntry HashmapPendingResponses_Remove(void *responses, char *tid, int *rc)
{
    assert(responses != NULL && "NULL HashmapPendingResponses pointer");
    assert(tid != NULL && "NULL tid pointer");

    PendingResponseEntry *entry = Hashmap_delete(((HashmapPendingResponses *)responses)->hashmap, tid);
    check(entry != NULL, "Hashmap_delete failed");

    PendingResponseEntry rentry = *entry;
    *rc = 0;
    free(entry);

    return rentry;
error:
    *rc = -1;
    return (PendingResponseEntry) { 0 };
}
