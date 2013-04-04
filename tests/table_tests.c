#include <time.h>

#include "minunit.h"
#include <dht/table.h>
#include <lcthw/darray.h>

char *test_DhtTable_AddBucket()
{
    int i = 0;
    DhtTable table = {.id = {{0}}, .buckets = {0}, .end = 0};
    DhtNode node = {.id = {{0}}, {0}};

    for (i = 0; i < HASH_BYTES; i++)
    {
	table.id.value[i] = 0xAA;
	node.id.value[i] = 0xAA;
    }
    node.id.value[HASH_BYTES - 1]++;

    for (i = 0; i < MAX_TABLE_BUCKETS; i++)
    {
	Bucket *added = DhtTable_AddBucket(&table);
	Bucket *found = DhtTable_FindBucket(&table, &node.id);

	mu_assert(table.end == i + 1, "Wrong bucket count");
	mu_assert(added == found, "Found wrong bucket");
    }

    for (i = 0; i < MAX_TABLE_BUCKETS; i++)
    {
	Bucket *found = DhtTable_FindBucket(&table, &node.id);
	mu_assert(found == table.buckets[table.end - 1], "Expected last bucket");
	
	node.id.value[i / 8] ^= (0x80 >> (i % 8));
	found = DhtTable_FindBucket(&table, &node.id);

	mu_assert(found == table.buckets[i], "Found wrong bucket");
	node.id.value[i / 8] ^= (0x80 >> (i % 8));
    }

    for (i = 0; i < MAX_TABLE_BUCKETS; i++)
    {
	free(table.buckets[i]);
    }

    return NULL;
}

char *test_DhtTable_InsertNode()
{
    DhtHash id = {{0}};
    
    DhtTable *table = DhtTable_Create(&id);

    /* good and bad are near the id and shiftable to new buckets */
    DhtNode *good_nodes = calloc(BUCKET_K + 1, sizeof(DhtNode));
    DhtNode *bad_nodes = calloc(BUCKET_K + 1, sizeof(DhtNode));

    /* far only fit in the first bucket */
    DhtNode *far_nodes = calloc(BUCKET_K + 1, sizeof(DhtNode));
    
    time_t now = time(NULL);

    /* create nodes */
    int i = 0;
    for (i = 0; i < BUCKET_K + 1; i++)
    {
	good_nodes[i].id.value[0] = id.value[0];
	good_nodes[i].id.value[1] = ~i;
	good_nodes[i].reply_time = now;
	mu_assert(DhtNode_Status(&good_nodes[i], now) == Good, "Wrong status");

	bad_nodes[i].id.value[0] = id.value[0];
	bad_nodes[i].id.value[2] = ~i;
	bad_nodes[i].pending_queries = NODE_MAX_PENDING;
	mu_assert(DhtNode_Status(&bad_nodes[i], now) == Bad, "Wrong status");

	far_nodes[i].id.value[0] = ~id.value[0];
	far_nodes[i].id.value[3] = ~i;
	far_nodes[i].reply_time = now;
	mu_assert(DhtNode_Status(&far_nodes[i], now) == Good, "Wrong status");
    }

    DhtTable_InsertNodeResult result;

    /* fill first bucket with bad nodes */
    for (i = 0; i < BUCKET_K; i++)
    {
	result = DhtTable_InsertNode(table, &bad_nodes[i]);
	mu_assert(result.rc == OKAdded, "Wrong result");
	mu_assert(result.bucket == table->buckets[0], "Wrong bucket");
	mu_assert(result.replaced == NULL, "Bad replaced");
    }

    /* replace bad nodes with good */
    for (i = 0; i < BUCKET_K; i++)
    {
	result = DhtTable_InsertNode(table, &good_nodes[i]);
	mu_assert(result.rc == OKReplaced, "Wrong result");
	mu_assert(result.replaced != NULL, "Nothing replaced");
	mu_assert(&bad_nodes[0] <= result.replaced
		  && result.replaced < &bad_nodes[BUCKET_K], "Wrong replaced");
	mu_assert(result.bucket == table->buckets[0], "Wrong bucket");
    }

    /* readd a single good node */
    result = DhtTable_InsertNode(table, &good_nodes[0]);
    mu_assert(result.rc == OKAlreadyAdded, "Wrong result");
    mu_assert(result.replaced == NULL, "Bad replaced");
    mu_assert(result.bucket == table->buckets[0], "Wrong bucket");

    /* adding far nodes, will shift good nodes to second bucket */
    for (i = 0; i < BUCKET_K; i++)
    {
	result = DhtTable_InsertNode(table, &far_nodes[i]);
	mu_assert(result.rc == OKAdded, "Wrong result");
	mu_assert(result.bucket == table->buckets[0], "Wrong bucket");
	mu_assert(result.replaced == NULL, "Bad replaced");
    }

    /* the ninth far node will not be added: the first bucket is full */
    result = DhtTable_InsertNode(table, &far_nodes[BUCKET_K]);
    mu_assert(result.rc == OKFull, "Wrong result");
    mu_assert(result.bucket == NULL, "Bad bucket");
    mu_assert(result.replaced == NULL, "Bad replaced");
    
    /* readd a single good node  */
    result = DhtTable_InsertNode(table, &good_nodes[0]);
    mu_assert(result.rc == OKAlreadyAdded, "Wrong result");
    mu_assert(result.replaced == NULL, "Bad replaced");
    mu_assert(result.bucket == table->buckets[1], "Wrong bucket");

    mu_assert(table->end == 2, "Too many buckets");

    free(good_nodes);
    free(bad_nodes);
    free(far_nodes);
    DhtTable_Destroy(table);

    return NULL;
}

