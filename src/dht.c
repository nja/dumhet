#include <stdlib.h>
#include <dht.h>
#include <dbg.h>
#include <assert.h>
#include <time.h>

/* DhtHash */

DhtHash *DhtHash_Clone(DhtHash *hash)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");

    DhtHash *clone = malloc(sizeof(DhtHash));
    check_mem(clone);

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	clone->value[i] = hash->value[i];
    }

    return clone;
error:
    return NULL;
}

void DhtHash_Destroy(DhtHash *hash)
{
    assert(hash != NULL && "NULL DhtHash pointer");
    free(hash);
}

DhtHash *DhtHash_Random(RandomState *rs)
{
    assert(rs != NULL && "NULL RandomState pointer");

    DhtHash *hash = malloc(sizeof(DhtHash));
    check_mem(hash);

    int rc = Random_Fill(rs, hash->value, HASH_BYTES);
    check(rc == 0, "Random_Fill failed");

    return hash;
error:
    return NULL;
}

DhtHash *DhtHash_Prefixed(DhtHash *hash, DhtHash *prefix, int prefix_len)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    DhtHash *result = DhtHash_Clone(hash);
    check(result != NULL, "DhtHash_Clone failed");

    int i = 0;

    while (prefix_len >= 8)
    {
	result->value[i] = prefix->value[i];

	prefix_len -= 8;
	i++;
    }

    if (i < HASH_BYTES)
    {
	char mask = ((char)~0) << (8 - prefix_len);
	result->value[i] &= ~mask;
	result->value[i] |= mask & prefix->value[i];
    }

    return result;
error:
    return NULL;
}

DhtHash *DhtHash_PrefixedRandom(RandomState *rs, DhtHash *prefix, int prefix_len)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    DhtHash *random = NULL;

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    random = DhtHash_Random(rs);
    check(random != NULL, "DhtHash_Random failed");

    DhtHash *prefixed = DhtHash_Prefixed(random, prefix, prefix_len);
    check(prefixed != NULL, "DhtHash_Prefixed failed");

    DhtHash_Destroy(random);

    return prefixed;
error:
    DhtHash_Destroy(random);

    return NULL;
}

