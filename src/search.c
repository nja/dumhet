#include <assert.h>

#include <dht/search.h>
#include <dht/table.h>
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
            if (*node != NULL)
            {
                copy = DhtNode_Copy(*node);
                check_mem(copy);

                result = DhtTable_InsertNode(dest, copy);
                check(result.rc == OKAdded, "DhtTable_InsertNode failed");
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

int ShouldQuery(DhtNode *node, time_t time);

struct QueryNodesContext
{
    time_t time;
    DArray *nodes;
};

int AddIfQueryable(void *vcontext, DhtNode *node)
{
    struct QueryNodesContext *context = (struct QueryNodesContext *)vcontext;
    assert(context != NULL && "NULL QueryNodesContext pointer");
    assert(node != NULL && "NULL DhtNode pointer");
    assert(context->nodes != NULL && "NULL DArray pointer in QueryNodesContext");

    if (ShouldQuery(node, context->time))
    {
        int rc = DArray_push(context->nodes, node);
        check(rc == 0, "DArray_push failed");
    }

    return 0;
error:
    return -1;
}

int Search_NodesToQuery(Search *search, DArray *nodes, time_t time)
{
    assert(search != NULL && "NULL Search pointer");
    assert(nodes != NULL && "NULL DArray pointer");

    struct QueryNodesContext context;
    context.time = time;
    context.nodes = nodes;

    int rc = DhtTable_ForEachNode(search->table, &context, AddIfQueryable);
    check(rc == 0, "DhtTable_ForEachNode failed");

    return 0;
error:
    return -1;
}

int ShouldQuery(DhtNode *node, time_t time)
{
    assert(node != NULL && "NULL DhtNode pointer");

    if (node->reply_time != 0)
        return 0;

    if (node->pending_queries >= SEARCH_MAX_PENDING)
        return 0;

    if (difftime(time, node->query_time) < SEARCH_RESPITE)
        return 0;

    return 1;
}

int Search_AddPeers(Search *search, Peer *peers, int count)
{
    assert(search != NULL && "NULL Search pointer");
    assert(peers != NULL && "NULL Peer pointer");

    Peer *end = peers + count;

    while (peers < end)
    {
        int rc = Peers_AddPeer(search->peers, peers);
        check(rc == 0, "Peers_AddPeer failed");
        peers++;
    }

    return 0;
error:
    return -1;
}
