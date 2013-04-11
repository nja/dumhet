#include <time.h>

#include "minunit.h"
#include <dht/table.h>
#include <lcthw/darray.h>

char *test_Table_AddBucket()
{
    int i = 0;
    Table table = {.id = {{0}}, .buckets = {0}, .end = 0};
    Node node = {.id = {{0}}, {0}};

    for (i = 0; i < HASH_BYTES; i++)
    {
	table.id.value[i] = 0xAA;
	node.id.value[i] = 0xAA;
    }
    node.id.value[HASH_BYTES - 1]++;

    for (i = 0; i < MAX_TABLE_BUCKETS; i++)
    {
	Bucket *added = Table_AddBucket(&table);
	Bucket *found = Table_FindBucket(&table, &node.id);

	mu_assert(table.end == i + 1, "Wrong bucket count");
	mu_assert(added == found, "Found wrong bucket");
    }

    for (i = 0; i < MAX_TABLE_BUCKETS; i++)
    {
	Bucket *found = Table_FindBucket(&table, &node.id);
	mu_assert(found == table.buckets[table.end - 1], "Expected last bucket");
	
	node.id.value[i / 8] ^= (0x80 >> (i % 8));
	found = Table_FindBucket(&table, &node.id);

	mu_assert(found == table.buckets[i], "Found wrong bucket");
	node.id.value[i / 8] ^= (0x80 >> (i % 8));
    }

    for (i = 0; i < MAX_TABLE_BUCKETS; i++)
    {
	free(table.buckets[i]);
    }

    return NULL;
}

char *test_Table_InsertNode()
{
    Hash id = {{0}};
    
    Table *table = Table_Create(&id);

    /* good and bad are near the id and shiftable to new buckets */
    Node *good_nodes = calloc(BUCKET_K + 1, sizeof(Node));
    Node *bad_nodes = calloc(BUCKET_K + 1, sizeof(Node));

    /* far only fit in the first bucket */
    Node *far_nodes = calloc(BUCKET_K + 1, sizeof(Node));
    
    time_t now = time(NULL);

    /* create nodes */
    int i = 0;
    for (i = 0; i < BUCKET_K + 1; i++)
    {
	good_nodes[i].id.value[0] = id.value[0];
	good_nodes[i].id.value[1] = ~i;
	good_nodes[i].reply_time = now;
	mu_assert(Node_Status(&good_nodes[i], now) == Good, "Wrong status");

	bad_nodes[i].id.value[0] = id.value[0];
	bad_nodes[i].id.value[2] = ~i;
	bad_nodes[i].pending_queries = NODE_MAX_PENDING;
	mu_assert(Node_Status(&bad_nodes[i], now) == Bad, "Wrong status");

	far_nodes[i].id.value[0] = ~id.value[0];
	far_nodes[i].id.value[3] = ~i;
	far_nodes[i].reply_time = now;
	mu_assert(Node_Status(&far_nodes[i], now) == Good, "Wrong status");
    }

    Table_InsertNodeResult result;

    /* fill first bucket with bad nodes */
    for (i = 0; i < BUCKET_K; i++)
    {
	result = Table_InsertNode(table, &bad_nodes[i]);
	mu_assert(result.rc == OKAdded, "Wrong result");
	mu_assert(result.bucket == table->buckets[0], "Wrong bucket");
	mu_assert(result.replaced == NULL, "Bad replaced");
    }

    /* replace bad nodes with good */
    for (i = 0; i < BUCKET_K; i++)
    {
	result = Table_InsertNode(table, &good_nodes[i]);
	mu_assert(result.rc == OKReplaced, "Wrong result");
	mu_assert(result.replaced != NULL, "Nothing replaced");
	mu_assert(&bad_nodes[0] <= result.replaced
		  && result.replaced < &bad_nodes[BUCKET_K], "Wrong replaced");
	mu_assert(result.bucket == table->buckets[0], "Wrong bucket");
    }

    /* readd a single good node */
    result = Table_InsertNode(table, &good_nodes[0]);
    mu_assert(result.rc == OKAlreadyAdded, "Wrong result");
    mu_assert(result.replaced == NULL, "Bad replaced");
    mu_assert(result.bucket == table->buckets[0], "Wrong bucket");

    /* adding far nodes, will shift good nodes to second bucket */
    for (i = 0; i < BUCKET_K; i++)
    {
	result = Table_InsertNode(table, &far_nodes[i]);
	mu_assert(result.rc == OKAdded, "Wrong result");
	mu_assert(result.bucket == table->buckets[0], "Wrong bucket");
	mu_assert(result.replaced == NULL, "Bad replaced");
    }

    /* the ninth far node will not be added: the first bucket is full */
    result = Table_InsertNode(table, &far_nodes[BUCKET_K]);
    mu_assert(result.rc == OKFull, "Wrong result");
    mu_assert(result.bucket == NULL, "Bad bucket");
    mu_assert(result.replaced == NULL, "Bad replaced");
    
    /* readd a single good node  */
    result = Table_InsertNode(table, &good_nodes[0]);
    mu_assert(result.rc == OKAlreadyAdded, "Wrong result");
    mu_assert(result.replaced == NULL, "Bad replaced");
    mu_assert(result.bucket == table->buckets[1], "Wrong bucket");

    mu_assert(table->end == 2, "Too many buckets");

    free(good_nodes);
    free(bad_nodes);
    free(far_nodes);
    Table_Destroy(table);

    return NULL;
}

