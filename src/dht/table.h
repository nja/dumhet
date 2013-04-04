#ifndef _dht_h
#define _dht_h

#include <dht/bucket.h>
#include <dht/hash.h>
#include <dht/node.h>

#define MAX_TABLE_BUCKETS (HASH_BITS + 1 - BUCKET_LAST_BITS)

typedef struct DhtTable {
    Hash id;
    Bucket *buckets[MAX_TABLE_BUCKETS];
    int end;
} DhtTable;

DhtTable *DhtTable_Create(Hash *id);
void DhtTable_Destroy(DhtTable *table);
void DhtTable_DestroyNodes(DhtTable *table);

int DhtTable_ForEachNode(DhtTable *table, void *context, NodeOp op);

Bucket *DhtTable_AddBucket(DhtTable *table);
Bucket *DhtTable_FindBucket(DhtTable *table, Hash *id);

Node *DhtTable_FindNode(DhtTable *table, Hash *id);

enum DhtTable_InsertNodeResultRc {
    ERROR, OKAdded, OKReplaced, OKFull, OKAlreadyAdded
};
DArray *DhtTable_GatherClosest(DhtTable *table, Hash *id);


typedef struct DhtTable_InsertNodeResult {
    enum DhtTable_InsertNodeResultRc rc;
    Bucket *bucket;
    Node *replaced;
} DhtTable_InsertNodeResult;

DhtTable_InsertNodeResult DhtTable_InsertNode(DhtTable *table, Node *node);

int DhtTable_MarkReply(DhtTable *table, Hash *id);
void DhtTable_MarkQuery(DhtTable *table, Hash *id);

#endif
