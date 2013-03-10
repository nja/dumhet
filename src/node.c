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

DhtNode *DhtNode_Create(DhtHash *id)
{
    assert(id != NULL && "NULL DhtHash pointer");

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

int DhtNode_Destroy(DhtNode *node)
{
    free(node);
    return 0;
}

void DhtNode_DestroyBlock(DhtNode *node, size_t count)
{
    assert(((node == NULL && count == 0)
	    || (node != NULL && count > 0)) && "Bad count for block");
    count = count;		/* Unused */
    free(node);
}
