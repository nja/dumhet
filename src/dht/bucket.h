#ifndef _bucket_h
#define _bucket_h

#include <time.h>

#include <lcthw/darray.h>
#include <dht/node.h>

#define BUCKET_K 8
#define BUCKET_LAST_BITS 3

typedef struct DhtBucket {
    int index;
    int count;
    time_t change_time;
    DhtNode *nodes[BUCKET_K];
} DhtBucket;

DhtBucket *DhtBucket_Create();
void DhtBucket_Destroy(DhtBucket *bucket);

int DhtBucket_ContainsNode(DhtBucket *bucket, DhtNode *node);
int DhtBucket_IsFull(DhtBucket *bucket);

DhtNode *DhtBucket_ReplaceBad(DhtBucket *bucket, DhtNode *node);
DhtNode *DhtBucket_ReplaceQuestionable(DhtBucket *bucket, DhtNode *node);

int DhtBucket_AddNode(DhtBucket *bucket, DhtNode *node);

int DhtBucket_GatherGoodNodes(DhtBucket *bucket, DArray *found);

#endif
