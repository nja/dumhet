#include <assert.h>
#include <time.h>

#include <dht/bucket.h>
#include <dht/node.h>
#include <lcthw/dbg.h>

Bucket *Bucket_Create()
{
    Bucket *bucket = calloc(1, sizeof(Bucket));
    check_mem(bucket);

    bucket->change_time = time(NULL);

    return bucket;
error:
    return NULL;
}

void Bucket_Destroy(Bucket *bucket)
{
    free(bucket);
}

int Bucket_ContainsNode(Bucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    DhtNode **bucket_node = bucket->nodes;

    while (bucket_node < bucket->nodes + BUCKET_K)
    {
        if (*bucket_node != NULL &&
            DhtHash_Equals(&node->id, &(*bucket_node)->id))
        {
            return 1;
        }

        bucket_node++;
    }

    return 0;
}

/* Returns the replaced node, or NULL when no bad was found */
DhtNode *Bucket_ReplaceBad(Bucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    time_t now = time(NULL);

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        if (bucket->nodes[i] == NULL)
            continue;

	if (DhtNode_Status(bucket->nodes[i], now) == Bad)
	{
	    DhtNode *replaced = bucket->nodes[i];
	    bucket->nodes[i] = node;
	    bucket->change_time = now;

	    return replaced;
	}
    }

    return NULL;
}

DhtNode *Bucket_ReplaceQuestionable(Bucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    time_t now = time(NULL), oldest_time = now;
    int oldest_i = -1, i = 0;

    for (i = 0; i < BUCKET_K; i++)
    {
        if (bucket->nodes[i] == NULL)
            continue;

	if (DhtNode_Status(bucket->nodes[i], now) == Questionable)
	{
	    if (bucket->nodes[i]->reply_time < oldest_time) {
		oldest_time = bucket->nodes[i]->reply_time;
		oldest_i = i;
	    }

	    if (bucket->nodes[i]->query_time < oldest_time) {
		oldest_time = bucket->nodes[i]->query_time;
		oldest_i = i;
	    }
	}
    }

    if (oldest_i > -1)
    {
	// TODO: ping before replacing
	DhtNode *replaced = bucket->nodes[oldest_i];
	bucket->nodes[oldest_i] = node;
	bucket->change_time = now;

	return replaced;
    }

    return NULL;
}

int Bucket_IsFull(Bucket *bucket)
{
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(bucket->count >= 0 && "Negative Bucket count");
    assert(bucket->count <= BUCKET_K && "Too large Bucket count");

    return BUCKET_K == bucket->count;
}

int Bucket_AddNode(Bucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");
    assert(bucket->count >= 0 && "Negative Bucket count");
    assert(bucket->count <= BUCKET_K && "Too large Bucket count");

    check(!Bucket_IsFull(bucket), "Bucket full");

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        if (bucket->nodes[i] == NULL)
        {
            bucket->nodes[i] = node;
            bucket->count++;
            bucket->change_time = time(NULL);

            assert(bucket->count <= BUCKET_K && "Too large Bucket count");

            return 0;
        }
    }

    log_err("Bucket with count %d had no empty slots", bucket->count);
error:
    return -1;
}

int Bucket_GatherGoodNodes(Bucket *bucket, DArray *found)
{
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(found != NULL && "NULL DArray pointer");

    DhtNode **node = bucket->nodes;

    while (DArray_count(found) < BUCKET_K && node < bucket->nodes + BUCKET_K)
    {
        if (*node != NULL && DhtNode_Status(*node, time(NULL)) == Good)
        {
            int rc = DArray_push(found, *node);
            check(rc == 0, "DArray_push failed");
        }

        node++;
    }

    return 0;
error:
    return -1;
}
