#include "minunit.h"
#include <dht/table.h>
#include <dht/search.h>

char *test_Search_CreateDestroy()
{
    DhtHash id = { "1234512345abcdeabcd" };
    Search *search = Search_Create(&id);
    mu_assert(search != NULL, "Search_Create failed");

    Search_Destroy(search);

    return NULL;
}

char *test_Search_CopyTable()
{
    DhtHash id = { "abcdeABCDE12345!@#$" };
    DhtHash invid = id;

    const int low_bits = 3;
    mu_assert(1 << low_bits >= BUCKET_K, "Not enough bits");

    DhtHash_Invert(&invid);

    DhtTable *table = DhtTable_Create(&id);

    int i = 0;
    for (i = 0; i < HASH_BITS - low_bits; i++)
    {
        DhtTable_InsertNodeResult result;
        DhtNode *node;

        int j = 0;
        for (j = 0; j < BUCKET_K; j++)
        {
            node = DhtNode_Create(&invid);

            node->id.value[HASH_BYTES - 1] &= ~0 << low_bits;
            node->id.value[HASH_BYTES - 1] |= j;

            int rc = DhtHash_Prefix(&node->id, &id, i);
            mu_assert(rc == 0, "DhtHash_Prefix failed");
            
            result = DhtTable_InsertNode(table, node);
            mu_assert(result.rc == OKAdded, "Unexpected rc");
        }
    }

    int shared = 0;
    for (shared = 0; shared < HASH_BITS - low_bits; shared++)
    {
        DhtHash target = invid;
        int rc = DhtHash_Prefix(&target, &id, shared);
        mu_assert(rc == 0, "DhtHash_Prefix failed");
        mu_assert(DhtHash_SharedPrefix(&id, &target) == shared, "Wrong shared prefix");

        Search *search = Search_Create(&target);
        rc = Search_CopyTable(search, table);
        mu_assert(rc == 0, "Search_CopyTable failed");

        mu_assert(search->table->end - 1 == shared, "Wrong sized search table");

        for (i = 0; i < shared; i++)
        {
            DhtBucket *bucket = table->buckets[i];

            int j = 0;
            for (j = 0; j < BUCKET_K; j++)
            {
                DhtNode *node = bucket->nodes[j];

                if (node == NULL)
                {
                    continue;
                }

                DhtNode *found = DhtTable_FindNode(search->table, &node->id);
                mu_assert(found != NULL, "Missing node from original table");
            }
        }

        DhtTable_ForEachNode(search->table, NULL, DhtNode_DestroyOp);
        Search_Destroy(search);
    }

    DhtTable_DestroyNodes(table);
    DhtTable_Destroy(table);

    return NULL;
}

char *test_Search_NodesToQuery()
{
    time_t now = time(NULL);
    DhtHash id = { "foo" };
    Search *search = Search_Create(&id);

    DhtNode *already_replied = DhtNode_Create(&id);
    already_replied->reply_time = now;
    already_replied->id.value[0] = 1;

    DhtNode *max_pending = DhtNode_Create(&id);
    max_pending->pending_queries = SEARCH_MAX_PENDING;
    max_pending->id.value[0] = 2;

    DhtNode *just_queried = DhtNode_Create(&id);
    just_queried->query_time = now;
    just_queried->id.value[0] = 3;

    DhtNode *should_query_a = DhtNode_Create(&id);
    should_query_a->pending_queries = 1;
    should_query_a->id.value[0] = 4;

    DhtNode *should_query_b = DhtNode_Create(&id);
    should_query_b->id.value[0] = 5;

    DhtTable_InsertNode(search->table, already_replied);
    DhtTable_InsertNode(search->table, should_query_a);
    DhtTable_InsertNode(search->table, max_pending);
    DhtTable_InsertNode(search->table, just_queried);
    DhtTable_InsertNode(search->table, should_query_b);

    DArray *nodes = DArray_create(sizeof(DhtNode *), 2);

    int rc = Search_NodesToQuery(search, nodes, now);
    mu_assert(rc == 0, "Search_NodesToQuery failed");

    mu_assert(DArray_count(nodes) == 2, "Too many nodes to query");
    mu_assert(DArray_first(nodes) == should_query_a, "Wrong node");
    mu_assert(DArray_last(nodes) == should_query_b, "Bad node");

    DhtTable_DestroyNodes(search->table);
    Search_Destroy(search);
    DArray_destroy(nodes);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Search_CreateDestroy);
    mu_run_test(test_Search_CopyTable);
    mu_run_test(test_Search_NodesToQuery);

    return NULL;
}

RUN_TESTS(all_tests);
