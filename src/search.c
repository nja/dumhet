#include <assert.h>

#include <dht/search.h>
#include <lcthw/dbg.h>

Search *Search_Create(DhtHash *id)
{
    assert(id != NULL && "NULL DhtHash pointer");

    Search *search = malloc(sizeof(Search));
    check_mem(search);

    search->table = DhtTable_Create(id);
    check_mem(search->table);

    return search;
error:
    free(search);
    return NULL;
}

void Search_Destroy(Search *search)
{
    DhtTable_Destroy(search->table);
    free(search);
}

int Search_CopyTable(Search *search, DhtTable *source)
{
    assert(search != NULL && "NULL Search pointer");
    assert(source != NULL && "NULL DhtTable pointer");

    DhtTable *dest = search->table;

    assert(dest != NULL && "NULL DhtTable pointer");
    assert(dest->end == 1 && "Expected a single bucket");
    assert(dest->buckets[0]->count == 0 && "Expected an empty bucket");

    DhtBucket *last = DhtTable_FindBucket(source, &dest->id);
    DhtBucket **bucket = source->buckets;
    DhtNode *copy = NULL;
    DhtTable_InsertNodeResult result = { 0 };
    do
    {
        DhtNode **node = (*bucket)->nodes;

        while (node < &(*bucket)->nodes[BUCKET_K])
        {
            debug("Source bucket %d", (*bucket)->index);
            if (*node != NULL)
            {
                copy = DhtNode_Copy(*node);
                check_mem(copy);

                result = DhtTable_InsertNode(dest, copy);
                check(result.rc == OKAdded, "DhtTable_InsertNode failed");
                debug("Copied to dest bucket %d", result.bucket->index);
            }

            node++;
        }
    } while (*bucket++ != last);

    return 0;
error:
    DhtNode_Destroy(copy);
    DhtNode_Destroy(result.replaced);
    return -1;
}
