#include "minunit.h"
#include <dht/table.h>
#include <dht/search.h>

char *test_Search_CreateDestroy()
{
    Hash id = { "1234512345abcdeabcd" };
    Search *search = Search_Create(&id);
    mu_assert(search != NULL, "Search_Create failed");

    mu_assert(search->table != NULL, "NULL table");
    mu_assert(search->peers != NULL, "NULL peers");
    mu_assert(search->tokens != NULL, "NULL tokens hashmap");

    Search_Destroy(search);

    return NULL;
}

int BaddenTwoThirds(int *i, Node *node)
{
    if ((*i)++ % 3 != 0)
    {
        node->reply_time = 0;
        node->pending_queries = NODE_MAX_PENDING;
    }

    return 0;
}

char *test_Search_CopyTable()
{
    Hash id = { "abcdeABCDE12345!@#$" };
    Hash invid = id;
    Hash_Invert(&invid);

    Table *table = Table_Create(&id);

    const int n = 3 * BUCKET_K;

    int i = 0;
    for (i = 0; i < n; i++)
    {
        Node *node;

        node = Node_Create(&invid);
        int rc = Hash_Prefix(&node->id, &id, i / BUCKET_K);
        node->id.value[8] = i;
        mu_assert(rc == 0, "Hash_PrefixedRandom failed");

        node->reply_time = time(NULL); /* Make node good */

        mu_assert(Node_Status(node, time(NULL)) == Good, "argh");

        rc = Hash_Prefix(&node->id, &id, i);
        mu_assert(rc == 0, "Hash_Prefix failed");

        Table_InsertNodeResult result = Table_InsertNode(table, node);
        mu_assert(result.rc == OKAdded, "Unexpected rc");
    }

    i = 0;
    Table_ForEachNode(table, &i, (NodeOp)BaddenTwoThirds);

    for (i = 0; i < 5; i++)
    {
        Hash searchid = invid;
        Hash_Prefix(&searchid, &id, i);

        Search *search = Search_Create(&searchid);
        mu_assert(search != NULL, "Search_Create failed");
        int rc = Search_CopyTable(search, table);
        mu_assert(rc == 0, "Search_CopyTable failed");

        mu_assert(search->table->end == 1, "Too many buckets");
        mu_assert(search->table->buckets[0]->count == BUCKET_K, "Too few nodes");

        Search_Destroy(search);
    }

    Table_DestroyNodes(table);
    Table_Destroy(table);

    return NULL;
}

char *test_Search_SetGetToken()
{
    Hash id = { "asdf" };
    Search *search = Search_Create(&id);

    Hash nodeid = { "node id" };
    struct FToken token = { .data = "token", .len = 5 };

    int rc = Search_SetToken(search, &nodeid, token);
    mu_assert(rc == 0, "Search_SetToken failed");

    struct FToken *result = Search_GetToken(search, &nodeid);
    mu_assert(result != NULL, "Search_GetToken failed");

    mu_assert(token.len == result->len, "Wrong result len");

    int cmp = memcmp(token.data, result->data, token.len);
    mu_assert(cmp == 0, "Wrong result data");

    Search_Destroy(search);

    return NULL;
}

char *test_Search_NodesToQuery()
{
    time_t now = time(NULL);
    Hash id = { "foo" };
    Search *search = Search_Create(&id);

    Node *already_replied = Node_Create(&id);
    already_replied->reply_time = now;
    already_replied->id.value[0] = 1;

    Node *max_pending = Node_Create(&id);
    max_pending->pending_queries = SEARCH_MAX_PENDING;
    max_pending->id.value[0] = 2;

    Node *just_queried = Node_Create(&id);
    just_queried->query_time = now;
    just_queried->id.value[0] = 3;

    Node *should_query_a = Node_Create(&id);
    should_query_a->pending_queries = 1;
    should_query_a->id.value[0] = 4;

    Node *should_query_b = Node_Create(&id);
    should_query_b->id.value[0] = 5;

    Table_InsertNode(search->table, already_replied);
    Table_InsertNode(search->table, should_query_a);
    Table_InsertNode(search->table, max_pending);
    Table_InsertNode(search->table, just_queried);
    Table_InsertNode(search->table, should_query_b);

    DArray *nodes = DArray_create(sizeof(Node *), 2);

    int rc = Search_NodesToQuery(search, nodes, now);
    mu_assert(rc == 0, "Search_NodesToQuery failed");

    mu_assert(DArray_count(nodes) == 2, "Too many nodes to query");
    mu_assert(DArray_first(nodes) == should_query_a, "Wrong node");
    mu_assert(DArray_last(nodes) == should_query_b, "Bad node");

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
    mu_run_test(test_Search_SetGetToken);

    return NULL;
}

RUN_TESTS(all_tests);
