#ifndef _dht_h
#define _dht_h

#include <netinet/in.h>
#include <stdint.h>
#include <time.h>

#include <dht/hash.h>

typedef struct DhtNode {
    DhtHash id;
    struct in_addr addr;
    uint16_t port;              /* network byte order */
    time_t reply_time;
    time_t query_time;
    int pending_queries;
} DhtNode;

DhtNode *DhtNode_Create(DhtHash *id);
void DhtNode_Destroy(DhtNode *node);
void DhtNode_DestroyBlock(DhtNode *node, size_t count);

#define BUCKET_K 8

typedef struct DhtBucket {
    int index;
    int count;
    time_t change_time;
    DhtNode *nodes[BUCKET_K];
} DhtBucket;

DhtBucket *DhtBucket_Create();
void DhtBucket_Destroy(DhtBucket *bucket);

typedef struct DhtTable {
    DhtHash id;
    DhtBucket *buckets[HASH_BITS];
    int end;
} DhtTable;

DhtTable *DhtTable_Create(DhtHash *id);
void DhtTable_Destroy(DhtTable *dht);

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

typedef enum DhtNodeStatus { Unknown, Good, Questionable, Bad } DhtNodeStatus;

#define NODE_RESPITE (15 * 60)
#define NODE_MAX_PENDING 2

DhtNodeStatus DhtNode_Status(DhtNode *node, time_t time);

#endif
