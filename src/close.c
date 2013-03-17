#include <lcthw/darray.h>
#include <dht/bucket.h>
#include <dht/close.h>
#include <dht/hash.h>
#include <dht/node.h>

struct CloseNode
{
    DhtNode *node;
    DhtDistance distance;
};

CloseNodes *CloseNodes_Create(DhtHash *id)
{
    CloseNodes *nodes = malloc(sizeof(CloseNodes));
    check_mem(nodes);

    nodes->id = *id;
    nodes->close_nodes = DArray_create(sizeof(struct CloseNode *), BUCKET_K + 1);
    check_mem(nodes->close_nodes);

    return nodes;
error:
    free(nodes);
    return NULL;
}

void CloseNodes_Destroy(CloseNodes *close)
{
    if (close == NULL)
        return;

    while (DArray_count(close->close_nodes) > 0)
    {
        free(DArray_pop(close->close_nodes));
    }

    DArray_destroy(close->close_nodes);
    free(close);
}

DArray *CloseNodes_GetNodes(CloseNodes *close)
{
    assert(close != NULL && "NULL CloseNodes pointer");

    DArray *nodes = DArray_create(sizeof(DhtNode *),
                                  DArray_max(close->close_nodes));
    check(nodes != NULL, "DArray_create failed");

    int i = 0;
    for (i = 0; i < DArray_count(close->close_nodes); i++)
    {
        struct CloseNode *close_node = DArray_get(close->close_nodes, i);
        check(close_node != NULL, "NULL CloseNodes entry");
        check(close_node->node != NULL, "NULL DhtNode pointer in CloseNodes entry");

        int rc = DArray_push(nodes, close_node->node);
        check(rc == 0, "DArray_push failed");
    }

    return nodes;
error:
    DArray_destroy(nodes);
    return NULL;
}

int FindIndex(DArray *close, DhtDistance *distance);
void ShiftFrom(DArray *close, int i);

int CloseNodes_Add(CloseNodes *close, DhtNode *node)
{
    assert(close != NULL && "NULL CloseNodes pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    DhtDistance distance = DhtHash_Distance(&close->id, &node->id);
    int i = FindIndex(close->close_nodes, &distance);

    if (i < 0)
        return 0;

    struct CloseNode *close_node = malloc(sizeof(struct CloseNode));
    check_mem(close_node);

    close_node->node = node;
    close_node->distance = distance;

    if (DArray_count(close->close_nodes) < BUCKET_K)
        DArray_push(close->close_nodes, NULL);

    ShiftFrom(close->close_nodes, i);
    DArray_set(close->close_nodes, i, close_node);

    return 0;
error:
    return -1;
}

int FindIndex(DArray *close, DhtDistance *distance)
{
    assert(close != NULL && "NULL DArray pointer");
    assert(distance != NULL && "NULL DhtDistance pointer");

    int i = 0;
    while (i < BUCKET_K)
    {
        if (i == DArray_count(close))
            return i;

        struct CloseNode *other = DArray_get(close, i);

        if (DhtDistance_Compare(distance, &other->distance) <= 0)
            return i;

        i++;
    }

    return -1;
}

void ShiftFrom(DArray *close, int from)
{
    assert(close != NULL && "NULL DArray pointer");
    assert(BUCKET_K - 1 < DArray_max(close) && "Too small DArray");
    assert(0 <= from && from <= DArray_end(close) && "Bad shift index");

    if (DArray_end(close) == BUCKET_K)
        free(DArray_last(close));

    int i;
    for (i = DArray_end(close) - 1; i > from; i--)
    {
        struct CloseNode *tmp = DArray_get(close, i - 1);
        DArray_set(close, i, tmp);
    }
}
