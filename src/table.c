#include <assert.h>
#include <arpa/inet.h>
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

int Table_CopyAndAddNode(Table *dest, Node *node)
{
    assert(dest != NULL && "NULL Table pointer");
    assert(node != NULL && "NULL Node pointer");

    if (Node_Status(node, time(NULL)) != Good)
    {
        return 0;
    }

    Node *copy = Node_Copy(node);
    check_mem(copy);

    Table_InsertNodeResult result = Table_InsertNode(dest, copy);

    if (result.rc == ERROR
        || result.rc == OKFull
        || result.rc == OKAlreadyAdded)
    {
        Node_Destroy(copy);
    }

    if (result.rc == OKReplaced)
    {
        assert(result.replaced != NULL && "OKReplaced with NULL .replaced");
        Node_Destroy(result.replaced);
    }

    return 0;
error:
    return -1;
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

    Bucket **bucket;
    for (bucket = table->buckets;
         bucket < &table->buckets[table->end];
         bucket++)
    {
        Node **node;
        for (node = (*bucket)->nodes;
             node < &(*bucket)->nodes[BUCKET_K];
             node++)
        {
            if (*node == NULL)
            {
                continue;
            }

            int rc = operate(context, *node);
            check(rc == 0, "Operation on Node failed");
        }
    }

    return 0;
error:
    return -1;
}

int Table_ForEachCloseNode(Table *table, void *context, NodeOp operate)
{
    assert(table != NULL && "NULL Table pointer");
    assert(operate != NULL && "NULL function pointer");

    DArray *nodes = Table_GatherClosest(table, &table->id);
    check(nodes != NULL, "CloseNodes_GetNodes failed");

    while (DArray_count(nodes) > 0)
    {
        Node *node = DArray_pop(nodes);

        int rc = operate(context, node);
        check(rc == 0, "NodeOp on close node failed");
    }

    DArray_destroy(nodes);

    return 0;
error:
    DArray_destroy(nodes);
    return -1;
}

Node *Table_FindNode(Table *table, Hash *id)
{
    assert(table != NULL && "NULL Table pointer");
    assert(id != NULL && "NULL Hash pointer");

    Bucket *bucket = Table_FindBucket(table, id);
    Node **node;

    for (node = bucket->nodes;
         node < &bucket->nodes[BUCKET_K];
         node++)
    {
        if (*node != NULL && Hash_Equals(id, &(*node)->id))
        {
            return *node;
        }
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

int Table_MarkReply(Table *table, Node *node)
{
    assert(table != NULL && "NULL Table pointer");
    assert(node != NULL && "NULL Node pointer");

    Node *found = Table_FindNode(table, &node->id);

    if (found == NULL)
    {
        int rc = Table_CopyAndAddNode(table, node);
        check(rc == 0, "Table_CopyAndAddNode failed");

        Node *added = Table_FindNode(table, &node->id);

        if (added != NULL)
        {
            added->reply_time = time(NULL);
        }

        return 0;
    }

    found->reply_time = time(NULL);

    if (found->pending_queries > 0)
    {
        found->pending_queries--;
    }

    return 0;
error:
    return -1;
}

int Table_MarkQuery(Table *table, Node *node)
{
    assert(table != NULL && "NULL Table pointer");
    assert(node != NULL && "NULL Node pointer");

    Node *found = Table_FindNode(table, &node->id);

    if (found == NULL)
    {
        int rc = Table_CopyAndAddNode(table, node);
        check(rc == 0, "Table_CopyAndAddNode failed");

        found = Table_FindNode(table, &node->id);

        if (found == NULL)
            return 0;
    }

    found->query_time = time(NULL);

    return 0;
error:
    return -1;
}

int AppendLine(bstring dump, Node *node)
{
    assert(dump != NULL && "NULL bstring");
    assert(node != NULL && "NULL Node pointer");

    bstring line = bformat("%s %s %d\n",
                           Hash_Str(&node->id),
                           inet_ntoa(node->addr),
                           node->port);
    int rc = bconcat(dump, line);

    check(rc == BSTR_OK, "bconcat failed");
error:
    bdestroy(line);
    return rc;
}

bstring Table_Dump(Table *table)
{
    assert(table != NULL && "NULL Table pointer");

    bstring dump = bfromcstr("");
    check_mem(dump);

    Node tmp_node = { table->id };

    AppendLine(dump, &tmp_node);

    int rc = Table_ForEachNode(table, dump, (NodeOp)AppendLine);
    check(rc == 0, "Error dumping table nodes");

    return dump;
error:
    bdestroy(dump);
    return NULL;
}

int IsHexCh(char ch)
{
    return ('0' <= ch && ch <= '9')
        || ('a' <= ch && ch <= 'f');
}

int HexVal(char ch, char *val)
{
    if ('0' <= ch && ch <= '9')
    {
        *val = ch - '0';
        return 0;
    }
    else if ('a' <= ch && ch <= 'f')
    {
        *val = 0xa + ch - 'a';
        return 0;
    }

    log_err("Invalid hex digit");
    return -1;
}

int ReadLine(bstring line, Node *result)
{
    struct bstrList *list = bsplit(line, ' ');
    check(list != NULL, "bsplit failed");
    check(list->qty == 3, "Invalid line");

    bstring hashstr = list->entry[0];
    bstring addrstr = list->entry[1];
    bstring portstr = list->entry[2];

    Node node = {{{ 0 }}};

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
        char hich = bchar(hashstr, i * 2);
        char loch = bchar(hashstr, i * 2 + 1);

        char hi = 0, lo = 0;

        check(HexVal(hich, &hi) == 0, "Invalid hash");
        check(HexVal(loch, &lo) == 0, "Invalid hash");

        node.id.value[i] = (hi << 4) | lo;
    }

    int rc = inet_aton(bdata(addrstr), &node.addr);
    check(rc != 0, "Invalid address");

    char *end = NULL;
    long port = strtol(bdatae(portstr, ""), &end, 10);
    check(end != NULL, "strtol error");
    check(*end == '\0', "Invalid port");
    check(0 <= port && port <= (uint16_t)~0, "Invalid port");

    node.port = (uint16_t)port;

    bstrListDestroy(list);
    *result = node;
    return 0;
error:
    bstrListDestroy(list);
    return -1;
}

Table *Table_Read(bstring dump)
{
    Node tmp = {{{ 0 }}};
    Node *node = NULL;
    Table *table = NULL;

    struct bstrList *lines = bsplit(dump, '\n');
    check(lines != NULL, "bsplit failed");
    check(lines->qty > 0, "bsplit failed");

    int rc = ReadLine(lines->entry[0], &tmp);
    check(rc == 0, "ReadLine failed");
    check(tmp.addr.s_addr == 0, "Invalid first line");
    check(tmp.port == 0, "Invalid first line");

    table = Table_Create(&tmp.id);
    check(table != NULL, "Table_Create failed");

    int i = 1;
    for (i = 1; i < lines->qty; i++)
    {
        if (blength(lines->entry[i]) == 0)
        {
            continue;
        }

        node = malloc(sizeof(Node));
        check_mem(node);
        
        rc = ReadLine(lines->entry[i], node);
        check(rc == 0, "ReadLine failed");

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

    bstrListDestroy(lines);
    return table;
error:
    bstrListDestroy(lines);
    Node_Destroy(node);
    Table_DestroyNodes(table);
    Table_Destroy(table);

    return NULL;
}
