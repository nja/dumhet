#include <assert.h>
#include <stdlib.h>

#include <dht/table.h>
#include <lcthw/dbg.h>

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

    DhtBucket *bucket = DhtTable_FindBucket(table, &node->id);
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

            bucket = DhtTable_FindBucket(table, &node->id);
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

DhtBucket *DhtTable_FindBucket(DhtTable *table, DhtHash *id)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(id != NULL && "NULL DhtHash pointer");
    assert(table->end > 0 && "No buckets in table");

    int pfx = DhtHash_SharedPrefix(&table->id, id);
    int i = pfx < table->end ? pfx : table->end - 1;

    return table->buckets[i];
}

int DhtTable_ForEachNode(DhtTable *table, int (*operate)(DhtNode *))
{
    assert(operate != NULL && "NULL function pointer");

    DhtBucket **bucket = table->buckets;
    while (bucket < &table->buckets[table->end])
    {
        DhtNode **node = (*bucket)->nodes;
        while (node < &(*bucket)->nodes[BUCKET_K])
        {
            if (*node == NULL)
            {
                node++;
                continue;
            }

            int rc = operate(*node);
            check(rc == 0, "Operation on DhtNode failed");
            node++;
        }

        bucket++;
    }

    return 0;
error:
    return -1;
}

DhtNode *DhtTable_FindNode(DhtTable *table, DhtHash *id)
{
    assert(table != NULL && "NULL DhtTable pointer");
    assert(id != NULL && "NULL DhtHash pointer");

    DhtBucket *bucket = DhtTable_FindBucket(table, id);
    DhtNode **node = bucket->nodes;

    while (node < &bucket->nodes[BUCKET_K])
    {
        if (node != NULL && DhtHash_Equals(id, &(*node)->id))
        {
            return *node;
        }

        node++;
    }

    return NULL;
}

