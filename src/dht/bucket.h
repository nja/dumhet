#ifndef _bucket_h
#define _bucket_h

#include <time.h>

#include <dht/node.h>

#define BUCKET_K 8

typedef struct DhtBucket {
    int index;
    int count;
    time_t change_time;
    DhtNode *nodes[BUCKET_K];
} DhtBucket;

DhtBucket *DhtBucket_Create();
void DhtBucket_Destroy(DhtBucket *bucket);

int DhtBucket_ContainsNode(DhtBucket *bucket, DhtNode *node);

DhtNode *DhtBucket_ReplaceBad(DhtBucket *bucket, DhtNode *node);
DhtNode *DhtBucket_ReplaceQuestionable(DhtBucket *bucket, DhtNode *node);
    
#endif