int DhtHash_SharedPrefix(DhtHash *a, DhtHash *b)
{
    int hi = 0, bi = 0;

    for (hi = 0; hi < HASH_BYTES; hi++)
    {
	if (a->value[hi] == b->value[hi])
	{
	    bi += 8;
	    continue;
	}

	uint8_t mask = 1 << 7;
	while (mask)
	{
	    if ((a->value[hi] & mask) == (b->value[hi] & mask))
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
    return bi;
}

/* DhtDistance */

DhtDistance *DhtHash_Distance(DhtHash *a, DhtHash *b)
{
    assert(a != NULL && b != NULL && "NULL DhtHash pointer");

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
    assert(a != NULL && b != NULL && "NULL DhtDistance pointer");

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

/* DhtBucket */

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
    assert(bucket != NULL && "NULL DhtBucket pointer");
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

/* DhtTable */

DhtTable *DhtTable_Create(DhtHash *id)
{
    assert(id != NULL && "NULL DhtHash id pointer");

    DhtTable *table = calloc(1, sizeof(DhtTable));
    check_mem(table);

    table->id = *id;
    
    DhtBucket *bucket = DhtTable_AddBucket(table);
    check_mem(bucket);

    assert(table->end == 1 && "Wrong table end");
    assert(table->buckets[0] == bucket && "Wrong first bucket");
    assert(bucket->index == 0 && "Wrong index on bucket");

    return table;
error:
    return NULL;
}

void DhtTable_Destroy(DhtTable *table)
{
    assert(table != NULL && "NULL DhtTable pointer");

    int i = 0;
    for (i = 0; i < table->end; i++)
    {
	DhtBucket_Destroy(table->buckets[i]);
    }

    free(table);
}

int DhtTable_HasShiftableNodes(DhtHash *id, DhtBucket *bucket, DhtNode *node)
{
    assert(id != NULL && "NULL DhtHash pointer");
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");
    assert(bucket->index < HASH_BITS && "Bad bucket index");

    if (bucket->index == HASH_BITS - 1)
	return 0;

    if (bucket->index < DhtHash_SharedPrefix(id, &node->id))
    {
	return 1;
    }

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	if (bucket->nodes[i] != NULL
	    && bucket->index < DhtHash_SharedPrefix(id, &bucket->nodes[i]->id))
	{
	    return 1;
	}
    }

    return 0;
}

int DhtTable_IsLastBucket(DhtTable *table, DhtBucket *bucket)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(bucket != NULL && "NULL DhtBucket pointer");

    return table->end - 1 == bucket->index;
}

DhtTable_InsertNodeResult DhtTable_InsertNode(DhtTable *table, DhtNode *node)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    DhtBucket *bucket = DhtTable_FindBucket(table, node);
    check(bucket != NULL, "Found no bucket for node");

    if (DhtBucket_ContainsNode(bucket, node)) {
	return (DhtTable_InsertNodeResult)
	{ .rc = OKAlreadyAdded, .bucket = bucket, .replaced = NULL};
    }

    if (bucket->count == BUCKET_K)
    {
	DhtNode *replaced = NULL;

	if ((replaced = DhtBucket_ReplaceBad(bucket, node))) {
	    return (DhtTable_InsertNodeResult)
	    { .rc = OKReplaced, .bucket = bucket, replaced = replaced};
	}

	if ((replaced = DhtBucket_ReplaceQuestionable(bucket, node))) {
	    return (DhtTable_InsertNodeResult)
	    { .rc = OKReplaced, .bucket = bucket, replaced = replaced};
	}

	if (DhtTable_IsLastBucket(table, bucket)
	    && DhtTable_HasShiftableNodes(&table->id, bucket, node))
	{
	    DhtBucket *new_bucket = DhtTable_AddBucket(table);
	    check(new_bucket != NULL, "Error adding new bucket");

	    DhtTable_InsertNodeResult result = DhtTable_InsertNode(table, node);
	    check(result.rc != ERROR, "Insert when growing failed");
	    assert(result.bucket != NULL && "NULL bucket");
	    assert(result.replaced == NULL && "Unexpected replaced node");

	    return result;
	}

	return (DhtTable_InsertNodeResult)
	{ .rc = OKFull, .bucket = NULL, replaced = NULL};
    }

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	if (bucket->nodes[i] == NULL)
	{
	    bucket->nodes[i] = node;
	    bucket->count++;

	    bucket->change_time = time(NULL);

	    return (DhtTable_InsertNodeResult)
	    { .rc = OKAdded, .bucket = bucket, .replaced = NULL};
	}
    }

    sentinel("Bucket with full count but no empty slots");

error:
    return (DhtTable_InsertNodeResult)
    { .rc = ERROR, .bucket = NULL, .replaced = NULL};
}

int DhtTable_ReaddBucketNodes(DhtTable *table, DhtBucket *bucket)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(bucket != NULL && "NULL DhtBucket pointer");

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	DhtNode *node = bucket->nodes[i];

	if (node == NULL) continue;
	
	bucket->nodes[i] = NULL;
	bucket->count--;

	DhtTable_InsertNodeResult result = DhtTable_InsertNode(table, node);
	check(result.rc == OKAdded, "Readd failed");

	assert(result.bucket != NULL && "NULL bucket from insert");
	assert(result.replaced == NULL && "Unexpected replaced");
    }

    return 1;
error:
    return 0;
}

DhtBucket *DhtTable_AddBucket(DhtTable *table)
{
    assert(table->end < HASH_BITS && "Adding one bucket too many");

    DhtBucket *bucket = DhtBucket_Create();
    check_mem(bucket);
    
    bucket->index = table->end;
    table->buckets[table->end++] = bucket;

    if (bucket->index > 0)
    {
	DhtBucket *prev_bucket = table->buckets[bucket->index - 1];
	int rc = DhtTable_ReaddBucketNodes(table, prev_bucket);
	check(rc, "Readding nodes failed");
    }

    return bucket;
error:
    return NULL;
}

DhtBucket *DhtTable_FindBucket(DhtTable *table, DhtNode *node)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(node != NULL && "NULL DhtNode pointer");

    int pfx = DhtHash_SharedPrefix(&table->id, &node->id);
    int i = pfx < table->end ? pfx : table->end - 1;

    return table->buckets[i];
}

/* DhtNode */

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
