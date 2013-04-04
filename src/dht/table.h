#ifndef _dht_h
#define _dht_h

#include <dht/bucket.h>
#include <dht/hash.h>
#include <dht/node.h>

#define MAX_TABLE_BUCKETS (HASH_BITS + 1 - BUCKET_LAST_BITS)

typedef struct Table {
    Hash id;
    Bucket *buckets[MAX_TABLE_BUCKETS];
    int end;
} Table;

Table *Table_Create(Hash *id);
void Table_Destroy(Table *table);
void Table_DestroyNodes(Table *table);

int Table_ForEachNode(Table *table, void *context, NodeOp op);

Bucket *Table_AddBucket(Table *table);
Bucket *Table_FindBucket(Table *table, Hash *id);

Node *Table_FindNode(Table *table, Hash *id);

enum Table_InsertNodeResultRc {
    ERROR, OKAdded, OKReplaced, OKFull, OKAlreadyAdded
};
DArray *Table_GatherClosest(Table *table, Hash *id);

typedef struct Table_InsertNodeResult {
    enum Table_InsertNodeResultRc rc;
    Bucket *bucket;
    Node *replaced;
} Table_InsertNodeResult;

Table_InsertNodeResult Table_InsertNode(Table *table, Node *node);

int Table_MarkReply(Table *table, Hash *id);
void Table_MarkQuery(Table *table, Hash *id);

#endif