char *test_DhtTable_InsertNode_FullTable()
{
    DhtHash id = {{0}};
    DhtHash inv = {{0}};

    DhtHash_Invert(&inv);

    const int low_bits = 3;
    mu_assert(1 << low_bits >= BUCKET_K, "Not enough bits");

    DhtTable *table = DhtTable_Create(&id);
    mu_assert(table != NULL, "DhtTable_Create failed");
    
    DhtTable_InsertNodeResult result;

    DhtNode *node = DhtNode_Create(&id);
    result = DhtTable_InsertNode(table, node);
    mu_assert(result.rc == OKAdded, "Error adding id node");

    int i = 0;
    for (i = 0; i < HASH_BITS; i++)
    {
        int suffix = HASH_BITS - i;
        int shift = low_bits < suffix ? low_bits : suffix - 1;

        int j = 0;
        for (j = 0; j < BUCKET_K && j < 1 << shift; j++)
        {
            node = DhtNode_Create(&inv);

            node->id.value[HASH_BYTES - 1] &= ~0 << shift;
            node->id.value[HASH_BYTES - 1] |= j;

            DhtHash_Prefix(&node->id, &id, i);

            result = DhtTable_InsertNode(table, node);
            mu_assert(result.rc == OKAdded, "Error adding");
        }
    }

    for (i = 0; i < table->end; i++)
    {
        mu_assert(table->buckets[i]->count == BUCKET_K, "Bucket not full");
    }

    for (i = 0; i < MAX_TABLE_BUCKETS - 2; i++)
    {
        node = DhtNode_Create(&inv);
        DhtHash_Prefix(&node->id, &id, i);
        node->id.value[HASH_BYTES - 1] &= 0xf0;
        
        result = DhtTable_InsertNode(table, node);
        mu_assert(result.rc == OKFull, "Should be full");

        DhtNode_Destroy(node);
    }

    DhtTable_DestroyNodes(table);
    DhtTable_Destroy(table);

    return NULL;
}