char *test_Table_InsertNode_FullTable()
{
    Hash id = {{0}};
    Hash inv = {{0}};

    Hash_Invert(&inv);

    const int low_bits = 3;
    mu_assert(1 << low_bits >= BUCKET_K, "Not enough bits");

    Table *table = Table_Create(&id);
    mu_assert(table != NULL, "Table_Create failed");
    
    Table_InsertNodeResult result;

    Node *node = Node_Create(&id);
    result = Table_InsertNode(table, node);
    mu_assert(result.rc == OKAdded, "Error adding id node");

    int i = 0;
    for (i = 0; i < HASH_BITS; i++)
    {
        int suffix = HASH_BITS - i;
        int shift = low_bits < suffix ? low_bits : suffix - 1;

        int j = 0;
        for (j = 0; j < BUCKET_K && j < 1 << shift; j++)
        {
            node = Node_Create(&inv);

            node->id.value[HASH_BYTES - 1] &= ~0 << shift;
            node->id.value[HASH_BYTES - 1] |= j;

            Hash_Prefix(&node->id, &id, i);

            result = Table_InsertNode(table, node);
            mu_assert(result.rc == OKAdded, "Error adding");
        }
    }

    for (i = 0; i < table->end; i++)
    {
        mu_assert(table->buckets[i]->count == BUCKET_K, "Bucket not full");
    }

    for (i = 0; i < MAX_TABLE_BUCKETS - 2; i++)
    {
        node = Node_Create(&inv);
        Hash_Prefix(&node->id, &id, i);
        node->id.value[HASH_BYTES - 1] &= 0xf0;
        
        result = Table_InsertNode(table, node);
        mu_assert(result.rc == OKFull, "Should be full");

        Node_Destroy(node);
    }

    Table_DestroyNodes(table);
    Table_Destroy(table);

    return NULL;
}

char *test_Table_InsertNode_AddBucket()
{
    Hash id = {{ 0 }};
    Table *table = Table_Create(&id);

    Node *node;
    Table_InsertNodeResult result;

    int i = 0;
    for (i = 0; i < BUCKET_K; i++)
    {
        node = Node_Create(&id);
        node->id.value[HASH_BYTES - 1] = i;
        result = Table_InsertNode(table, node);

        mu_assert(result.rc == OKAdded, "Wrong rc");
        mu_assert(table->end == 1, "Added unnecessary bucket");
    }

    node = Node_Create(&id);
    node->id.value[HASH_BYTES - 1] = i;
    result = Table_InsertNode(table, node);

    mu_assert(result.rc == OKFull, "Wrong rc");
    mu_assert(table->end == 2, "Added more than one bucket");

    node->id.value[0] = ~0;

    result = Table_InsertNode(table, node);
    mu_assert(result.rc == OKAdded, "Wrong rc");
    mu_assert(table->end == 2, "Added bucket without reason");

    Table_DestroyNodes(table);
    Table_Destroy(table);

    return NULL;
}

Node **MakeNodes(int count, char high)
{
    Node **nodes = calloc(count, sizeof(Node *));

    int i = 0;
    for (i = 0; i < count; i++)
    {
        Hash id = {{ 0 }};
        id.value[0] = high;
        id.value[2] = i + 1;
        nodes[i] = Node_Create(&id);
        nodes[i]->reply_time = time(NULL);
    }

    return nodes;
}

int HasNode(DArray *nodes, Node *node)
{
    int i = 0;
    for (i = 0; i < DArray_end(nodes); i++)
    {
        if (node == DArray_get(nodes, i))
            return 1;
    }

    return 0;
}

