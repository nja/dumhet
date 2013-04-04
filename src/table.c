#include <assert.h>
#include <stdlib.h>

#include <dht/close.h>
#include <dht/table.h>
#include <lcthw/dbg.h>

Table *Table_Create(Hash *id)
{
    assert(id != NULL && "NULL Hash id pointer");

    Table *table = calloc(1, sizeof(Table));
    check_mem(table);

    table->id = *id;
    
    Bucket *bucket = Table_AddBucket(table);
    check_mem(bucket);

    assert(table->end == 1 && "Wrong table end");
    assert(table->buckets[0] == bucket && "Wrong first bucket");
    assert(bucket->index == 0 && "Wrong index on bucket");

    return table;
error:
    return NULL;
}

void Table_Destroy(Table *table)
{
    if (table == NULL)
	return;

    int i = 0;
    for (i = 0; i < table->end; i++)
    {
	Bucket_Destroy(table->buckets[i]);
    }

    free(table);
}

void Table_DestroyNodes(Table *table)
{
    Table_ForEachNode(table, NULL, Node_DestroyOp);
}

int Table_HasShiftableNodes(Hash *id, Bucket *bucket, Node *node)
{
    assert(id != NULL && "NULL Hash pointer");
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(node != NULL && "NULL Node pointer");
    assert(bucket->index < MAX_TABLE_BUCKETS && "Bad bucket index");

    if (bucket->index < Hash_SharedPrefix(id, &node->id))
    {
	return 1;
    }

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        Node *current = bucket->nodes[i];
	if (current != NULL
	    && bucket->index < Hash_SharedPrefix(id, &current->id))
	{
	    return 1;
	}
    }

    return 0;
}

int Table_IsLastBucket(Table *table, Bucket *bucket)
{
    assert(table != NULL && "NULL Table pointer");
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(table->end <= MAX_TABLE_BUCKETS && "Too large table end");
    assert(table->end >= 0 && "Negative table end");

    return table->end - 1 == bucket->index;
}

int Table_CanAddBucket(Table *table)
{
    assert(table != NULL && "NULL Table pointer");

    return table->end < MAX_TABLE_BUCKETS;
}

int Table_ShiftBucketNodes(Table *table, Bucket *bucket)
{
    assert(table != NULL && "NULL Table pointer");
    assert(bucket != NULL && "NULL Bucket pointer");
    assert(bucket->index + 1 < table->end && "Shifting with too few buckets");

    Bucket *next = table->buckets[bucket->index + 1];
    assert(next != NULL && "NULL Bucket pointer");
    assert(!Bucket_IsFull(next) && "Shifting to full bucket");

    int i = 0;
    for (i = 0; i < BUCKET_K && !Bucket_IsFull(next); i++)
    {
	Node *node = bucket->nodes[i];

	if (node == NULL)
            continue;

	if (Hash_SharedPrefix(&table->id, &node->id) <= bucket->index)
            continue;

        int rc = Bucket_AddNode(next, node);
        check(rc == 0, "Bucket_AddNode failed");

        bucket->nodes[i] = NULL;
        bucket->count--;
    }

    assert(bucket->count >= 0 && "Negative Bucket count");

    return 0;
error:
    return -1;
}

Table_InsertNodeResult Table_InsertNode(Table *table, Node *node)
{
    assert(table != NULL && "NULL Table pointer");
    assert(node != NULL && "NULL Node pointer");

    Bucket *bucket = Table_FindBucket(table, &node->id);
    check(bucket != NULL, "Found no bucket for node");

    if (Bucket_ContainsNode(bucket, node)) {
	return (Table_InsertNodeResult)
	{ .rc = OKAlreadyAdded, .bucket = bucket, .replaced = NULL};
    }

    int rc;

    if (Bucket_IsFull(bucket))
    {
	Node *replaced = NULL;

	if ((replaced = Bucket_ReplaceBad(bucket, node))) {
	    return (Table_InsertNodeResult)
	    { .rc = OKReplaced, .bucket = bucket, replaced = replaced};
	}

	if ((replaced = Bucket_ReplaceQuestionable(bucket, node))) {
	    return (Table_InsertNodeResult)
	    { .rc = OKReplaced, .bucket = bucket, replaced = replaced};
	}

	if (Table_IsLastBucket(table, bucket)
            && Table_CanAddBucket(table)
	    && Table_HasShiftableNodes(&table->id, bucket, node))
	{
	    Bucket *new_bucket = Table_AddBucket(table);
	    check(new_bucket != NULL, "Error adding new bucket");

            rc = Table_ShiftBucketNodes(table, bucket);
            check(rc == 0, "Table_ShiftBucketNodes failed");

            bucket = Table_FindBucket(table, &node->id);
            check(bucket != NULL, "Found no bucket after adding one");
	}
    }

    if (Bucket_IsFull(bucket))
    {
        return (Table_InsertNodeResult)
        { .rc = OKFull, .bucket = NULL, .replaced = NULL};
    }

    rc = Bucket_AddNode(bucket, node);
    check(rc == 0, "Bucket_AddNode failed");

    return (Table_InsertNodeResult)
    { .rc = OKAdded, .bucket = bucket, .replaced = NULL};
error:
    return (Table_InsertNodeResult)
    { .rc = ERROR, .bucket = NULL, .replaced = NULL};
}

