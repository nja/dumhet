#include <assert.h>
#include <time.h>

#include <dht/hash.h>
#include <dht/node.h>
#include <lcthw/dbg.h>

NodeStatus Node_Status(Node *node, time_t time)
{
    assert(node != NULL && "NULL Node pointer");

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

Node *Node_Create(Hash *id)
{
    assert(id != NULL && "NULL Hash pointer");

    Node *node = calloc(1, sizeof(Node));
    check_mem(node);

    node->id = *id;

    return node;
error:
    return NULL;
}

Node *Node_Copy(Node *source)
{
    Node *copy = Node_Create(&source->id);
    check_mem(copy);

    copy->addr = source->addr;
    copy->port = source->port;

    return copy;
error:
    return NULL;
}

void Node_Destroy(Node *node)
{
    free(node);
}

int Node_DestroyOp(void *context, Node *node)
{
    (void)(context);
    free(node);

    return 0;
}

void Node_DestroyBlock(Node **nodes, size_t count)
{
    assert(((nodes == NULL && count == 0)
	    || (nodes != NULL && count > 0)) && "Bad count for block");

    unsigned int i = 0;
    for (i = 0; i < count; i++)
    {
        Node_Destroy(nodes[i]);
        nodes[i] = NULL;
    }
}

int Node_Same(Node *a, Node *b)
{
    assert(a != NULL && "NULL Node pointer");
    assert(b != NULL && "NULL Node pointer");

    return a->addr.s_addr == b->addr.s_addr
        && a->port == b->port
        && Hash_Equals(&a->id, &b->id);
}