char *test_Table_GatherClosest()
{
    Hash id = {{ 0 }};

    int i = 0;
    for (i = 0; i <= BUCKET_K; i++)
    {
        Table *table = Table_Create(&id);

        int far = i;
        int close = BUCKET_K - i;

        Node **far_nodes = MakeNodes(far, 0x40);
        Node **close_nodes = MakeNodes(close, 0x20);
        Node **filler_nodes = MakeNodes(BUCKET_K, 0x80);

        Table_InsertNodeResult result;
        int j = 0;
        for (j = 0; j < BUCKET_K; j++)
        {
            result = Table_InsertNode(table, filler_nodes[j]);
            mu_assert(result.rc == OKAdded, "add");
        }

        for (j = 0; j < far; j++)
        {
            result = Table_InsertNode(table, far_nodes[j]);
            mu_assert(result.rc == OKAdded, "add");
        }
        
        for (j = 0; j < close; j++)
        {
            result = Table_InsertNode(table, close_nodes[j]);
            mu_assert(result.rc == OKAdded, "add");
        }            

        Hash target = {{ 0 }};
        target.value[1] = 16;

        DArray *found = Table_GatherClosest(table, &target);

        for (j = 0; j < BUCKET_K; j++)
            mu_assert(!HasNode(found, filler_nodes[j]), "Filler node found");

        for (j = 0; j < far; j++)
            mu_assert(HasNode(found, far_nodes[j]), "Far node missing");

        for (j = 0; j < close; j++)
            mu_assert(HasNode(found, close_nodes[j]), "Close node missing");

        Table_ForEachNode(table, NULL, Node_DestroyOp);
        Table_Destroy(table);
        DArray_destroy(found);

        free(far_nodes);
        free(close_nodes);
        free(filler_nodes);
    }

    return NULL;
}

char *test_Table_FindNode_EmptyBucket()
{
    Hash id = { "id" };
    Table *table = Table_Create(&id);

    Node *node = Table_FindNode(table, &id);
    mu_assert(node == NULL, "Mystery node found");

    Table_Destroy(table);

    return NULL;
}

Table *RandomTable(int buckets)
{
    Hash id;
    Node *node = NULL;
    Table *table = NULL;
    RandomState *rs = RandomState_Create(buckets);
    check(rs != NULL, "RandomState_Create failed");

    int rc = Hash_Random(rs, &id);
    check(rc == 0, "Hash_Random failed");

    table = Table_Create(&id);
    check(table != NULL, "Table_Create failed");

    int i = 0, j = 0;

    for (i = 0; i < buckets; i++)
    {
        for (j = 0; j < BUCKET_K; j++)
        {
            rc = Hash_PrefixedRandom(rs, &id, &table->id, i);
            check(rc == 0, "Hash_PrefixedRandom failed");

            node = Node_Create(&id);
            check(node != NULL, "Node_Create failed");

            rc = Random_Fill(rs, (char *)&node->addr, sizeof(node->addr));
            check(rc == 0, "Random_Fill failed");

            rc = Random_Fill(rs, (char *)&node->port, sizeof(node->port));
            check(rc == 0, "Random_Fill failed");

            Table_InsertNodeResult result =
                Table_InsertNode(table, node);

            check(result.rc != ERROR, "Table_InsertNode failed");
            Node_Destroy(result.replaced);

            if (result.rc != OKAdded)
            {
                Node_Destroy(node);
                node = NULL;
            }
        }
    }

    RandomState_Destroy(rs);

    return table;
error:
    RandomState_Destroy(rs);
    Node_Destroy(node);
    Table_DestroyNodes(table);
    Table_Destroy(table);

    return NULL;
}

int AlsoHasNode(Table *table, Node *node)
{
    Node *other = Table_FindNode(table, &node->id);
    check(other != NULL, "Table_FindNode failed");
    check(Node_Same(other, node), "Nodes differ");

    return 0;
error:
    return -1;
}

char *test_TableDump()
{
    Table *table = RandomTable(17);
    mu_assert(table != NULL, "RandomTable failed");

    bstring dump = Table_Dump(table);
    mu_assert(dump != NULL, "Table_Dump failed");

    Table *read = Table_Read(dump);
    mu_assert(read != NULL, "Table_Read failed");

    int rc = Table_ForEachNode(table, read, (NodeOp)AlsoHasNode);
    mu_assert(rc == 0, "Nodes lost after dump and read");

    bdestroy(dump);
    Table_DestroyNodes(table);
    Table_Destroy(table);
    Table_DestroyNodes(read);
    Table_Destroy(read);

    return NULL;
}


char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Table_AddBucket);
    mu_run_test(test_Table_InsertNode);
    mu_run_test(test_Table_InsertNode_FullTable);
    mu_run_test(test_Table_InsertNode_AddBucket);
    mu_run_test(test_Table_GatherClosest);
    mu_run_test(test_Table_FindNode_EmptyBucket);
    mu_run_test(test_TableDump);

    return NULL;
}

RUN_TESTS(all_tests);
