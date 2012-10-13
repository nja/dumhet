#include "minunit.h"
#include <dht.h>
#include <time.h>

char *test_DhtHash_Distance()
{
    DhtHash ha = {{0}}, hb = {{0}}, hc = {{0}};
    DhtDistance zero_d = {{0}};

    DhtDistance *daa = DhtHash_Distance(&ha, &ha);

    mu_assert(DhtDistance_Compare(&zero_d, daa) == 0, "Wrong distance");

    hb.value[0] = 0x80;
    hc.value[0] = 0xC0;

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
    DhtNode node = {.id = {{0}}, 0};

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

    DhtNode *node = calloc(1, sizeof(DhtNode));

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
    DhtNode *good_nodes = calloc(BUCKET_K, sizeof(DhtNode));
    DhtNode *bad_nodes = calloc(BUCKET_K, sizeof(DhtNode));
    
    time_t now = time(NULL);
    
    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
	good_nodes[i].id.value[0] = i;
	good_nodes[i].reply_time = now;
	mu_assert(DhtNode_Status(&good_nodes[i], now) == Good, "Wrong status");

	bad_nodes[i].id.value[1] = i;
	bad_nodes[i].pending_queries = NODE_MAX_PENDING;
	mu_assert(DhtNode_Status(&bad_nodes[i], now) == Bad, "Wrong status");
    }

    DhtNode *replaced = NULL;
    DhtBucket *bucket = NULL;

    for (i = 0; i < BUCKET_K; i++)
    {
	bucket = DhtTable_InsertNode(table, &bad_nodes[i], &replaced);
	mu_assert(bucket == table->buckets[0], "Wrong bucket");
	mu_assert(replaced == NULL, "Bad replaced");
    }

    for (i = 0; i < BUCKET_K; i++)
    {
	bucket = DhtTable_InsertNode(table, &good_nodes[i], &replaced);
	mu_assert(replaced != NULL, "Nothing replaced");
	mu_assert(&bad_nodes[0] <= replaced
		  && replaced < &bad_nodes[BUCKET_K], "Wrong replaced");
	mu_assert(bucket == table->buckets[0], "Wrong bucket");
    }

    mu_assert(table->end == 1, "Too many buckets");

    free(good_nodes);
    free(bad_nodes);
    DhtTable_Destroy(table);

    return NULL;
}    

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtHash_Distance);
    mu_run_test(test_DhtTable_AddBucket);
    mu_run_test(test_DhtNode_Status);
    mu_run_test(test_DhtTable_InsertNode);

    return NULL;
}

RUN_TESTS(all_tests);
