#include <time.h>

#include "minunit.h"
#include <dht/dht.h>
#include <lcthw/darray.h>

char *test_DhtHash_Clone()
{
    DhtHash *orig = malloc(sizeof(DhtHash));

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	orig->value[i] = i;
    }

    DhtHash *clone = DhtHash_Clone(orig);
    mu_assert(clone != NULL, "DhtHash_Clone failed");
    mu_assert(clone != orig, "Not cloned");

    int cmp = DhtDistance_Compare(orig, clone);
    mu_assert(cmp == 0, "Clone different from original");

    DhtHash_Destroy(orig);
    DhtHash_Destroy(clone);

    return NULL;
}

void DhtHash_Invert(DhtHash *hash)
{
    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	hash->value[i] = ~hash->value[i];
    }
}

char *test_DhtHash_Prefix()
{
    DhtHash prefix = {{ "1234567890qwertyuio" }};
    DhtHash inv = prefix;

    DhtHash_Invert(&inv);

    int i = 0;
    for (i = 0; i <= HASH_BITS; i++)
    {
	DhtHash result = inv;

        int rc = DhtHash_Prefix(&result, &prefix, i);
        mu_assert(rc == 0, "DhtHash_Prefix failed");

	int shared = DhtHash_SharedPrefix(&result, &prefix);
	mu_assert(shared == i, "Wrong prefix");
    }

    return NULL;
}

char *test_DhtHash_PrefixedRandom()
{
    DhtHash prefix = {{ 0 }};
    
    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	prefix.value[i] = i;
    }

    RandomState *rs = RandomState_Create(0);

    for (i = 0; i <= HASH_BITS; i++)
    {
	DhtHash random = {{ 0 }};

        int rc = DhtHash_PrefixedRandom(rs, &random, &prefix, i);
        mu_assert(rc == 0, "DhtHash_PrefixedRandom failed");

	int shared = DhtHash_SharedPrefix(&prefix, &random);

	mu_assert(shared >= i, "Wrong prefix");
    }

    RandomState_Destroy(rs);
    return NULL;
}

char *test_DhtHash_Distance()
{
    DhtHash ha = {{0}}, hb = {{0}}, hc = {{0}};
    DhtDistance zero_d = {{0}};

    DhtDistance *daa = DhtHash_Distance(&ha, &ha);

    mu_assert(DhtDistance_Compare(&zero_d, daa) == 0, "Wrong distance");

    hb.value[0] = 0xE0;
    hc.value[0] = 0xF0;

    DhtDistance *dab = DhtHash_Distance(&ha, &hb);
    DhtDistance *dac = DhtHash_Distance(&ha, &hc);

    mu_assert(DhtDistance_Compare(daa, dab) < 0, "Wrong distance");
    mu_assert(DhtDistance_Compare(dab, dac) < 0, "Wrong distance");

    DhtDistance_Destroy(daa);
    DhtDistance_Destroy(dab);
    DhtDistance_Destroy(dac);

    return NULL;
}

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

    for (i = 0; i < HASH_BITS; i++)
    {
	DhtBucket *added = DhtTable_AddBucket(&table);
	DhtBucket *found = DhtTable_FindBucket(&table, &node);

	mu_assert(table.end == i + 1, "Wrong bucket count");
	mu_assert(added == found, "Found wrong bucket");
    }

    for (i = 0; i < HASH_BITS; i++)
    {
	DhtBucket *found = DhtTable_FindBucket(&table, &node);
	mu_assert(found == table.buckets[table.end - 1], "Expected last bucket");
	
	node.id.value[i / 8] ^= (0x80 >> (i % 8));
	found = DhtTable_FindBucket(&table, &node);

	mu_assert(found == table.buckets[i], "Found wrong bucket");
	node.id.value[i / 8] ^= (0x80 >> (i % 8));
    }

    for (i = 0; i < HASH_BITS; i++)
    {
	free(table.buckets[i]);
    }

    return NULL;
}

