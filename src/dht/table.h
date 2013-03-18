#ifndef _dht_h
#define _dht_h

#include <dht/bucket.h>
#include <dht/hash.h>
#include <dht/node.h>

#define MAX_TABLE_BUCKETS (HASH_BITS + 1 - BUCKET_LAST_BITS)

typedef struct DhtTable {
    DhtHash id;
    DhtBucket *buckets[MAX_TABLE_BUCKETS];
    int end;
} DhtTable;

DhtTable *DhtTable_Create(DhtHash *id);
void DhtTable_Destroy(DhtTable *table);
void DhtTable_DestroyNodes(DhtTable *table);

int DhtTable_ForEachNode(DhtTable *table, void *context, NodeOp op);

DhtBucket *DhtTable_AddBucket(DhtTable *table);
DhtBucket *DhtTable_FindBucket(DhtTable *table, DhtHash *id);

DhtNode *DhtTable_FindNode(DhtTable *table, DhtHash *id);

enum DhtTable_InsertNodeResultRc {
    ERROR, OKAdded, OKReplaced, OKFull, OKAlreadyAdded
};
DArray *DhtTable_GatherClosest(DhtTable *table, DhtHash *id);


typedef struct DhtTable_InsertNodeResult {
    enum DhtTable_InsertNodeResultRc rc;
    DhtBucket *bucket;
    DhtNode *replaced;
} DhtTable_InsertNodeResult;

DhtTable_InsertNodeResult DhtTable_InsertNode(DhtTable *table, DhtNode *node);

int DhtTable_MarkReply(DhtTable *table, DhtHash *id);

#endif
