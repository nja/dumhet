#include <assert.h>
#include <time.h>

#include <dht/hash.h>
#include <dht/node.h>
#include <lcthw/dbg.h>

DhtNodeStatus DhtNode_Status(DhtNode *node, time_t time)
{
    assert(node != NULL && "NULL DhtNode pointer");

    if (node->reply_time != 0)
    {
	if (difftime(time, node->reply_time) < NODE_RESPITE)
	    return Good;

	if (difftime(time, node->query_time) < NODE_RESPITE)
	    return Good;

	if (node->pending_queries < NODE_MAX_PENDING)
	    return Questionable;

	return Bad;
    }

    if (node->pending_queries < NODE_MAX_PENDING)
	return Unknown;

    return Bad;
}

DhtNode *DhtNode_Create(Hash *id)
{
    assert(id != NULL && "NULL Hash pointer");

    DhtNode *node = calloc(1, sizeof(DhtNode));
    check_mem(node);

    node->id = *id;

    return node;
error:
    return NULL;
}

DhtNode *DhtNode_Copy(DhtNode *source)
{
    DhtNode *copy = DhtNode_Create(&source->id);
    check_mem(copy);

    copy->addr = source->addr;
    copy->port = source->port;

    return copy;
error:
    return NULL;
}

void DhtNode_Destroy(DhtNode *node)
{
    free(node);
}

int DhtNode_DestroyOp(void *context, DhtNode *node)
{
    (void)(context);
    free(node);

    return 0;
}

void DhtNode_DestroyBlock(DhtNode **nodes, size_t count)
{
    assert(((nodes == NULL && count == 0)
	    || (nodes != NULL && count > 0)) && "Bad count for block");

    unsigned int i = 0;
    for (i = 0; i < count; i++)
    {
        DhtNode_Destroy(nodes[i]);
        nodes[i] = NULL;
    }
}

int DhtNode_Same(DhtNode *a, DhtNode *b)
{
    assert(a != NULL && "NULL DhtNode pointer");
    assert(b != NULL && "NULL DhtNode pointer");

    return a->addr.s_addr == b->addr.s_addr
        && a->port == b->port
        && Hash_Equals(&a->id, &b->id);
}