char *test_DhtNode_Status()
{
    time_t now = time(NULL);
    DhtHash id = {{ 0 }};

    DhtNode *node = DhtNode_Create(&id);

    for (node->pending_queries = 0; node->pending_queries < NODE_MAX_PENDING;
	 node->pending_queries++) {
	mu_assert(DhtNode_Status(node, now) == Unknown, "Wrong status");
    }

    mu_assert(DhtNode_Status(node, now) == Bad, "Wrong status");

    node->pending_queries = 0;
    node->reply_time = now;

    mu_assert(DhtNode_Status(node, now) == Good, "Wrong status");

    node->reply_time -= NODE_RESPITE;
    node->query_time = now;

    mu_assert(DhtNode_Status(node, now) == Good, "Wrong status");

    node->query_time -= NODE_RESPITE;
    
    for (node->pending_queries = 0; node->pending_queries < NODE_MAX_PENDING;
	 node->pending_queries++) {
	mu_assert(DhtNode_Status(node, now) == Questionable, "Wrong status");
    }

    mu_assert(DhtNode_Status(node, now) == Bad, "Wrong status");

    free(node);

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
	good_nodes[i].id.value[1] = i;
	good_nodes[i].reply_time = now;
	mu_assert(DhtNode_Status(&good_nodes[i], now) == Good, "Wrong status");

	bad_nodes[i].id.value[0] = id.value[0];
	bad_nodes[i].id.value[2] = i;
	bad_nodes[i].pending_queries = NODE_MAX_PENDING;
	mu_assert(DhtNode_Status(&bad_nodes[i], now) == Bad, "Wrong status");

	far_nodes[i].id.value[0] = ~id.value[0];
	far_nodes[i].id.value[3] = i;
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

    DhtTable *table = DhtTable_Create(&id);
    mu_assert(table != NULL, "DhtTable_Create failed");
    
    DhtNode *node;
    DArray *nodes = DArray_create(sizeof(DhtNode *), HASH_BITS * BUCKET_K);
    mu_assert(nodes != NULL, "DArray_create failed");
    
    DhtTable_InsertNodeResult result;

    int i = 0, rc = 0;

    for (i = 0; i < HASH_BITS; i++)
    {
        int j = 0;
        for (j = 0; j < BUCKET_K; j++)
        {
            node = DhtNode_Create(&inv);
            rc = DArray_push(nodes, node);
            mu_assert(rc == 0, "DArray_push failed");

            DhtHash_Prefix(&node->id, &id, i);

            result = DhtTable_InsertNode(table, node);
            mu_assert(result.rc == OKAdded, "Error adding");
        }
    }

    for (i = 0; i <= HASH_BITS; i++)
    {
        node = DhtNode_Create(&inv);
        DhtHash_Prefix(&node->id, &id, i);

        result = DhtTable_InsertNode(table, node);
        mu_assert(result.rc == OKFull, "Should be full");

        DhtNode_Destroy(node);
    }

    DhtTable_Destroy(table);

    while (DArray_count(nodes) > 0)
    {
        node = DArray_pop(nodes);
        mu_assert(node != NULL, "DArray_pop failed");
        DhtNode_Destroy(node);
    }

    DArray_destroy(nodes);

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
        result = DhtTable_InsertNode(table, node);

        mu_assert(result.rc == OKAdded, "Wrong rc");
        mu_assert(table->end == 1, "Added unnecessary bucket");
    }

    node = DhtNode_Create(&id);
    result = DhtTable_InsertNode(table, node);

    mu_assert(result.rc == OKFull, "Wrong rc");
    mu_assert(table->end == 2, "Added more than one bucket");

    node->id.value[0] = ~0;

    result = DhtTable_InsertNode(table, node);
    mu_assert(result.rc == OKAdded, "Wrong rc");
    mu_assert(table->end == 2, "Added bucket without reason");

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtHash_Clone);
    mu_run_test(test_DhtHash_Prefix);
    mu_run_test(test_DhtHash_PrefixedRandom);
    mu_run_test(test_DhtHash_Distance);
    mu_run_test(test_DhtTable_AddBucket);
    mu_run_test(test_DhtNode_Status);
    mu_run_test(test_DhtTable_InsertNode);
    mu_run_test(test_DhtTable_InsertNode_FullTable);
    mu_run_test(test_DhtTable_InsertNode_AddBucket);

    return NULL;
}

RUN_TESTS(all_tests);
