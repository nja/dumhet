#ifndef _dht_h
#define _dht_h

#include <dht/bucket.h>
#include <dht/hash.h>
#include <dht/node.h>

typedef struct DhtTable {
    DhtHash id;
    DhtBucket *buckets[HASH_BITS];
    int end;
} DhtTable;

DhtTable *DhtTable_Create(DhtHash *id);
void DhtTable_Destroy(DhtTable *dht);

int DhtTable_ForEachNode(DhtTable *table, int (*NodeOperation)(DhtNode *node));

DhtBucket *DhtTable_AddBucket(DhtTable *table);
DhtBucket *DhtTable_FindBucket(DhtTable *table, DhtNode *node);

enum DhtTable_InsertNodeResultRc {
    ERROR, OKAdded, OKReplaced, OKFull, OKAlreadyAdded
};

typedef struct DhtTable_InsertNodeResult {
    enum DhtTable_InsertNodeResultRc rc;
    DhtBucket *bucket;
    DhtNode *replaced;
} DhtTable_InsertNodeResult;

DhtTable_InsertNodeResult DhtTable_InsertNode(DhtTable *table, DhtNode *node);

#endif
