#ifndef _bucket_h
#define _bucket_h

#include <time.h>

#include <lcthw/darray.h>
#include <dht/node.h>

#define BUCKET_K 8
#define BUCKET_LAST_BITS 3

typedef struct Bucket {
    int index;
    int count;
    time_t change_time;
    DhtNode *nodes[BUCKET_K];
} Bucket;

Bucket *Bucket_Create();
void Bucket_Destroy(Bucket *bucket);

int Bucket_ContainsNode(Bucket *bucket, DhtNode *node);
int Bucket_IsFull(Bucket *bucket);

DhtNode *Bucket_ReplaceBad(Bucket *bucket, DhtNode *node);
DhtNode *Bucket_ReplaceQuestionable(Bucket *bucket, DhtNode *node);

int Bucket_AddNode(Bucket *bucket, DhtNode *node);

int Bucket_GatherGoodNodes(Bucket *bucket, DArray *found);

#endif
