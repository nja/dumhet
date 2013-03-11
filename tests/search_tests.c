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

    DhtHash_Invert(&invid);

    DhtTable *table = DhtTable_Create(&id);

    int i = 0;
    for (i = 0; i < HASH_BITS; i++)
    {
        DhtTable_InsertNodeResult result;
        DhtNode *node;

        do
        {
            node = DhtNode_Create(&invid);
            int rc = DhtHash_Prefix(&node->id, &id, i);
            mu_assert(rc == 0, "DhtHash_Prefix failed");
            
            result = DhtTable_InsertNode(table, node);
        } while (result.rc == OKAdded);

        mu_assert(result.rc == OKFull, "Unexpected rc");
        DhtNode_Destroy(node);
    }

    int shared = 0;
    for (shared = 0; shared < HASH_BITS; shared++)
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

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Search_CreateDestroy);
    mu_run_test(test_Search_CopyTable);

    return NULL;
}

RUN_TESTS(all_tests);