Bucket *Table_AddBucket(Table *table)
{
    assert(table->end < MAX_TABLE_BUCKETS && "Adding one bucket too many");

    Bucket *bucket = Bucket_Create();
    check_mem(bucket);
    
    bucket->index = table->end;
    table->buckets[table->end++] = bucket;

    return bucket;
error:
    return NULL;
}

Bucket *Table_FindBucket(Table *table, Hash *id)
{
    assert(table != NULL && "NULL Table pointer");
    assert(id != NULL && "NULL Hash pointer");
    assert(table->end > 0 && "No buckets in table");

    int pfx = Hash_SharedPrefix(&table->id, id);
    int i = pfx < table->end ? pfx : table->end - 1;

    assert(table->buckets[i] != NULL && "Found NULL bucket");

    return table->buckets[i];
}

int Table_ForEachNode(Table *table, void *context, NodeOp operate)
{
    assert(table != NULL && "NULL Table pointer");
    assert(operate != NULL && "NULL function pointer");

    Bucket **bucket = table->buckets;
    while (bucket < &table->buckets[table->end])
    {
        Node **node = (*bucket)->nodes;
        while (node < &(*bucket)->nodes[BUCKET_K])
        {
            if (*node == NULL)
            {
                node++;
                continue;
            }

            int rc = operate(context, *node);
            check(rc == 0, "Operation on Node failed");
            node++;
        }

        bucket++;
    }

    return 0;
error:
    return -1;
}

Node *Table_FindNode(Table *table, Hash *id)
{
    assert(table != NULL && "NULL Table pointer");
    assert(id != NULL && "NULL Hash pointer");

    Bucket *bucket = Table_FindBucket(table, id);
    Node **node = bucket->nodes;

    while (node < &bucket->nodes[BUCKET_K])
    {
        if (*node != NULL && Hash_Equals(id, &(*node)->id))
        {
            return *node;
        }

        node++;
    }

    return NULL;
}

int CloseNodes_AddOp(void *close, Node *node)
{
    return CloseNodes_Add((CloseNodes *)close, node);
}

DArray *Table_GatherClosest(Table *table, Hash *id)
{
    assert(table != NULL && "NULL Table pointer");
    assert(id != NULL && "NULL Hash pointer");

    CloseNodes *close = CloseNodes_Create(id);
    check(close != NULL, "CloseNodes_Create failed");

    int rc = Table_ForEachNode(table, close, CloseNodes_AddOp);
    check(rc == 0, "Table_ForEachNode failed");

    DArray *found = CloseNodes_GetNodes(close);
    check(found != NULL, "CloseNodes_GetNodes failed");

    CloseNodes_Destroy(close);

    return found;
error:
    CloseNodes_Destroy(close);
    return NULL;
}

int Table_MarkReply(Table *table, Hash *id)
{
    assert(table != NULL && "NULL Table pointer");
    assert(id != NULL && "NULL Hash pointer");

    Node *node = Table_FindNode(table, id);

    if (node == NULL)
        return 0;

    check(node->pending_queries > 0, "No pending queries on node");

    node->reply_time = time(NULL);
    node->pending_queries--;

    return 0;
error:
    return -1;
}

void Table_MarkQuery(Table *table, Hash *id)
{
    assert(table != NULL && "NULL Table pointer");
    assert(id != NULL && "NULL Hash pointer");

    Node *node = Table_FindNode(table, id);

    if (node == NULL)
        return;

    node->query_time = time(NULL);

    return;
}
