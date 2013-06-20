#include <assert.h>

#include <dht/search.h>
#include <dht/table.h>
#include <lcthw/dbg.h>

Search *Search_Create(Hash *id)
{
    assert(id != NULL && "NULL Hash pointer");

    Search *search = calloc(1, sizeof(Search));
    check_mem(search);

    search->table = Table_Create(id);
    check(search->table != NULL, "Table_Create failed");

    search->peers = Peers_Create(id);
    check(search->peers != NULL, "Peers_Create failed");

    return search;
error:
    Search_Destroy(search);
    return NULL;
}

void Search_Destroy(Search *search)
{
    if (search == NULL)
    {
        return;
    }

    Table_DestroyNodes(search->table);
    Table_Destroy(search->table);
    Peers_Destroy(search->peers);
    free(search);
}

int CopyAndAdd(Table *dest, Node *node)
{
    if (Node_Status(node, time(NULL)) != Good)
    {
        return 0;
    }

    Node *copy = Node_Copy(node);
    check_mem(copy);

    Table_InsertNodeResult result = Table_InsertNode(dest, copy);

    if (result.rc == ERROR
        || result.rc == OKFull
        || result.rc == OKAlreadyAdded)
    {
        Node_Destroy(copy);
    }

    if (result.rc == OKReplaced)
    {
        assert(result.replaced != NULL && "OKReplaced with NULL .replaced");
        Node_Destroy(result.replaced);
    }

    return 0;
error:
    return -1;
}

int Search_CopyTable(Search *search, Table *source)
{
    assert(search != NULL && "NULL Search pointer");
    assert(source != NULL && "NULL Table pointer");
    assert(search->table != NULL && "NULL Table pointer");

    return Table_ForEachNode(source, search->table, (NodeOp)CopyAndAdd);
}

int ShouldQuery(Node *node, time_t time);

struct QueryNodesContext
{
    time_t time;
    DArray *nodes;
};

int AddIfQueryable(void *vcontext, Node *node)
{
    struct QueryNodesContext *context = (struct QueryNodesContext *)vcontext;
    assert(context != NULL && "NULL QueryNodesContext pointer");
    assert(node != NULL && "NULL Node pointer");
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

    int rc = Table_ForEachNode(search->table, &context, AddIfQueryable);
    check(rc == 0, "Table_ForEachNode failed");

    return 0;
error:
    return -1;
}

int ShouldQuery(Node *node, time_t time)
{
    assert(node != NULL && "NULL Node pointer");

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