char *test_DhtTable_InsertNode_AddBucket()
{
    DhtHash id = {{ 0 }};
    DhtTable *table = DhtTable_Create(&id);

    DhtNode *node;
    DhtTable_InsertNodeResult result;

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        node = DhtNode_Create(&id);
        node->id.value[HASH_BYTES - 1] = i;
        result = DhtTable_InsertNode(table, node);

        mu_assert(result.rc == OKAdded, "Wrong rc");
        mu_assert(table->end == 1, "Added unnecessary bucket");
    }

    node = DhtNode_Create(&id);
    node->id.value[HASH_BYTES - 1] = i;
    result = DhtTable_InsertNode(table, node);

    mu_assert(result.rc == OKFull, "Wrong rc");
    mu_assert(table->end == 2, "Added more than one bucket");

    node->id.value[0] = ~0;

    result = DhtTable_InsertNode(table, node);
    mu_assert(result.rc == OKAdded, "Wrong rc");
    mu_assert(table->end == 2, "Added bucket without reason");

    DhtTable_DestroyNodes(table);
    DhtTable_Destroy(table);

    return NULL;
}

DhtNode **MakeNodes(int count, char high)
{
    DhtNode **nodes = calloc(count, sizeof(DhtNode *));

    int i = 0;
    for (i = 0; i < count; i++)
    {
        DhtHash id = {{ 0 }};
        id.value[0] = high;
        id.value[2] = i + 1;
        nodes[i] = DhtNode_Create(&id);
        nodes[i]->reply_time = time(NULL);
    }

    return nodes;
}

int HasNode(DArray *nodes, DhtNode *node)
{
    int i = 0;
    for (i = 0; i < DArray_end(nodes); i++)
    {
        if (node == DArray_get(nodes, i))
            return 1;
    }

    return 0;
}

char *test_DhtTable_GatherClosest()
{
    DhtHash id = {{ 0 }};

    int i = 0;
    for (i = 0; i <= BUCKET_K; i++)
    {
        DhtTable *table = DhtTable_Create(&id);

        int far = i;
        int close = BUCKET_K - i;

        DhtNode **far_nodes = MakeNodes(far, 0x40);
        DhtNode **close_nodes = MakeNodes(close, 0x20);
        DhtNode **filler_nodes = MakeNodes(BUCKET_K, 0x80);

        DhtTable_InsertNodeResult result;
        int j = 0;
        for (j = 0; j < BUCKET_K; j++)
        {
            result = DhtTable_InsertNode(table, filler_nodes[j]);
            mu_assert(result.rc == OKAdded, "add");
        }

        for (j = 0; j < far; j++)
        {
            result = DhtTable_InsertNode(table, far_nodes[j]);
            mu_assert(result.rc == OKAdded, "add");
        }
        
        for (j = 0; j < close; j++)
        {
            result = DhtTable_InsertNode(table, close_nodes[j]);
            mu_assert(result.rc == OKAdded, "add");
        }            

        DhtHash target = {{ 0 }};
        target.value[1] = 16;

        DArray *found = DhtTable_GatherClosest(table, &target);

        for (j = 0; j < BUCKET_K; j++)
            mu_assert(!HasNode(found, filler_nodes[j]), "Filler node found");

        for (j = 0; j < far; j++)
            mu_assert(HasNode(found, far_nodes[j]), "Far node missing");

        for (j = 0; j < close; j++)
            mu_assert(HasNode(found, close_nodes[j]), "Close node missing");

        DhtTable_ForEachNode(table, NULL, DhtNode_DestroyOp);
        DhtTable_Destroy(table);
        DArray_destroy(found);

        free(far_nodes);
        free(close_nodes);
        free(filler_nodes);
    }

    return NULL;
}

char *test_DhtTable_FindNode_EmptyBucket()
{
    DhtHash id = { "id" };
    DhtTable *table = DhtTable_Create(&id);

    DhtNode *node = DhtTable_FindNode(table, &id);
    mu_assert(node == NULL, "Mystery node found");

    DhtTable_Destroy(table);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtTable_AddBucket);
    mu_run_test(test_DhtTable_InsertNode);
    mu_run_test(test_DhtTable_InsertNode_FullTable);
    mu_run_test(test_DhtTable_InsertNode_AddBucket);
    mu_run_test(test_DhtTable_GatherClosest);
    mu_run_test(test_DhtTable_FindNode_EmptyBucket);

    return NULL;
}

RUN_TESTS(all_tests);
