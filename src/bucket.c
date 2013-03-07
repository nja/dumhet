#include <assert.h>

#include <dht/bucket.h>
#include <dht/node.h>
#include <lcthw/dbg.h>

DhtBucket *DhtBucket_Create()
{
    DhtBucket *bucket = calloc(1, sizeof(DhtBucket));
    check_mem(bucket);

    bucket->change_time = time(NULL);

    return bucket;
error:
    return NULL;
}

void DhtBucket_Destroy(DhtBucket *bucket)
{
    free(bucket);
}

int DhtBucket_ContainsNode(DhtBucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    int i = 0;

    for (i = 0; i < BUCKET_K; i++)
    {
	if (bucket->nodes[i] == node)
	    return 1;
    }

    return 0;
}

/* Returns the replaced node, or NULL when no bad was found */
DhtNode *DhtBucket_ReplaceBad(DhtBucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    time_t now = time(NULL);

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	assert(bucket->nodes[i] != NULL && "Empty slot in full bucket");

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

DhtNode *DhtBucket_ReplaceQuestionable(DhtBucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    time_t now = time(NULL), oldest_time = now;
    int oldest_i = -1, i = 0;

    for (i = 0; i < BUCKET_K; i++)
    {
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
