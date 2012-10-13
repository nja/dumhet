#include <stdlib.h>
#include <dht.h>
#include <dbg.h>
#include <assert.h>
#include <time.h>

void DhtHash_Destroy(DhtHash *hash)
{
    free(hash);
}

DhtDistance *DhtHash_Distance(DhtHash *a, DhtHash *b)
{
    DhtDistance *distance = malloc(sizeof(DhtDistance));
    check_mem(distance);

    int i = 0;

    for (i = 0; i < HASH_BYTES; i++)
    {
	distance->value[i] = a->value[i] ^ b->value[i];
    }

    return distance;
error:
    return NULL;
}

int DhtDistance_Compare(DhtDistance *a, DhtDistance *b)
{
    int i = 0;

    for (i = 0; i < HASH_BYTES; i++)
    {
	if (a->value[i] < b->value[i])
	    return -1;

	if (b->value[i] < a->value[i])
	    return 1;
    }

    return 0;
}

DhtBucket *DhtBucket_Create()
{
    DhtBucket *bucket = calloc(1, sizeof(DhtBucket));
    check_mem(bucket);

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
    time_t now = time(NULL);

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	assert(bucket->nodes[i] != NULL && "Empty slot in full bucket");

	if (DhtNode_Status(bucket->nodes[i], now) == Bad)
	{
	    DhtNode *replaced = bucket->nodes[i];
	    bucket->nodes[i] = node;

	    return replaced;
	}
    }

    return NULL;
}

DhtNode *DhtBucket_ReplaceQuestionable(DhtBucket *bucket, DhtNode *node)
{
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

	return replaced;
    }

    return NULL;
}

DhtTable *DhtTable_Create(DhtHash *id)
{
    DhtTable *table = calloc(1, sizeof(DhtTable));
    check_mem(table);

    table->id = *id;
    
    DhtBucket *bucket = DhtTable_AddBucket(table);
    check_mem(bucket);

    return table;
error:
    return NULL;
}

void DhtTable_Destroy(DhtTable *table)
{
    int i = 0;
    for (i = 0; i < table->end; i++)
    {
	DhtBucket_Destroy(table->buckets[i]);
    }

    free(table);
}    

DhtBucket *DhtTable_InsertNode(DhtTable *table, DhtNode *node, DhtNode **replaced)
{
    DhtBucket *bucket = DhtTable_FindBucket(table, node);
    check(bucket != NULL, "Found no bucket for node");

    if (DhtBucket_ContainsNode(bucket, node))
	return bucket;

    if (bucket->count == BUCKET_K)
    {
	debug("Full bucket, replacing bad");

	if ((*replaced = DhtBucket_ReplaceBad(bucket, node)))
	    return bucket;

	debug("Replacing questionable");
	if ((*replaced = DhtBucket_ReplaceQuestionable(bucket, node)))
	    return bucket;

	if (table->end - 1 == bucket->index && table->end < HASH_BITS)
	{
	    debug("Adding bucket");

	    DhtBucket *new_bucket = DhtTable_AddBucket(table);
	    check(new_bucket != NULL, "Error adding new bucket");

	    DhtBucket *added_to = DhtTable_InsertNode(table, node, replaced);
	    assert(*replaced == NULL && "Unexpected replaced node");

	    return added_to;
	}

	return NULL;
    }

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	if (bucket->nodes[i] == NULL)
	{
	    bucket->nodes[i] = node;
	    bucket->count++;
	    return bucket;
	}
    }

    sentinel("Bucket with full count but no empty slots");

error:
    // TODO: return error marker?
    return NULL;
}

void DhtTable_ReaddBucketNodes(DhtTable *table, DhtBucket *bucket)
{
    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	DhtNode *node = bucket->nodes[i];

	if (node == NULL) continue;
	
	bucket->nodes[i] = NULL;
	bucket->count--;

	DhtNode *replaced = NULL;
	DhtTable_InsertNode(table, node, &replaced);
	assert(replaced == NULL && "Unexpected replaced");
    }    
}

DhtBucket *DhtTable_AddBucket(DhtTable *table)
{
    assert(table->end < HASH_BITS && "Adding one bucket too many");
    DhtBucket *bucket = NULL;

    bucket = DhtBucket_Create();
    check_mem(bucket);
    bucket->index = table->end;
    table->buckets[table->end++] = bucket;

    if (table->end > 1)
    {
	DhtBucket *prev_bucket = table->buckets[table->end - 1];
	DhtTable_ReaddBucketNodes(table, prev_bucket);
    }

    return bucket;
error:
    return NULL;
}

DhtBucket *DhtTable_FindBucket(DhtTable *table, DhtNode *node)
{
    int hi = 0, bi = 0;

    for (hi = 0; hi < HASH_BYTES && bi < table->end; hi++)
    {
	if (table->id.value[hi] == node->id.value[hi])
	{
	    bi += 8;
	    continue;
	}

	uint8_t mask = 0x80;
	while (mask)
	{
	    if ((table->id.value[hi] & mask) == (node->id.value[hi] & mask))
	    {
		bi++;
		mask = mask >> 1;
	    }
	    else
	    {
		goto done;
	    }
	}
		
    }

done:
    return table->buckets[bi < table->end ? bi : table->end - 1];
}

DhtNodeStatus DhtNode_Status(DhtNode *node, time_t time)
{
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
