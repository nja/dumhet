#ifndef _dht_h
#define _dht_h

#include <stdint.h>
#include <time.h>
#include <random.h>
#include <netinet/in.h>

#define HASH_BYTES 20
#define HASH_BITS (HASH_BYTES * 8)

typedef struct DhtHash {
    char value[HASH_BYTES];	/* Network byte order */
} DhtHash;

void DhtHash_Destroy(DhtHash *hash);
DhtHash *DhtHash_Prefixed(DhtHash *hash, DhtHash *prefix, int prefix_len);
DhtHash *DhtHash_Clone(DhtHash *hash);
int DhtHash_SharedPrefix(DhtHash *a, DhtHash *b);
DhtHash *DhtHash_Random(RandomState *rs);
DhtHash *DhtHash_PrefixedRandom(RandomState *rs, DhtHash *prefix, int prefix_len);

typedef DhtHash DhtDistance;

#define DhtDistance_Destroy(D) DhtHash_Destroy((D))

DhtDistance *DhtHash_Distance(DhtHash *a, DhtHash *b);

int DhtDistance_Compare(DhtDistance *a, DhtDistance *b);

typedef struct DhtNode {
    DhtHash id;
    struct in_addr addr;
    uint16_t port;              /* network byte order */
    time_t reply_time;
    time_t query_time;
    int pending_queries;
} DhtNode;

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
