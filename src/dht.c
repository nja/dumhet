#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <dht/dht.h>
#include <lcthw/dbg.h>

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
    free(hash);
}

int DhtHash_Random(RandomState *rs, DhtHash *hash)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(hash != NULL && "NULL DhtHash pointer");

    int rc = Random_Fill(rs, (char *)hash->value, HASH_BYTES);
    check(rc == 0, "Random_Fill failed");

    return 0;
error:
    return -1;
}

int DhtHash_Prefix(DhtHash *hash, DhtHash *prefix, int prefix_len)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    int i = 0;

    while (prefix_len >= 8)
    {
	hash->value[i] = prefix->value[i];

	prefix_len -= 8;
	i++;
    }

    if (i < HASH_BYTES)
    {
	char mask = ((char)~0) << (8 - prefix_len);
	hash->value[i] &= ~mask;
	hash->value[i] |= mask & prefix->value[i];
    }

    return 0;
error:
    return -1;
}

int DhtHash_PrefixedRandom(RandomState *rs, DhtHash *hash, DhtHash *prefix, int prefix_len)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    int rc = DhtHash_Random(rs, hash);
    check(rc == 0, "DhtHash_Random failed");

    rc = DhtHash_Prefix(hash, prefix, prefix_len);
    check(rc == 0, "DhtHash_Prefix failed");

    return 0;
error:
    return -1;
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
        uint8_t xor = a->value[hi] ^ b->value[hi];
	while (mask)
	{
            if (!(mask & xor))
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
	if ((unsigned char)a->value[i] < (unsigned char)b->value[i])
	    return -1;

	if ((unsigned char)b->value[i] < (unsigned char)a->value[i])
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
    if (table == NULL)
	return;

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

    if (bucket->index < DhtHash_SharedPrefix(id, &node->id))
    {
	return 1;
    }

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        DhtNode *current = bucket->nodes[i];
	if (current != NULL
	    && bucket->index < DhtHash_SharedPrefix(id, &current->id))
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
    assert(table->end <= HASH_BITS && "Too large table end");
    assert(table->end >= 0 && "Negative table end");

    return table->end - 1 == bucket->index;
}

int DhtTable_CanAddBucket(DhtTable *table)
{
    assert(table != NULL && "NULL DhtTable pointer");

    return table->end < HASH_BITS;
}

int DhtBucket_IsFull(DhtBucket *bucket)
{
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(bucket->count >= 0 && "Negative DhtBucket count");
    assert(bucket->count <= BUCKET_K && "Too large DhtBucket count");

    return BUCKET_K == bucket->count;
}

int DhtBucket_AddNode(DhtBucket *bucket, DhtNode *node)
{
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(node != NULL && "NULL DhtNode pointer");
    assert(bucket->count >= 0 && "Negative DhtBucket count");
    assert(bucket->count <= BUCKET_K && "Too large DhtBucket count");

    check(!DhtBucket_IsFull(bucket), "Bucket full");

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        if (bucket->nodes[i] == NULL)
        {
            bucket->nodes[i] = node;
            bucket->count++;
            bucket->change_time = time(NULL);

            assert(bucket->count <= BUCKET_K && "Too large DhtBucket count");

            return 0;
        }
    }

    log_err("Bucket with count %d had no empty slots", bucket->count);
error:
    return -1;
}

int DhtTable_ShiftBucketNodes(DhtTable *table, DhtBucket *bucket)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(bucket != NULL && "NULL DhtBucket pointer");
    assert(bucket->index + 1 < table->end && "Shifting with too few buckets");

    DhtBucket *next = table->buckets[bucket->index + 1];
    assert(next != NULL && "NULL DhtBucket pointer");
    assert(!DhtBucket_IsFull(next) && "Shifting to full bucket");

    int i = 0;
    for (i = 0; i < BUCKET_K && !DhtBucket_IsFull(next); i++)
    {
	DhtNode *node = bucket->nodes[i];

	if (node == NULL)
            continue;

	if (DhtHash_SharedPrefix(&table->id, &node->id) <= bucket->index)
            continue;

        int rc = DhtBucket_AddNode(next, node);
        check(rc == 0, "DhtBucket_AddNode failed");

        bucket->nodes[i] = NULL;
        bucket->count--;
    }

    assert(bucket->count >= 0 && "Negative DhtBucket count");

    return 0;
error:
    return -1;
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

    int rc;

    if (DhtBucket_IsFull(bucket))
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
            && DhtTable_CanAddBucket(table)
	    && DhtTable_HasShiftableNodes(&table->id, bucket, node))
	{
	    DhtBucket *new_bucket = DhtTable_AddBucket(table);
	    check(new_bucket != NULL, "Error adding new bucket");

            rc = DhtTable_ShiftBucketNodes(table, bucket);
            check(rc == 0, "DhtTable_ShiftBucketNodes failed");

            bucket = DhtTable_FindBucket(table, node);
            check(bucket != NULL, "Found no bucket after adding one");
	}
    }

    if (DhtBucket_IsFull(bucket))
    {
        return (DhtTable_InsertNodeResult)
        { .rc = OKFull, .bucket = NULL, .replaced = NULL};
    }

    rc = DhtBucket_AddNode(bucket, node);
    check(rc == 0, "DhtBucket_AddNode failed");

    return (DhtTable_InsertNodeResult)
    { .rc = OKAdded, .bucket = bucket, .replaced = NULL};
error:
    return (DhtTable_InsertNodeResult)
    { .rc = ERROR, .bucket = NULL, .replaced = NULL};
}

DhtBucket *DhtTable_AddBucket(DhtTable *table)
{
    assert(table->end < HASH_BITS && "Adding one bucket too many");

    DhtBucket *bucket = DhtBucket_Create();
    check_mem(bucket);
    
    bucket->index = table->end;
    table->buckets[table->end++] = bucket;

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

void DhtNode_Destroy(DhtNode *node)
{
    free(node);
}

void DhtNode_DestroyBlock(DhtNode *node, size_t count)
{
    assert(((node == NULL && count == 0)
	    || (node != NULL && count > 0)) && "Bad count for block");
    count = count;		/* Unused */
    free(node);
}
